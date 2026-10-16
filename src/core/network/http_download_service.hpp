#pragma once

#include <QObject>
#include <QHash>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QFile>
#include <QUrl>

namespace core {
namespace network {


class HttpDownloadService final : public QObject
{
    Q_OBJECT
public:
    explicit HttpDownloadService(QObject *parent = nullptr);
    ~HttpDownloadService() override;

    /**
     * Starts downloading `url` to `destination_path`
     *
     * Data is initially written to:
     *      destination_path.<request_id>.part
     * and renamed to destination_path only after the download
     * finished successfully.
     *
     * @return request_id  used for progress/cancellation
     */

    QString download(const QUrl &url, const QString &destination_path, bool overwrite = true);
    void cancel(const QString &request_id);
    bool isDownloading(const QString &request_id) const;

signals:

    void downloadStartedChanged(
        const QString &request_id,
        const QUrl &url
        );

    void downloadProgressChanged(
        const QString &request_id,
        qint64 bytes_received,
        qint64 bytes_total
        );
    void downloadCompletedChanged(
        const QString &request_id,
        const QString &local_path
        );
    void downloadFailedChanged(
        const QString &request_id,
        const QString &error
        );
    void downloadCancelledChanged(
        const QString &request_id
        );

private:
    struct DownloadTask
    {
        QString request_id;
        QString destination_path;
        QString temporary_path;

        QFile *file = nullptr;
        QNetworkReply *reply = nullptr;

        bool overwrite = true;
        bool cancelled = false;
        bool writeFailed = false;
        QString writeError;
    };
    void handleReadyRead(DownloadTask *task);
    void handleFinished(DownloadTask *task);
    void cleanupTask(DownloadTask *task);

private:

    QNetworkAccessManager m_networkManager;
    QHash<QString, DownloadTask *> m_tasks;
};

} // namespace network
} // namespace core
