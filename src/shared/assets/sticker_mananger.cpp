#include "sticker_mananger.hpp"

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>

StickerMananger::StickerMananger(QObject *parent)
    : QObject{parent}
{}

QStringList StickerMananger::getStickers() const
{
    QStringList stickers;
    QDirIterator it(":/stickers", {"*.png"}, QDir::Files, QDirIterator::Subdirectories);
    while(it.hasNext ()) {
        const QString path = it.next ();
        stickers.append ("qrc" + path);
    }
    stickers.sort ();
    return stickers;

}
