#include "remote_video_item.hpp"


#include <QPainter>
#include <QPainterPath>

RemoteVideoItem::RemoteVideoItem(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
    setFillColor(Qt::transparent);
    setOpaquePainting(false);
}

void RemoteVideoItem::setFrame(const QImage &frame)
{
    m_frame = frame;
    update();

    // Usually unnecessary at 30 FPS.
    // emit frameChanged();
}


void RemoteVideoItem::paint(QPainter *painter)
{
    if (m_frame.isNull()) {
        return;
    }

    const QRectF bounds = boundingRect();

    painter->save();

    if (m_radius > 0.0) {
        painter->setRenderHint(QPainter::Antialiasing, true);

        QPainterPath clipPath;
        clipPath.addRoundedRect(
            bounds,
            m_radius,
            m_radius
            );

        painter->setClipPath(clipPath);
    }

    const QSizeF scaledSize =
        QSizeF(m_frame.size()).scaled(
            bounds.size(),
            Qt::KeepAspectRatio
            );

    const QRectF destination(
        (bounds.width() - scaledSize.width()) / 2.0,
        (bounds.height() - scaledSize.height()) / 2.0,
        scaledSize.width(),
        scaledSize.height()
        );

    painter->setRenderHint(
        QPainter::SmoothPixmapTransform,
        true
        );

    painter->drawImage(
        destination,
        m_frame
        );

    painter->restore();
}

void RemoteVideoItem::setRadius(const qreal &raduis)
{
    m_radius = raduis;
}