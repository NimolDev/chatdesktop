#include "media_repository_impl.hpp"
#include "storage/media_cache.hpp"

#include "storage/user_data.hpp"
#include <QCryptographicHash>
#include <QFileInfo>
#include <QStringList>
#include <QDir>
#include <QStandardPaths>
#include <algorithm>
#include <QFileDialog>

namespace data {
namespace repository {

MediaRepositoryImpl::MediaRepositoryImpl(QObject *parent)
    : domain::repository::MediaRepository(parent), m_downloads(this)
{
    using core::network::HttpDownloadService;
    connect(&m_downloads, &HttpDownloadService::downloadProgressChanged, this,
            [this](const QString &request, qint64 received, qint64 total) {
                if (m_requests.contains(request)) {
                    emit downloadProgress(m_requests.value(request).messageId, received, total);
                }
            });
    connect(&m_downloads, &HttpDownloadService::downloadCompletedChanged, this,
            [this](const QString &request, const QString &path) {
                const auto pending = m_requests.value(request);
                const auto message = takeMessage(request);
                if (!message.isEmpty()) {
                    if (pending.temporary) {
                        const bool cached = core::storage::MediaCache::saveFile(pending.cachePath, path);
                        if (!cached) {
                            emit downloadFailed(message, tr("Unable to save encrypted media cache."));
                            return;
                        }
                        emit downloadCompleted(message, pending.cachePath);
                    } else {
                        core::storage::MediaCache::saveLocalFilePath(pending.url, path, pending.accountId);
                        emit downloadCompleted(message, path);
                    }
                }
            });
    connect(&m_downloads, &HttpDownloadService::downloadFailedChanged, this,
            [this](const QString &request, const QString &error) {
                const auto message = takeMessage(request);
                if (!message.isEmpty()) {
                    emit downloadFailed(message, error);
                }
            });
    connect(&m_downloads, &HttpDownloadService::downloadCancelledChanged, this,
            [this](const QString &request) { takeMessage(request); });
}

QString MediaRepositoryImpl::takeMessage(const QString &requestId)
{
    const auto request = m_requests.take(requestId);
    if (!request.messageId.isEmpty()) {
        m_messages.remove(request.messageId);
    }
    return request.messageId;
}

void MediaRepositoryImpl::downloadFile(const QString &messageId, const QUrl &url)
{
    if (messageId.isEmpty() || !url.isValid() || url.host().isEmpty()
        || (url.scheme() != QStringLiteral("http") && url.scheme() != QStringLiteral("https"))) {
        emit downloadFailed(messageId, tr("Invalid file download URL."));
        return;
    }
    const auto existing = m_messages.value(messageId);
    if (!existing.isEmpty()) {
        if (m_requests.value(existing).url == url) {
            return;
        }
        cancelDownload(messageId);
    }
    const auto user = core::storage::UserData::getCurrentUser();
    const QString accountId = user ? QString(user->user_id).remove(QLatin1Char('-')) : QString{};
    // Hash both identity and URL to avoid unsafe filenames and stale URL reuse.
    const auto key = QString::fromLatin1(QCryptographicHash::hash(
        messageId.toUtf8() + '\0' + url.toEncoded(), QCryptographicHash::Sha256).toHex());
    const bool genericFile = messageId.startsWith(QStringLiteral("file:"));
    QString path;
    QString cachePath = core::storage::MediaCache::filePath(key, QFileInfo(url.path()).suffix(), accountId);
    std::shared_ptr<QTemporaryFile> temporary;
    if (genericFile) {
        const auto savedPath = core::storage::MediaCache::localFilePath(url, accountId);
        if (!savedPath.isEmpty()) {
            emit downloadStarted(messageId);
            emit downloadCompleted(messageId, savedPath);
            return;
        }
        // Sanitize only the suggested name; the dialog returns the full destination.
        QString name = QFileInfo(url.path()).fileName();
        name.replace(QLatin1Char('\\'), QLatin1Char('_'));
        for (auto &character : name) {
            if (character.unicode() < 32) {
                character = QLatin1Char('_');
            }
        }
        if (name.isEmpty() || name == QStringLiteral(".") || name == QStringLiteral("..")) {
            name = QStringLiteral("download");
        }
        const QString directory = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
        path = QFileDialog::getSaveFileName(
            nullptr, tr("Save File"), QDir(directory).filePath(name), tr("All Files (*)"));
        if (path.isEmpty()) {
            return; // User canceled.
        }
        path = QFileInfo(path).absoluteFilePath();
        const QString destinationDirectory = QFileInfo(path).absolutePath();
        if (!QDir().mkpath(destinationDirectory)) {
            emit downloadFailed(messageId, tr("Unable to access the folder: %1").arg(destinationDirectory));
            return;
        }
        // Do not let concurrent downloads replace the same destination.
        if (std::any_of(m_requests.cbegin(), m_requests.cend(),
                        [&path](const PendingDownload &pending) { return pending.path == path; })) {
            emit downloadFailed(messageId, tr("A download to this file is already in progress."));
            return;
        }
    } else {
        // Voice playback continues to use the media cache.
        QString extension = QFileInfo(url.path()).suffix().toLower();
        const QStringList audioExtensions = {
            QStringLiteral("m4a"), QStringLiteral("mp3"), QStringLiteral("aac"),
            QStringLiteral("wav"), QStringLiteral("ogg"), QStringLiteral("opus"),
            QStringLiteral("flac"), QStringLiteral("mp4"), QStringLiteral("webm")
        };
        if (!audioExtensions.contains(extension)) {
            extension = QStringLiteral("bin");
        }
        cachePath = core::storage::MediaCache::voicePath(key, extension, accountId);
        if (core::storage::MediaCache::exists(cachePath)) {
            emit downloadCompleted(messageId, cachePath);
            return;
        }
        temporary = std::make_shared<QTemporaryFile>();
        if (!temporary->open()) {
            emit downloadFailed(messageId, tr("Unable to prepare media download."));
            return;
        }
        path = temporary->fileName();
        temporary->close();
    }
    qDebug() << "Download path" << path;
    const auto request = m_downloads.download(url, path, true);
    if (request.isEmpty()) {
        emit downloadFailed(messageId, tr("Unable to start file download."));
        return;
    }
    m_requests.insert(request, {messageId, url, path, accountId, cachePath, temporary});
    m_messages.insert(messageId, request);
    emit downloadStarted(messageId);
}

void MediaRepositoryImpl::cancelDownload(const QString &messageId)
{
    const auto request = m_messages.take(messageId);
    m_requests.remove(request);
    m_downloads.cancel(request);
}

} // namespace repository
} // namespace data
