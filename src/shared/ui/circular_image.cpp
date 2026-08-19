#include "circular_image.hpp"

#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPainter>
#include <QPainterPath>

CircularImage::CircularImage(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
    setAntialiasing(true);
    setMipmap(true);

    // Important for Retina / high-DPI rendering
    setRenderTarget(QQuickPaintedItem::FramebufferObject);
}

void CircularImage::paint(QPainter *painter)
{
    if (m_image.isNull())
        return;

    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setRenderHint(QPainter::SmoothPixmapTransform, true);

    QRectF rect = boundingRect();

    if (m_borderWidth > 0.0) {
        const qreal halfBorder = m_borderWidth / 2.0;

        rect.adjust(
            halfBorder,
            halfBorder,
            -halfBorder,
            -halfBorder
            );
    }

    QPainterPath circlePath;
    circlePath.addEllipse(rect);

    painter->save();

    painter->setClipPath(circlePath);

    /*
     * Equivalent to Image.PreserveAspectCrop.
     */

    const QSizeF sourceSize = m_image.size();
    const QSizeF targetSize = rect.size();

    const qreal sourceRatio =
        sourceSize.width() / sourceSize.height();

    const qreal targetRatio =
        targetSize.width() / targetSize.height();

    QRectF sourceRect;

    if (sourceRatio > targetRatio) {
        // Image is wider -> crop left/right

        const qreal newWidth =
            sourceSize.height() * targetRatio;

        const qreal x =
            (sourceSize.width() - newWidth) / 2.0;

        sourceRect = QRectF(
            x,
            0,
            newWidth,
            sourceSize.height()
            );
    } else {
        // Image is taller -> crop top/bottom

        const qreal newHeight =
            sourceSize.width() / targetRatio;

        const qreal y =
            (sourceSize.height() - newHeight) / 2.0;

        sourceRect = QRectF(
            0,
            y,
            sourceSize.width(),
            newHeight
            );
    }

    painter->drawImage(
        rect,
        m_image,
        sourceRect
        );

    painter->restore();

    // Draw border AFTER clipping
    if (m_borderWidth > 0.0) {
        QPen pen(m_borderColor);
        pen.setWidthF(m_borderWidth);

        painter->setPen(pen);
        painter->setBrush(Qt::NoBrush);

        painter->drawEllipse(rect);
    }
}

QUrl CircularImage::source() const
{
    return m_source;
}

void CircularImage::setSource(const QUrl &source)
{
    if (m_source == source)
        return;

    m_source = source;

    loadImage();

    emit sourceChanged();

    update();
}

void CircularImage::loadImage()
{
    cancelRemoteLoad();
    m_image = QImage();
    setErrorString({});
    setLoading(false);

    if (m_source.isEmpty()) {
        update();
        return;
    }

    const QString scheme = m_source.scheme().toLower();
    if (scheme == QStringLiteral("http") || scheme == QStringLiteral("https")) {
        loadRemoteImage();
        return;
    }

    QString path;

    if (m_source.scheme() == "qrc") {
        path = ":" + m_source.path();
    } else if (m_source.isLocalFile()) {
        path = m_source.toLocalFile();
    } else {
        path = m_source.toString();
    }

    if (!m_image.load(path))
        setErrorString(tr("Unable to load image from %1").arg(m_source.toString()));

    update();
}

void CircularImage::loadRemoteImage()
{
    QNetworkRequest request(m_source);
    request.setAttribute(
        QNetworkRequest::RedirectPolicyAttribute,
        QNetworkRequest::NoLessSafeRedirectPolicy);

    QNetworkReply *reply = m_networkManager.get(request);
    m_reply = reply;
    setLoading(true);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        // A changed source may leave an older, aborted reply queued for delivery.
        if (m_reply != reply) {
            reply->deleteLater();
            return;
        }

        m_reply = nullptr;
        setLoading(false);

        if (reply->error() != QNetworkReply::NoError) {
            setErrorString(reply->errorString());
        } else {
            QImage downloadedImage;
            const QByteArray data = reply->readAll();
            if (downloadedImage.loadFromData(data)) {
                m_image = std::move(downloadedImage);
                setErrorString({});
            } else {
                setErrorString(tr("The downloaded data is not a supported image"));
            }
        }

        reply->deleteLater();
        update();
    });
}

void CircularImage::cancelRemoteLoad()
{
    if (!m_reply)
        return;

    QNetworkReply *reply = m_reply;
    m_reply = nullptr;
    reply->abort();
    reply->deleteLater();
}

void CircularImage::setLoading(bool loading)
{
    if (m_loading == loading)
        return;

    m_loading = loading;
    emit loadingChanged();
}

void CircularImage::setErrorString(const QString &errorString)
{
    if (m_errorString == errorString)
        return;

    m_errorString = errorString;
    emit errorStringChanged();
}

QColor CircularImage::borderColor() const
{
    return m_borderColor;
}

void CircularImage::setBorderColor(const QColor &color)
{
    if (m_borderColor == color)
        return;

    m_borderColor = color;

    emit borderColorChanged();

    update();
}

qreal CircularImage::borderWidth() const
{
    return m_borderWidth;
}

void CircularImage::setBorderWidth(qreal width)
{
    if (qFuzzyCompare(m_borderWidth, width))
        return;

    m_borderWidth = width;

    emit borderWidthChanged();

    update();
}

bool CircularImage::isLoading() const
{
    return m_loading;
}

QString CircularImage::errorString() const
{
    return m_errorString;
}
