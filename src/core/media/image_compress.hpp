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

    // Returns an encrypted MediaCache path; no plaintext JPEG is written to disk.
    static QFuture<QString> compressImageToCache(
        const QString &input, const QString &messageId,
         const QString &account = {}, int maxWidth = 1920, int maxHeight = 1920, int quality = 85);
};

} // namespace media
} // namespace core
