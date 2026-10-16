#pragma once
#include <QString>

namespace core {
namespace storage {

class MediaCache
{
public:
    MediaCache();

    static QString voicePath(
        const QString &message_id,
        const QString &extension);

    static QString imagePath(
        const QString &message_id,
        const QString &extension = QStringLiteral ("jpg"));

    static QString filePath(const QString &message_id, const QString &extension);

    static bool exists(const QString &path);
    static bool remove(const QString &path);

private:
    static QString basePath();
};


} // namespace storage
} // namespace core
