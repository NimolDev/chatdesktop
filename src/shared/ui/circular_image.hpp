#ifndef CIRCULAR_IMAGE_H
#define CIRCULAR_IMAGE_H


#include <QColor>
#include <QImage>
#include <QNetworkAccessManager>
#include <QPointer>
#include <QQuickPaintedItem>
#include <QUrl>

#include <QtQml/qqmlregistration.h>

class QNetworkReply;

class CircularImage : public QQuickPaintedItem
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QUrl source
                   READ source
                       WRITE setSource
                           NOTIFY sourceChanged)

    Q_PROPERTY(QColor borderColor
                   READ borderColor
                       WRITE setBorderColor
                           NOTIFY borderColorChanged)

    Q_PROPERTY(qreal borderWidth
                   READ borderWidth
                       WRITE setBorderWidth
                           NOTIFY borderWidthChanged)

    Q_PROPERTY(bool loading READ isLoading NOTIFY loadingChanged)
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorStringChanged)

public:
    explicit CircularImage(QQuickItem *parent = nullptr);

    void paint(QPainter *painter) override;

    QUrl source() const;
    void setSource(const QUrl &source);

    QColor borderColor() const;
    void setBorderColor(const QColor &color);

    qreal borderWidth() const;
    void setBorderWidth(qreal width);

    bool isLoading() const;
    QString errorString() const;

signals:
    void sourceChanged();
    void borderColorChanged();
    void borderWidthChanged();
    void loadingChanged();
    void errorStringChanged();

private:
    void loadImage();
    void loadRemoteImage();
    void cancelRemoteLoad();
    void setLoading(bool loading);
    void setErrorString(const QString &errorString);

private:
    QUrl m_source;
    QImage m_image;
    QNetworkAccessManager m_networkManager;
    QPointer<QNetworkReply> m_reply;
    bool m_loading = false;
    QString m_errorString;

    QColor m_borderColor = Qt::transparent;
    qreal m_borderWidth = 0.0;
};

#endif // CIRCULAR_IMAGE_H
