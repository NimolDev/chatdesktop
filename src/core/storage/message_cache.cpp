#include "message_cache.hpp"

#include "file_name_helper.hpp"
#include "local_storage.hpp"
#include "app_constants.hpp"

#include <mutex>
#include <exception>
#include <list>
#include <memory>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

namespace core {
namespace storage {

namespace {

constexpr std::uint32_t kMessageTag = 2;
std::mutex storeMutex;

QDir storeDirectory() {
    const QDir appData(QStandardPaths::writableLocation (QStandardPaths::AppDataLocation));
    return QDir(appData.filePath (QStringLiteral ("user_data")));
}

QString fileName(const QString &logical_name, const QString &secret)
{
    return FileNameHelper::hexName(logical_name, secret);
}
std::string storePassword()
{
    return core::constants::AppConstants::appSecretKey ().toStdString ();
}

struct OpenStore {
    QString path;
    std::unique_ptr<LocalStorage> storage;
};
// Accessed only while storeMutex is held; least recently used entries are last.
std::list<OpenStore> openStores;
constexpr size_t kMaxOpenStores = 8;

LocalStorage &openStore(const QString &path)
{
    for (auto it = openStores.begin(); it != openStores.end(); ++it) {
        if (it->path == path) {
            if (QFileInfo::exists(path)) {
                openStores.splice(openStores.begin(), openStores, it);
                return *openStores.front().storage;
            }
            openStores.erase(it);
            break;
        }
    }
    auto storage = std::make_unique<LocalStorage>(path.toStdString(), storePassword());
    if (openStores.size() >= kMaxOpenStores) {
        openStores.pop_back();
    }
    openStores.push_front({path, std::move(storage)});
    return *openStores.front().storage;
}

std::string pageRoot(int page)
{
    return  std::to_string(page);
}

} // namespace



bool MessageCache::save(const QString &objects,
                        const QString &logical_name,
                        const QString &secret_password)
{
    std::lock_guard lock(storeMutex);
    try {
        const QDir directory = storeDirectory();
        if (!QDir().mkpath(directory.absolutePath())) {
            qWarning() << "Could not create message cache directory";
            return false;
        }
        const QString file = directory.filePath(fileName(logical_name, secret_password));
        auto &storage = openStore(file);
        storage.insert(objects.toStdString(), kMessageTag);
        storage.sync();
        return true;
    } catch (const std::exception &error) {
        openStores.clear();
        qWarning() << "Could not save message cache:" << error.what();
        return false;
    }
}

std::optional<QStringList> MessageCache::read(const QString &logical_name,
                                               const QString &secret_password)
{
    std::lock_guard lock(storeMutex);
    try {
        const QString file = storeDirectory().filePath(fileName(logical_name, secret_password));
        if (!QFileInfo::exists(file)) {
            return QStringList{};
        }
        auto &storage = openStore(file);
        QStringList batches;
        for (const auto id : storage.ids(kMessageTag)) {
            const auto record = storage.get(id);
            if (!record) {
                qWarning() << "Message cache record is missing";
                return std::nullopt;
            }
            batches.append(QString::fromUtf8(record->data(), static_cast<qsizetype>(record->size())));
        }
        return batches;
    } catch (const std::exception &error) {
        openStores.clear();
        qWarning() << "Could not read message cache:" << error.what();
        return std::nullopt;
    }
}

bool MessageCache::savePage(const QString &objects, const QString &logical_name,
                            const QString &secret_password, int page)
{
    if (page < 1) {
        return false;
    }
    return saveSnapshot(objects, logical_name, secret_password, QString::fromStdString(pageRoot(page)));
}

bool MessageCache::savePending(const QString &objects, const QString &logical_name,
                               const QString &secret_password)
{
    return saveSnapshot(objects, logical_name, secret_password, QStringLiteral("pending-messages"));
}

bool MessageCache::saveSnapshot(const QString &objects, const QString &logical_name,
                                const QString &secret_password, const QString &root)
{
    std::lock_guard lock(storeMutex);
    try {
        const QDir directory = storeDirectory();
        if (!QDir().mkpath(directory.absolutePath())) {
            return false;
        }
        const QString file = directory.filePath(fileName(logical_name, secret_password));
        auto &storage = openStore(file);
        const auto id = storage.root(root.toStdString());
        if (!id) {
            storage.setRoot(root.toStdString(), storage.insert(objects.toStdString(), kMessageTag));
        } else if (!storage.update(*id, objects.toStdString())) {
            return false;
        }
        storage.sync();
        return true;
    } catch (const std::exception &error) {
        openStores.clear();
        qWarning() << "Could not save message cache page:" << error.what();
        return false;
    }
}

std::optional<QString> MessageCache::readPage(const QString &logical_name,
                                             const QString &secret_password, int page)
{
    if (page < 1) {
        return std::nullopt;
    }
    return readSnapshot(logical_name, secret_password, QString::fromStdString(pageRoot(page)));
}

std::optional<QString> MessageCache::readPending(const QString &logical_name,
                                                const QString &secret_password)
{
    return readSnapshot(logical_name, secret_password, QStringLiteral("pending-messages"));
}

std::optional<QString> MessageCache::readSnapshot(const QString &logical_name,
                                                 const QString &secret_password, const QString &root)
{
    std::lock_guard lock(storeMutex);
    try {
        const QString file = storeDirectory().filePath(fileName(logical_name, secret_password));
        if (!QFileInfo::exists(file)) {
            return std::nullopt;
        }
        auto &storage = openStore(file);
        const auto id = storage.root(root.toStdString());
        if (!id) {
            return std::nullopt;
        }
        const auto record = storage.get(*id);
        if (!record) {
            return std::nullopt;
        }
        return QString::fromUtf8(record->data(), qsizetype(record->size()));
    } catch (const std::exception &error) {
        openStores.clear();
        qWarning() << "Could not read message cache page:" << error.what();
        return std::nullopt;
    }
}

bool MessageCache::clear(const QString &logical_name, const QString &secret_password)
{
    std::lock_guard lock(storeMutex);
    try {
        const QString file = storeDirectory().filePath(fileName(logical_name, secret_password));
        if (!QFileInfo::exists(file)) {
            return true;
        }
        auto &storage = openStore(file);
        for (const auto id : storage.ids(kMessageTag)) {
            if (!storage.remove(id)) {
                return false;
            }
        }
        storage.sync();
        return true;
    } catch (const std::exception &error) {
        openStores.clear();
        qWarning() << "Could not clear message cache:" << error.what();
        return false;
    }
}

void MessageCache::closeAll()
{
    std::lock_guard lock(storeMutex);
    openStores.clear();
}

bool MessageCache::update(const std::function<bool (QJsonObject &)> &modify)
{
    return true;
}


} // namespace storage
} // namespace core
