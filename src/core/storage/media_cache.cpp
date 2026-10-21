#include "media_cache.hpp"
#include "local_storage.hpp"
#include "app_constants.hpp"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTemporaryFile>
#include <QCache>
#include <QDateTime>
#include <mutex>
#include <cstdio>
#include <algorithm>

namespace core {
namespace storage {
namespace {
std::mutex cacheMutex;
constexpr std::uint32_t mediaTag = 3;
struct CachedImage {
    QDateTime modified;
    qint64 size;
    QUrl url;
};
// Avoid decrypting and base64-encoding the same image on every model-role read.
QCache<QString, CachedImage> imageSources(32 * 1024);
}

QString MediaCache::basePath()
{
    return QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation))
        .filePath(QStringLiteral("user_data/media_cache"));
}

QString MediaCache::pathFor(const QString &kind, const QString &id, const QString &extension, const QString &account)
{
    const auto identity = account.toLower().remove(QLatin1Char('-')).toUtf8() + '\0'
                          + kind.toUtf8() + '\0' + id.toUtf8() + '\0' + extension.toUtf8();
    const auto name = QString::fromLatin1(QCryptographicHash::hash(identity, QCryptographicHash::Sha256).toHex());
    const auto path = QDir(basePath()).filePath(name);
    const auto legacyPath = path + QStringLiteral(".media");
    std::lock_guard lock(cacheMutex);
    if (!QFileInfo::exists(path) && !QFileInfo(path).isSymLink()
        && QFileInfo(legacyPath).isFile() && !QFileInfo(legacyPath).isSymLink()) {
        if (!QFile::rename(legacyPath, path)) {
            // Keep existing media readable if renaming fails.
            return legacyPath;
        }
        imageSources.remove(legacyPath);
    }
    return path;
}

QString MediaCache::voicePath(const QString &id, const QString &extension, const QString &account)
{
    return pathFor(QStringLiteral("voice"), id, extension, account);
}

QString MediaCache::imagePath(const QString &id, const QString &extension, const QString &account)
{
    return pathFor(QStringLiteral("image"), id, extension, account);
}

QString MediaCache::filePath(const QString &id, const QString &extension, const QString &account)
{
    return pathFor(QStringLiteral("file"), id, extension, account);
}

bool MediaCache::saveLocalFilePath(const QUrl &url, const QString &localPath, const QString &account)
{
    if (account.isEmpty() || url.isEmpty() || !url.isValid() || !QFileInfo(localPath).isFile()) {
        return false;
    }
    const auto path = pathFor(QStringLiteral("file-location"), url.toString(QUrl::FullyEncoded), {}, account);
    return save(path, QFileInfo(localPath).absoluteFilePath().toUtf8());
}

QString MediaCache::localFilePath(const QUrl &url, const QString &account)
{
    if (account.isEmpty() || url.isEmpty() || !url.isValid()) {
        return {};
    }
    const auto path = pathFor(QStringLiteral("file-location"), url.toString(QUrl::FullyEncoded), {}, account);
    const auto localPath = QString::fromUtf8(read(path));
    return QFileInfo(localPath).isAbsolute() && QFileInfo(localPath).isFile() ? localPath : QString{};
}

bool MediaCache::isCachePath(const QString &path)
{
    const QFileInfo info(path);
    QString name = info.fileName();
    // Accept the previous format for references held by an active transfer.
    if (name.endsWith(QStringLiteral(".media"))) {
        name.chop(6);
    }
    const bool hashedName = name.size() == 64 && std::all_of(name.cbegin(), name.cend(), [](QChar c) {
        return (c >= QLatin1Char('0') && c <= QLatin1Char('9'))
               || (c >= QLatin1Char('a') && c <= QLatin1Char('f'));
    });
    return info.absolutePath() == QDir(basePath()).absolutePath()
           && hashedName && !info.isSymLink();
}

bool MediaCache::save(const QString &path, const QByteArray &bytes)
{
    if (bytes.isEmpty() || !isCachePath(path)) {
        return false;
    }
    std::lock_guard lock(cacheMutex);
    imageSources.remove(path);
    if (!QDir().mkpath(basePath())) {
        return false;
    }
    // Build an encrypted store beside the destination, then atomically replace it.
    QTemporaryFile temporary(QDir(basePath()).filePath(QStringLiteral(".media-XXXXXX")));
    if (!temporary.open()) {
        return false;
    }
    const auto temporaryPath = temporary.fileName();
    temporary.close();
    try {
        {
            LocalStorage store(temporaryPath.toStdString(), constants::AppConstants::appSecretKey().toStdString());
            store.insert(std::string(bytes.constData(), size_t(bytes.size())), mediaTag);
            store.sync();
        }
        // POSIX rename replaces atomically; the existing cache survives write failures.
        return ::rename(QFile::encodeName(temporaryPath).constData(), QFile::encodeName(path).constData()) == 0;
    } catch (...) {
        return false;
    }
}

QByteArray MediaCache::read(const QString &path)
{
    if (!isCachePath(path)) {
        return {};
    }
    std::lock_guard lock(cacheMutex);
    if (!QFileInfo(path).isFile()) {
        return {};
    }
    try {
        LocalStorage store(path.toStdString(), constants::AppConstants::appSecretKey().toStdString());
        const auto records = store.ids(mediaTag);
        if (!records.empty()) {
            if (auto bytes = store.get(records.back())) {
                return QByteArray(bytes->data(), qsizetype(bytes->size()));
            }
        }
    } catch (...) {
        // Missing, corrupted, or unreadable cache entries are cache misses.
    }
    return {};
}

bool MediaCache::saveFile(const QString &path, const QString &source)
{
    QFile file(source);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    const auto bytes = file.readAll();
    return file.error() == QFile::NoError && save(path, bytes);
}

QUrl MediaCache::imageUrl(const QString &path)
{
    const QFileInfo info(path);
    {
        std::lock_guard lock(cacheMutex);
        if (const auto *cached = imageSources.object(path)) {
            if (info.isFile() && !info.isSymLink() && cached->modified == info.lastModified()
                && cached->size == info.size()) {
                return cached->url;
            }
            imageSources.remove(path);
        }
    }
    const auto bytes = read(path);
    if (bytes.isEmpty()) {
        return {};
    }
    const QUrl url(QStringLiteral("data:image/jpeg;base64,") + QString::fromLatin1(bytes.toBase64()));
    const auto cost = (bytes.size() * 3 + 1023) / 1024;
    if (cost <= imageSources.maxCost()) {
        std::lock_guard lock(cacheMutex);
        imageSources.insert(path, new CachedImage{info.lastModified(), info.size(), url}, int(cost));
    }
    return url;
}

bool MediaCache::migrateLegacyImage(const QString &id, const QString &account)
{
    if (id.isEmpty() || id.contains('/') || id.contains('\\')) {
        return false;
    }
    const auto legacy = QDir(QStandardPaths::writableLocation(QStandardPaths::CacheLocation))
                            .filePath(QStringLiteral("media/images/%1.jpg").arg(id));
    if (!QFileInfo(legacy).isFile() || QFileInfo(legacy).isSymLink()) {
        return false;
    }
    if (!saveFile(imagePath(id, QStringLiteral("jpg"), account), legacy)) {
        return false;
    }
    return QFile::remove(legacy);
}

bool MediaCache::exists(const QString &path)
{
    return !read(path).isEmpty();
}

bool MediaCache::remove(const QString &path)
{
    std::lock_guard lock(cacheMutex);
    imageSources.remove(path);
    return isCachePath(path) && (!QFileInfo::exists(path) || QFile::remove(path));
}

} // namespace storage
} // namespace core
