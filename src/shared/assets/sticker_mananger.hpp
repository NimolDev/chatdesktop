#pragma once

#include <QObject>
#include <qqmlregistration.h>

class StickerMananger : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    explicit StickerMananger(QObject *parent = nullptr);

    Q_INVOKABLE QStringList getStickers() const;


signals:
};
