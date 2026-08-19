#include "bubble.hpp"

#include <algorithm>

Bubble::Bubble(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
    setAntialiasing(true);
}

void Bubble::paint(QPainter *painter)
{
    if (!painter)
        return;

    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setRenderHint(QPainter::SmoothPixmapTransform, true);

    painter->setPen(Qt::NoPen);
    painter->setBrush(m_color);

    QRectF bubbleRect = boundingRect();

    if (bubbleRect.width() <= 0 || bubbleRect.height() <= 0)
        return;

    QPainterPath path;

    if (m_outgoing)
        path = createOutgoingPath(bubbleRect);
    else
        path = createIncomingPath(bubbleRect);

    painter->drawPath(path);
}
QPainterPath Bubble::createOutgoingPath(const QRectF &rect) const
{
    const qreal x = rect.left();
    const qreal y = rect.top();
    const qreal w = rect.width();
    const qreal h = rect.height();

    if (w <= 0 || h <= 0)
        return {};

    const qreal tailW = std::min(m_tailWidth, w * 0.25);

    // Tail occupies the right-most area.
    const qreal bubbleRight =  m_showTail ? x + w - tailW : x + w - tailW;
    const qreal bottom = y + h;

    const qreal r = std::min({
        m_radius,
        h / 2.0,
        (w - tailW) / 2.0
    });

    QPainterPath path;

    // ─────────────────────────────
    // Top-left
    // ─────────────────────────────

    path.moveTo(
        x + r,
        y
        );

    // Top
    path.lineTo(
        bubbleRight - r,
        y
        );

    // Top-right rounded corner
    path.quadTo(
        bubbleRight, y,
        bubbleRight, y + r
        );

    // Right edge
    //
    // Stop ABOVE bottom.
    //
    const qreal tailStartY =
        bottom - m_tailHeight;

    if (m_showTail) {

    path.lineTo(
        bubbleRight,
        tailStartY
        );

    path.cubicTo(
        // control point 1
        bubbleRight,
        bottom - m_tailHeight * 0.35,

        // control point 2
        bubbleRight + tailW * 0.30,
        bottom,

        // tail tip
        x + w,
        bottom
        );
    } else {
        path.lineTo (bubbleRight, bottom- r);

        path.quadTo (bubbleRight, bottom, bubbleRight - r, bottom);
    }
    // ─────────────────────────────
    // Bottom
    //
    // Tail tip → bubble left.
    // Same Y coordinate.
    // ─────────────────────────────

    path.lineTo(
        x + r,
        bottom
        );

    // Bottom-left corner
    path.quadTo(
        x,
        bottom,

        x,
        bottom - r
        );

    // Left
    path.lineTo(
        x,
        y + r
        );

    // Top-left
    path.quadTo(
        x,
        y,

        x + r,
        y
        );

    path.closeSubpath();

    return path;
}

QPainterPath Bubble::createIncomingPath(const QRectF &rect) const
{
    QPainterPath path = createOutgoingPath(rect);

    QTransform transform;

    transform.translate(
        rect.left() + rect.right(),
        0
        );

    transform.scale(-1.0, 1.0);

    return transform.map(path);
}

QColor Bubble::color() const
{
    return m_color;
}

void Bubble::setColor(const QColor &color)
{
    if (m_color == color)
        return;

    m_color = color;

    emit colorChanged();

    update();
}

bool Bubble::outgoing() const
{
    return m_outgoing;
}

void Bubble::setOutgoing(bool outgoing)
{
    if (m_outgoing == outgoing)
        return;

    m_outgoing = outgoing;

    emit outgoingChanged();

    update();
}

qreal Bubble::radius() const
{
    return m_radius;
}

void Bubble::setRadius(qreal radius)
{
    if (qFuzzyCompare(m_radius, radius))
        return;

    m_radius = radius;

    emit radiusChanged();

    update();
}

qreal Bubble::tailWidth() const
{
    return m_tailWidth;
}

void Bubble::setTailWidth(qreal width)
{
    if (qFuzzyCompare(m_tailWidth, width))
        return;

    m_tailWidth = width;

    emit tailWidthChanged();

    update();
}

qreal Bubble::tailHeight() const
{
    return m_tailHeight;
}

void Bubble::setTailHeight(qreal height)
{
    if (qFuzzyCompare(m_tailHeight, height))
        return;

    m_tailHeight = height;

    emit tailHeightChanged();

    update();
}

bool Bubble::showTail() const
{
    return m_showTail;
}

void Bubble::setShowTail(bool show)
{
    if (m_showTail == show) {
        return;
    }
    m_showTail = show;

    emit showTailChanged();

}