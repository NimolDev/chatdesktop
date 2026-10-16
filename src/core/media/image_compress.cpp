#include "image_compress.hpp"

#include "logging/logger.hpp"
#include "storage/media_cache.hpp"

#include <QImage>
#include <QImageReader>
#include <QImageWriter>
#include <QFileInfo>
#include <QDir>
#include <QPainter>
#include <QSaveFile>
#include <QUrl>

QFuture<QString> core::media::ImageCompress::compressImageToCache(
    const QString &input, const QString &messageId,
    int maxWidth, int maxHeight, int quality)
{
    if (messageId.isEmpty() || messageId == QStringLiteral(".")
        || messageId == QStringLiteral("..") || messageId.contains('/')
        || messageId.contains('\\')) {
        return QtFuture::makeReadyValueFuture(QString());
    }
    const QString output = core::storage::MediaCache::imagePath(messageId);
    return compressImageToJpg(input, output, maxWidth, maxHeight, quality)
        .then([output](bool success) { return success ? output : QString(); });
}

QFuture<bool> core::media::ImageCompress::compressImageToJpg(
    const QString &input,
    const QString &output,
    int maxWidth,
    int maxHeight,
    int quality)
{
    if (maxWidth <= 0 || maxHeight <= 0) {
        return QtFuture::makeReadyValueFuture(false);
    }
    const QUrl inputUrl(input);
    QImageReader reader(inputUrl.isLocalFile() ? inputUrl.toLocalFile() : input);
    reader.setAutoTransform (true); // Handle EXIF orientation

    if(!reader.canRead ()) {
        LOG_WARNING(QStringLiteral ("Cannot read image: %1").arg (reader.errorString ()));
        return QtFuture::makeReadyValueFuture (false);
    }
    // Decode at a smaller size when possible to reduce memory usage.
    const QSize original_size = reader.size ();

    if (original_size.isValid ()) {
        const QSize target_size = original_size.scaled (
            maxWidth,
            maxHeight,
            Qt::KeepAspectRatio
            );
        if (target_size.width () < original_size.width ()
            || target_size.height () < original_size.height ()) {
            reader.setScaledSize (target_size);
        }
    }

    QImage image = reader.read ();

    if (image.isNull ()) {
        LOG_WARNING (QStringLiteral ("Failed to load image: %1").arg (reader.errorString ()));
        return QtFuture::makeReadyValueFuture (false);
    }

    if (image.width() > maxWidth || image.height() > maxHeight) {
        image = image.scaled(maxWidth, maxHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    // jpeg does't not support alpha.
    // Flatten transparent image onto a white background.
    if (image.hasAlphaChannel ()) {
        QImage flattened(image.size (), QImage::Format_RGB888);
        flattened.fill (Qt::white);

        QPainter painter(&flattened);
        painter.drawImage (0,0, image);
        painter.end ();

        image = flattened;
    }

    QSaveFile destination(output);
    if (!destination.open(QIODevice::WriteOnly)) {
        LOG_WARNING(QStringLiteral("Cannot open image cache: %1").arg(destination.errorString()));
        return QtFuture::makeReadyValueFuture(false);
    }
    QImageWriter writer(&destination, "jpg");
    writer.setQuality (qBound (0, quality, 100));
    writer.setOptimizedWrite (true);

    if (!writer.write (image)) {
        LOG_WARNING (QStringLiteral ("Failed to save JPEG: %1").arg (writer.errorString ()));
        return QtFuture::makeReadyValueFuture (false);
    }
    return QtFuture::makeReadyValueFuture(destination.commit());
}
