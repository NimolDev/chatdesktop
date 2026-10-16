#include "storage/local_storage.hpp"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <unistd.h>

using core::storage::LocalStorage;

namespace {

void check(bool condition, const char *message)
{
    if (!condition) {
        throw std::runtime_error(message);
    }
}

class TemporaryDirectory
{
public:
    TemporaryDirectory()
    {
        std::string pattern = (std::filesystem::temp_directory_path() / "local-storage-XXXXXX").string();
        if (!::mkdtemp(pattern.data())) {
            throw std::runtime_error("Cannot create temporary directory");
        }
        path = pattern;
    }

    ~TemporaryDirectory()
    {
        std::error_code error;
        std::filesystem::remove_all(path, error);
    }

    std::filesystem::path path;
};

void roundTrip()
{
    TemporaryDirectory directory;
    const auto path = (directory.path / "ទិន្នន័យ.store").string();
    const std::string password = "ពាក្យសម្ងាត់🔐";
    const std::string rootName = "អ្នកប្រើប្រាស់🌏";
    std::string payload = "Hello 世界 សួស្តី";
    for (int repeat = 0; repeat < 20; ++repeat) {
        for (int byte = 0; byte < 256; ++byte) {
            payload.push_back(static_cast<char>(byte));
        }
    }

    std::uint64_t record = 0;
    std::uint64_t empty = 0;
    {
        LocalStorage storage(path, password, {128, true});
        check(storage.blockSize() == 128, "Constructor options were ignored");
        record = storage.insert(payload, 7);
        empty = storage.insert("", 7);
        check(storage.get(record) == payload, "Binary payload did not round trip");
        check(storage.get(empty) == "", "Empty payload did not round trip");
        storage.setRoot(rootName, record);
        storage.setRoot(std::string("binary\0root", 11), empty);
        check(storage.recordCount() == 2, "Directory counted as a user record");
        check(storage.ids(LocalStorage::kDirTag).empty(), "Directory exposed by ids");
        const auto ids = storage.ids(7);
        check(ids.size() == 2 && std::is_sorted(ids.begin(), ids.end()), "IDs are not ascending");
        std::size_t visits = 0;
        storage.forEach(7, [&](std::uint64_t id, const std::string &data) {
            check(storage.get(id) == data, "Callback cannot reenter storage");
            ++visits;
        });
        check(visits == 2, "forEach missed a record");
        check(storage.update(empty, payload), "Growing a record failed");
        check(storage.get(empty) == payload, "Growing a record corrupted data");
        check(storage.update(empty, ""), "Shrinking a record failed");
        check(!storage.update(999999, "missing"), "Updated nonexistent record");
        storage.sync();
    }
    {
        LocalStorage storage(path, password);
        check(storage.blockSize() == 128, "Stored block size not restored");
        check(storage.root(rootName) == record, "Unicode root not restored");
        check(storage.root(std::string("binary\0root", 11)) == empty, "Binary root not restored");
        check(storage.get(record) == payload, "Binary payload changed after reopening");
        check(storage.get(empty) == "", "Empty payload changed after reopening");
        check(storage.removeRoot(std::string("binary\0root", 11)), "Removing root failed");
        check(!storage.removeRoot("missing"), "Removed nonexistent root");
        check(storage.remove(record), "Removing record failed");
        check(!storage.exists(record) && !storage.get(record), "Removed record still present");
        check(!storage.root(rootName), "Removed record still has a root");
        record = storage.insert(payload, 7);
        storage.setRoot(rootName, record);
        storage.sync();
    }
    {
        LocalStorage storage(path, password);
        check(storage.root(rootName) == record, "Recycled record root not restored");
        check(storage.get(record) == payload, "Recycled record payload corrupted");
        check(storage.recordCount() == 2, "Incorrect record count after recycling blocks");
        const auto ids = storage.ids(7);
        check(ids.size() == 2 && std::is_sorted(ids.begin(), ids.end()), "Tag index not restored");
    }
    bool rejected = false;
    try {
        LocalStorage storage(path, "incorrect password");
    } catch (const std::runtime_error &) {
        rejected = true;
    }
    check(rejected, "Incorrect password accepted");
}

void deferredOpen()
{
    TemporaryDirectory directory;
    const auto path = (directory.path / "deferred.store").string();
    LocalStorage storage;
    bool rejected = false;
    try {
        storage.insert("too early");
    } catch (const std::logic_error &) {
        rejected = true;
    }
    check(rejected, "Unopened insertion was not rejected");
    storage.setPath(path, "password");
    const auto id = storage.insert("saved value", 1);
    storage.setRoot("user", id);
    storage.setPath(path, "password");
    check(storage.get(id) == "saved value", "Repeated setPath lost data");
    check(storage.root("user") == id, "Repeated setPath lost roots");
    const auto alias = directory.path / "alias.store";
    std::filesystem::create_symlink(path, alias);
    storage.setPath(alias.string(), "password");
    rejected = false;
    try {
        storage.setPath(path, "incorrect");
    } catch (const std::runtime_error &) {
        rejected = true;
    }
    check(rejected, "Same-file switch accepted an incorrect password");
    check(storage.get(id) == "saved value", "Failed authentication damaged live key");
    check(storage.update(id, "updated"), "Cannot write after failed authentication");
    // Repeated setPath must retain the exclusive lock on the active file.
    rejected = false;
    try {
        LocalStorage other(path, "password");
    } catch (const std::runtime_error &) {
        rejected = true;
    }
    check(rejected, "Same-file verification released the active file lock");
}

void switchStore()
{
    TemporaryDirectory directory;
    const auto firstPath = (directory.path / "first.store").string();
    const auto secondPath = (directory.path / "second.store").string();
    const auto thirdPath = (directory.path / "third.store").string();
    std::uint64_t secondId = 0;
    {
        LocalStorage second(secondPath, "second password", {256, false});
        secondId = second.insert(std::string(1000, 'b'), 2);
        second.setRoot("second", secondId);
    }

    LocalStorage storage(firstPath, "first password", {128, true});
    const auto firstId = storage.insert("first record", 1);
    storage.setRoot("first", firstId);
    for (const auto &target : {secondPath, (directory.path / "missing" / "store").string(), firstPath}) {
        bool rejected = false;
        try {
            storage.setPath(target, "wrong password");
        } catch (const std::runtime_error &) {
            rejected = true;
        }
        check(rejected, "Invalid switch accepted");
        check(storage.get(firstId) == "first record", "Failed switch damaged current data");
        check(storage.root("first") == firstId, "Failed switch damaged current roots");
        check(storage.blockSize() == 128, "Failed switch changed geometry");
    }

    storage.setPath(secondPath, "second password");
    check(storage.blockSize() == 256, "Switch did not load target geometry");
    check(storage.root("second") == secondId, "Switch did not load target roots");
    check(!storage.root("first") && storage.ids(1).empty(), "Old indexes leaked into new store");
    check(storage.get(secondId) == std::string(1000, 'b'), "Switch did not load target data");
    check(storage.update(secondId, "updated second"), "Cannot write after switching");
    {
        LocalStorage first(firstPath, "first password");
        check(first.get(firstId) == "first record", "Old store damaged or still locked");
    }

    storage.setPath(thirdPath, "third password");
    check(storage.recordCount() == 0 && storage.blockCount() == 0, "New store not empty");
    check(storage.freeBlock() == 0 && !storage.root("second"), "Old state leaked into new store");
    check(storage.blockSize() == 128, "New store did not retain constructor options");
    const auto thirdId = storage.insert("third record", 3);
    storage.setRoot("third", thirdId);

    storage.setPath(secondPath, "second password");
    check(storage.get(secondId) == "updated second", "Writes after switching were not persisted");
    storage.setPath(thirdPath, "third password");
    check(storage.get(thirdId) == "third record", "New store password or data is incorrect");
    check(storage.root("third") == thirdId, "New store root was not persisted");
}

} // namespace

int main()
{
    try {
        roundTrip();
        switchStore();
        deferredOpen();
        std::cout << "LocalStorage tests passed\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
