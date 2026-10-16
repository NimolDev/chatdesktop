#include "auth_storage.hpp"
#include "local_storage.hpp"
#include "file_name_helper.hpp"
#include "app_constants.hpp"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QSaveFile>
#include <QStandardPaths>
#include <QTemporaryFile>

#include <algorithm>
#include <mutex>
#include <stdexcept>

namespace core {
namespace storage {
namespace {

constexpr std::uint32_t kUserTag = 1;
std::mutex storeMutex;

QDir storeDirectory()
{
    const QDir appData(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    return QDir(appData.filePath(QStringLiteral("user_data")));
}

QString indexPath()
{
    return storeDirectory().filePath(FileNameHelper::hexName(QStringLiteral("active_auth")));
}

QString accountFileName(const QJsonObject &object)
{
    const auto user = object.value(QStringLiteral("user")).toObject();
    const QString username = user.value(QStringLiteral("username")).toString();
    const QString userId = user.value(QStringLiteral("id")).toString();
    if (username.isEmpty() || userId.isEmpty()) {
        throw std::runtime_error("Missing auth user identity");
    }
    const QString secret = QString::fromLatin1((userId + username).toUtf8().toBase64());
    return FileNameHelper::hexName(username, secret);
}

QStringList legacyPaths()
{
    const auto directory = storeDirectory();
    return {directory.filePath(FileNameHelper::hexName(QStringLiteral("auth"))),
            directory.filePath(QStringLiteral("auth.store")),
            directory.filePath(QStringLiteral("cache"))};
}

std::optional<QString> readActiveName()
{
    QFile index(indexPath());
    if (!index.exists()) {
        return std::nullopt;
    }
    if (!index.open(QIODevice::ReadOnly)) {
        throw std::runtime_error("Cannot read active auth index");
    }
    const QByteArray name = index.read(33);
    // An empty index is a logout marker. Never restore an old session in that case.
    if (!name.isEmpty() && (name.size() != 32 ||
        !std::all_of(name.begin(), name.end(), [](char c) {
            return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
        }))) {
        throw std::runtime_error("Invalid active auth filename");
    }
    return QString::fromLatin1(name);
}

bool writeActiveName(const QString &name)
{
    if (!QDir().mkpath(storeDirectory().absolutePath())) {
        return false;
    }
    QSaveFile index(indexPath());
    const QByteArray bytes = name.toLatin1();
    return index.open(QIODevice::WriteOnly) &&
           index.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner) &&
           index.write(bytes) == bytes.size() && index.commit();
}

std::string storePassword()
{
    return core::constants::AppConstants::appSecretKey().toStdString();
}

std::optional<QJsonObject> readSnapshot(const QString &path)
{
    if (!QFileInfo::exists(path)) {
        return std::nullopt;
    }
    core::storage::LocalStorage storage(path.toStdString(), storePassword());
    const auto ids = storage.ids(kUserTag);
    if (ids.size() != 1) {
        throw std::runtime_error("Invalid auth record count");
    }
    const auto value = storage.get(ids.front());
    if (!value) {
        throw std::runtime_error("Missing auth record");
    }
    QJsonParseError error;
    const auto document = QJsonDocument::fromJson(QByteArray::fromStdString(*value), &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) {
        throw std::runtime_error("Invalid auth JSON");
    }
    return document.object();
}

bool writeSnapshot(const QString &path, const QJsonObject &object)
{
    if (!QDir().mkpath(QFileInfo(path).absolutePath())) {
        return false;
    }

    // Build a complete encrypted snapshot before replacing the previous login.
    // The temporary file contains only LocalStorage ciphertext, never plaintext JSON.
    QTemporaryFile temporary(path + QStringLiteral(".XXXXXX"));
    if (!temporary.open()) {
        return false;
    }
    temporary.close();
    {
        core::storage::LocalStorage storage(
            temporary.fileName().toStdString(), storePassword(), {4096, true});
        storage.insert(QJsonDocument(object).toJson(QJsonDocument::Compact).toStdString(), kUserTag);
        storage.sync();
    }
    QFile source(temporary.fileName());
    QSaveFile destination(path);
    if (!source.open(QIODevice::ReadOnly) || !destination.open(QIODevice::WriteOnly)) {
        return false;
    }
    if (!destination.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner)) {
        return false;
    }
    while (!source.atEnd()) {
        const QByteArray block = source.read(64 * 1024);
        if (source.error() != QFileDevice::NoError ||
            destination.write(block) != block.size()) {
            return false;
        }
    }
    return destination.commit();
}

bool writeSession(const QJsonObject &object)
{
    const QString name = accountFileName(object);
    const QString path = storeDirectory().filePath(name);
    std::optional<QString> previous;
    try {
        previous = readActiveName();
    } catch (const std::exception &) {
        // A successful login can replace a corrupt lookup index.
    }
    if (!writeSnapshot(path, object)) {
        return false;
    }
    // Same-account refresh needs no index change. The snapshot commit is atomic.
    if ((!previous || *previous != name) && !writeActiveName(name)) {
        QFile::remove(path);
        return false;
    }
    if (previous && !previous->isEmpty() && *previous != name) {
        QFile::remove(storeDirectory().filePath(*previous));
    }
    for (const auto &legacy : legacyPaths()) {
        if (legacy != path) {
            QFile::remove(legacy);
        }
    }
    return true;
}

std::optional<QJsonObject> readSession()
{
    const auto name = readActiveName();
    if (name) {
        if (name->isEmpty()) {
            return std::nullopt;
        }
        const auto object = readSnapshot(storeDirectory().filePath(*name));
        if (object && accountFileName(*object) != *name) {
            throw std::runtime_error("Auth filename does not match user identity");
        }
        return object;
    }
    // Upgrade earlier fixed-name snapshots once, before an index exists.
    for (const auto &legacy : legacyPaths()) {
        const auto object = readSnapshot(legacy);
        if (object) {
            if (!writeSession(*object)) {
                throw std::runtime_error("Cannot migrate auth filename");
            }
            return object;
        }
    }
    return std::nullopt;
}

} // namespace

bool AuthStorage::save(const QJsonObject &object)
{
    std::lock_guard lock(storeMutex);
    try {
        return writeSession(object);
    } catch (const std::exception &error) {
        qWarning() << "Saving auth session failed:" << error.what();
        return false;
    }
}

std::optional<QJsonObject> AuthStorage::read()
{
    std::lock_guard lock(storeMutex);
    try {
        return readSession();
    } catch (const std::exception &error) {
        qWarning() << "Reading auth session failed:" << error.what();
        return std::nullopt;
    }
}

bool AuthStorage::clear()
{
    std::lock_guard lock(storeMutex);
    try {
        const auto name = readActiveName();
        if (!writeActiveName(QString{})) {
            return false;
        }
        // Commit logout first; stale snapshots must never restore a session.
        if (name && !name->isEmpty()) {
            QFile::remove(storeDirectory().filePath(*name));
        }
        for (const auto &legacy : legacyPaths()) {
            QFile::remove(legacy);
        }
        return true;
    } catch (const std::exception &error) {
        qWarning() << "Clearing auth session failed:" << error.what();
        return false;
    }
}

bool AuthStorage::update(const std::function<bool(QJsonObject &)> &modify)
{
    std::lock_guard lock(storeMutex);
    try {
        auto object = readSession();
        if (!object || !modify(*object)) {
            return false;
        }
        return writeSession(*object);
    } catch (const std::exception &error) {
        qWarning() << "Updating auth session failed:" << error.what();
        return false;
    }
}

} // namespace storage
} // namespace core
