#pragma once

#include "domain/repository/media_repository.hpp"
#include "network/http_download_service.hpp"
#include <QHash>
#include <QTemporaryFile>
#include <memory>

namespace data {
namespace repository {

class MediaRepositoryImpl final : public domain::repository::MediaRepository
{
public:
    explicit MediaRepositoryImpl(QObject *parent = nullptr);
    void downloadFile(const QString &messageId, const QUrl &url) override;
    void cancelDownload(const QString &messageId) override;

private:
    struct PendingDownload {
        QString messageId;
        QUrl url;
        QString path;
        QString accountId;
        QString cachePath;
        std::shared_ptr<QTemporaryFile> temporary;
    };
    QString takeMessage(const QString &requestId);
    core::network::HttpDownloadService m_downloads;
    QHash<QString, PendingDownload> m_requests;
    QHash<QString, QString> m_messages;
};

} // namespace repository
} // namespace data
