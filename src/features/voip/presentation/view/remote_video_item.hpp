#pragma once

#include <QImage>
#include <QQuickPaintedItem>
#include <QtQml/qqmlregistration.h>

class RemoteVideoItem : public QQuickPaintedItem
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QImage frame READ frame WRITE setFrame NOTIFY frameChanged)
    Q_PROPERTY(qreal radius READ radius WRITE setRadius NOTIFY radiusChanged FINAL)

public:
    explicit RemoteVideoItem(QQuickItem *parent = nullptr);
    QImage frame() const { return m_frame; }
    void setFrame(const QImage &frame);
    void paint(QPainter *painter) override;

    int radius() const { return m_radius; }
    void setRadius(const qreal  &raduis);


signals:
    void frameChanged();
    void radiusChanged();

private:
    QImage m_frame;
    qreal m_radius;
};
