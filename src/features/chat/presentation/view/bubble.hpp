#ifndef FEATURES_CHAT_PRESENTATION_VIEW_CHAT_BUBBLE_HPP
#define FEATURES_CHAT_PRESENTATION_VIEW_CHAT_BUBBLE_HPP

#include <QQuickPaintedItem>
#include <QColor>
#include <QPainter>
#include <QPainterPath>
#include <QtQml/qqmlregistration.h>

class Bubble : public QQuickPaintedItem
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QColor color
                   READ color
                       WRITE setColor
                           NOTIFY colorChanged)

    Q_PROPERTY(bool outgoing
                   READ outgoing
                       WRITE setOutgoing
                           NOTIFY outgoingChanged)

    Q_PROPERTY(qreal radius
                   READ radius
                       WRITE setRadius
                           NOTIFY radiusChanged)

    Q_PROPERTY(qreal tailWidth
                   READ tailWidth
                       WRITE setTailWidth
                           NOTIFY tailWidthChanged)

    Q_PROPERTY(qreal tailHeight
                   READ tailHeight
                       WRITE setTailHeight
                           NOTIFY tailHeightChanged)

    Q_PROPERTY(bool showTail
                   READ showTail
                       WRITE setShowTail
                           NOTIFY showTailChanged)

public:
    explicit Bubble(QQuickItem *parent = nullptr);

    void paint(QPainter *painter) override;

    QColor color() const;
    void setColor(const QColor &color);

    bool outgoing() const;
    void setOutgoing(bool outgoing);

    qreal radius() const;
    void setRadius(qreal radius);

    qreal tailWidth() const;
    void setTailWidth(qreal width);

    qreal tailHeight() const;
    void setTailHeight(qreal height);

    bool showTail() const;
    void setShowTail(bool show);


signals:
    void colorChanged();
    void outgoingChanged();
    void radiusChanged();
    void tailWidthChanged();
    void tailHeightChanged();
    void showTailChanged();

private:
    QPainterPath createOutgoingPath(const QRectF &rect) const;
    QPainterPath createIncomingPath(const QRectF &rect) const;

private:
    QColor m_color{ "#2F80ED" };

    bool m_outgoing{true};
    bool m_showTail = true;

    qreal m_radius{16.0};
    qreal m_tailWidth{12.0};
    qreal m_tailHeight{12.0};
};

#endif // FEATURES_CHAT_PRESENTATION_VIEW_CHAT_BUBBLE_HPP
