#pragma once

#include <QObject>
#include <QUrl>

namespace domain {
namespace repository  {

class MediaRepository : public QObject
{
    Q_OBJECT

public:
    explicit  MediaRepository(QObject *parent = nullptr) : QObject(parent) {};
    ~MediaRepository() override = default;

    virtual void downloadFile(
        const QString &message_id,
        const QUrl &url) = 0;
    virtual void cancelDownload(const QString &message_id) = 0;
signals:
    void downloadStarted(const QString &messageId);

    void downloadProgress(
        const QString &messageId,
        qint64 received,
        qint64 total
        );

    void downloadCompleted(
        const QString &messageId,
        const QString &localPath
        );

    void downloadFailed(
        const QString &messageId,
        const QString &error
        );
};

} // namespace repository
} // namespace domain
