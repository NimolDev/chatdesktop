#pragma once

#include <QFuture>
#include <QString>

namespace core {
namespace media {

class ImageCompress
{
public:
    static QFuture<bool> compressImageToJpg(
        const QString &input, const QString &output,
        int maxWidth = 1920, int maxHeight = 1920, int quality = 85);

    // Returns the cached local path, or an empty string if compression fails.
    static QFuture<QString> compressImageToCache(
        const QString &input, const QString &messageId,
        int maxWidth = 1920, int maxHeight = 1920, int quality = 85);
};

} // namespace media
} // namespace core
