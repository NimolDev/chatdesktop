#include "media_repository_impl.hpp"
#include "storage/media_cache.hpp"

#include "storage/user_data.hpp"
#include <QCryptographicHash>
#include <QFileInfo>
#include <QStringList>
#include <QDir>
#include <QStandardPaths>
#include <algorithm>

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
                    // if (!pending.accountId.isEmpty()
                    //     && !core::storage::MessageStore::saveMediaPath(pending.accountId, pending.url.toString(), path)) {
                    //     qWarning() << "Could not save downloaded media path";
                    // }
                    emit downloadCompleted(message, path);
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
    // const QString savedPath = core::storage::MessageStore::localMediaPath(accountId, url.toString());
    // if (QFileInfo(savedPath).isFile()) {
    //     emit downloadCompleted(messageId, savedPath);
    //     return;
    // }
    // Hash both identity and URL to avoid unsafe filenames and stale URL reuse.
    const auto key = QString::fromLatin1(QCryptographicHash::hash(
        messageId.toUtf8() + '\0' + url.toEncoded(), QCryptographicHash::Sha256).toHex());
    const bool genericFile = messageId.startsWith(QStringLiteral("file:"));
    QString path;
    if (genericFile) {
        const QString directory = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
        if (directory.isEmpty() || !QDir().mkpath(directory)) {
            emit downloadFailed(messageId, tr("Unable to access the Downloads folder."));
            return;
        }
        // Keep only the URL's final path component; never allow a remote path
        // to escape the Downloads directory.
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
        const QFileInfo info(name);
        const QString suffix = info.suffix().isEmpty() ? QString() : QLatin1Char('.') + info.suffix();
        const QString base = info.completeBaseName();
        path = QDir(directory).filePath(name);
        int number = 1;
        // Also reserve destinations used by concurrent downloads.
        while (QFileInfo::exists(path) || QFileInfo(path).isSymLink()
               || std::any_of(m_requests.cbegin(), m_requests.cend(),
                              [&path](const PendingDownload &pending) { return pending.path == path; })) {
            path = QDir(directory).filePath(QStringLiteral("%1 (%2)%3").arg(base).arg(number++).arg(suffix));
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
        path = core::storage::MediaCache::voicePath(key, extension);
        const QFileInfo cached(path);
        if (cached.isFile() && cached.size() > 0) {
            // if (!accountId.isEmpty()) {
            //     core::storage::MessageStore::saveMediaPath(accountId, url.toString(), path);
            // }
            emit downloadCompleted(messageId, path);
            return;
        }
    }
    qDebug() << "Download path" << path;
    const auto request = m_downloads.download(url, path, !genericFile);
    if (request.isEmpty()) {
        emit downloadFailed(messageId, tr("Unable to start file download."));
        return;
    }
    m_requests.insert(request, {messageId, url, path, accountId});
    m_messages.insert(messageId, request);
}

void MediaRepositoryImpl::cancelDownload(const QString &messageId)
{
    const auto request = m_messages.take(messageId);
    m_requests.remove(request);
    m_downloads.cancel(request);
}

} // namespace repository
} // namespace data
