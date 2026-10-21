#pragma once

#include <QByteArray>
#include <QString>
#include <QUrl>

namespace core {
namespace storage {

// Persistent media is encrypted using the same LocalStorage format as user data.
class MediaCache
{
public:
    static QString voicePath(const QString &id, const QString &extension, const QString &account = {});
    static QString imagePath(const QString &id, const QString &extension = QStringLiteral("jpg"), const QString &account = {});
    static QString filePath(const QString &id, const QString &extension, const QString &account = {});
    // Persist only the local filename, keyed by account and remote URL.
    static bool saveLocalFilePath(const QUrl &url, const QString &localPath, const QString &account);
    static QString localFilePath(const QUrl &url, const QString &account);
    static bool save(const QString &path, const QByteArray &bytes);
    static QByteArray read(const QString &path);
    static bool saveFile(const QString &path, const QString &source);
    static QUrl imageUrl(const QString &path);
    static bool migrateLegacyImage(const QString &id, const QString &account);
    static bool isCachePath(const QString &path);
    static bool exists(const QString &path);
    static bool remove(const QString &path);

private:
    static QString basePath();
    static QString pathFor(const QString &kind, const QString &id, const QString &extension, const QString &account);
};

} // namespace storage
} // namespace core
