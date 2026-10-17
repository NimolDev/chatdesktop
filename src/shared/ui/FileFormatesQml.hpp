#pragma once

#include "file_formates.hpp"

#include <QObject>
#include <qqmlintegration.h>
#include <QUrl>

class FileFormatesQml : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QStringList images READ images CONSTANT)
    Q_PROPERTY(QStringList videos READ videos CONSTANT)
    Q_PROPERTY(QString imageFilters READ imageFilters CONSTANT )
    Q_PROPERTY(QString imageAndVideo READ imageAndVideo CONSTANT)

public:
    explicit FileFormatesQml(QObject *parent = nullptr) : QObject(parent) {}

    QStringList images() { return FileFormates::kImages; }
    QStringList videos() { return FileFormates::kVideos; }

    QString imageFilters()
    {
        return "Image files " + FileFormates::filter (images ());
    }
    QString imageAndVideo() const
    {
        const QStringList extensions =
            FileFormates::kImages + FileFormates::kVideos;

        QStringList patterns;
        patterns.reserve(extensions.size());

        for (const QString &ext : extensions) {
            patterns.append("*." + ext);
        }
        return QStringLiteral("Image and Video Files (%1)")
            .arg(patterns.join(' '));
    }

    Q_INVOKABLE bool isImage(const QUrl &url) const
    {
        // return url.isLoca
        return url.isLocalFile()
               && FileFormates::matches (url.toLocalFile(), FileFormates::kImages);
    }

    Q_INVOKABLE bool isAllImage(const QStringList &urls)
    {
        return FileFormates::matches (urls,FileFormates::kImages);
    }
};
