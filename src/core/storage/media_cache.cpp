#include "media_cache.hpp"

#include <QDir>
#include <QFile>
#include <QStandardPaths>

QString core::storage::MediaCache::voicePath(
    const QString &message_id,
    const QString &extension)
{
    const QString directory =  basePath() + QStringLiteral("/media/voice");
    QDir().mkpath(directory);
    return QStringLiteral("%1/%2.%3")
        .arg(
            directory,
            message_id,
            extension
            );
}

QString core::storage::MediaCache::imagePath(
    const QString &message_id,
    const QString &extension)
{
    const QString directory = basePath() + QStringLiteral("/media/images");
    QDir().mkpath(directory);
    return QStringLiteral("%1/%2.%3")
        .arg(
            directory,
            message_id,
            extension
            );
}

bool core::storage::MediaCache::exists(const QString &path)
{
    return QFile::exists(path);
}

bool core::storage::MediaCache::remove(const QString &path)
{
    if (!QFile::exists(path)) {
        return true;
    }
    return QFile::remove(path);
}

QString core::storage::MediaCache::basePath()
{
    return QStandardPaths::writableLocation (QStandardPaths::CacheLocation);
}

QString core::storage::MediaCache::filePath(const QString &message_id, const QString &extension)
{
    const QString directory = basePath() + QStringLiteral("/media/files");
    QDir().mkpath(directory);
    return QStringLiteral("%1/%2.%3").arg(directory, message_id, extension);
}
