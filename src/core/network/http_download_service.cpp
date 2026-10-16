#include "http_download_service.hpp"

#include <QUuid>
#include <QFileInfo>
#include <QDir>
#include <QNetworkRequest>
#include <QTimer>

core::network::HttpDownloadService::HttpDownloadService(QObject *parent)
    : QObject(parent)
{
}

core::network::HttpDownloadService::~HttpDownloadService()
{
    const auto tasks = m_tasks.values ();

    for (auto *task : tasks) {
        if (task->reply) {
            task->reply->disconnect(this);
            task->reply->abort ();
        }

        if (task->file) {
            task->file->close ();
        }
        QFile::remove(task->temporary_path);
        cleanupTask (task);
    }
}

QString core::network::HttpDownloadService::download(
    const QUrl &url,
    const QString &destination_path, bool overwrite)
{
    if (!url.isValid() || url.host().isEmpty()
        || (url.scheme() != QStringLiteral("http") && url.scheme() != QStringLiteral("https"))) {
        return {};
    }
    if (destination_path.isEmpty ()) {
        return{};
    }
    const QString request_id = QUuid::createUuid ().toString (QUuid::WithoutBraces);
    const QFileInfo destination_info(destination_path);

    QDir directory;

    if (!directory.mkpath (destination_info.absolutePath())) {
        QTimer::singleShot(0, this, [this, request_id] {
            emit downloadFailedChanged(request_id, QStringLiteral("Unable to create destination directory"));
        });
        return request_id;
    }

    const QString temporary_path = destination_path + QLatin1Char('.') + request_id + QStringLiteral(".part");

    // remove stale partial download.
    if (QFile::exists (temporary_path)) {
        QFile::remove (temporary_path);
    }

    auto *file = new QFile(temporary_path, this);
    if (!file->open (QIODevice::WriteOnly)) {
        const auto error = file->errorString();
        QTimer::singleShot(0, this, [this, request_id, error] {
            emit downloadFailedChanged(request_id, error);
        });
        file->deleteLater ();
        return request_id;
    }

    QNetworkRequest request(url);
    request.setTransferTimeout(60000);

    request.setAttribute (
        QNetworkRequest::RedirectPolicyAttribute,
        QNetworkRequest::NoLessSafeRedirectPolicy
        );
    auto *reply = m_networkManager.get (request);
    auto *task = new DownloadTask{
        .request_id = request_id,
        .destination_path = destination_path,
        .temporary_path = temporary_path,
        .file = file,
        .reply = reply,
        .overwrite = overwrite
    };

    m_tasks.insert (request_id, task);

    connect(
        reply,
        &QNetworkReply::readyRead,
        this,
        [this, task]() {
            handleReadyRead (task);
        }
        );
    connect(
        reply,
        &QNetworkReply::downloadProgress,
        this,
        [this, task](qint64 bytes_received, qint64 bytes_total) {
            if (task->cancelled) {
                return;
            }
            emit this->downloadProgressChanged (
                task->request_id,
                bytes_received,
                bytes_total
                );
        }
        );
    connect(
        reply,
        &QNetworkReply::finished,
        this,
        [this, task]() {
            handleFinished (task);
        }
        );
    emit downloadStartedChanged (request_id, url);
    return request_id;
}

void core::network::HttpDownloadService::cancel(const QString &request_id)
{
    auto iterator = m_tasks.find (request_id);

    if (iterator == m_tasks.end ()) {
        return;
    }
    DownloadTask *task = iterator.value ();

    if (task->cancelled) {
        return;
    }
    task->cancelled = true;

    if (task->reply) {
        task->reply->abort ();
    }
}

bool core::network::HttpDownloadService::isDownloading(const QString &request_id) const
{
    return m_tasks.contains (request_id);
}

void core::network::HttpDownloadService::handleReadyRead(DownloadTask *task)
{
    if (!task
        || task->cancelled
        || task->writeFailed
        || !task->reply
        || !task->file) {
        return;
    }

    const QByteArray data = task->reply->readAll ();

    if (data.isEmpty ()) {
        return;
    }

    const qint64 written = task->file->write (data);

    if (written != data.size ()) {
        task->writeFailed = true;

        task->writeError = task->file->errorString ();
        QMetaObject::invokeMethod(task->reply, &QNetworkReply::abort, Qt::QueuedConnection);
    }
}

void core::network::HttpDownloadService::handleFinished(DownloadTask *task)
{
    if (!task) {
        return;
    }

    // There may still be unread buffered data when finished fires.
    handleReadyRead(task);
    if (task->file) {
        if (!task->file->flush()) {
            task->writeFailed = true;
            task->writeError = task->file->errorString();
        }
        if (task->file->size() == 0 && !task->writeFailed) {
            task->writeFailed = true;
            task->writeError = QStringLiteral("Downloaded file is empty.");
        }
        task->file->close();
    }

    // ---------------------------------------------------------
    // Cancelled
    // ---------------------------------------------------------
    if (task->cancelled) {
        QFile::remove(task->temporary_path);
        emit downloadCancelledChanged(
            task->request_id
            );
        cleanupTask(task);
        return;
    }

    // ---------------------------------------------------------
    // Local filesystem write failure
    // ---------------------------------------------------------
    if (task->writeFailed) {
        QFile::remove(task->temporary_path);
        emit downloadFailedChanged(
            task->request_id,
            task->writeError.isEmpty()
                ? QStringLiteral("Unable to write downloaded file")
                : task->writeError
            );
        cleanupTask(task);
        return;
    }

    // ---------------------------------------------------------
    // Network failure
    // ---------------------------------------------------------
    if (task->reply->error() != QNetworkReply::NoError) {
        const QString error = task->reply->errorString();
        QFile::remove(task->temporary_path);
        emit downloadFailedChanged(
            task->request_id,
            error
            );
        cleanupTask(task);
        return;
    }

    // ---------------------------------------------------------
    // Validate HTTP response
    // ---------------------------------------------------------
    const QVariant statusAttribute = task->reply->attribute(QNetworkRequest::HttpStatusCodeAttribute );

    if (statusAttribute.isValid()) {
        const int statusCode = statusAttribute.toInt();
        if (statusCode < 200 || statusCode >= 300) {
            QFile::remove( task->temporary_path);
            emit downloadFailedChanged(
                task->request_id,
                QStringLiteral("HTTP error: %1").arg(statusCode)
                );

            cleanupTask(task);
            return;
        }

    }

    // ---------------------------------------------------------
    // Replace existing cache file
    // ---------------------------------------------------------
    if (task->overwrite && QFile::exists(task->destination_path)) {
        if (!QFile::remove(task->destination_path)) {
            QFile::remove( task->temporary_path);
            emit downloadFailedChanged(
                task->request_id,
                QStringLiteral( "Unable to replace existing file")
                );
            cleanupTask(task);
            return;
        }
    }

    // ---------------------------------------------------------
    // .part -> final file
    // ---------------------------------------------------------
    if (!QFile::rename( task->temporary_path, task->destination_path)) {
        QFile::remove( task->temporary_path);
        emit downloadFailedChanged(
            task->request_id,
            QStringLiteral( "Unable to finalize downloaded file" )
            );
        cleanupTask(task);
        return;
    }

    const QString requestId = task->request_id;
    const QString destinationPath = task->destination_path;

    cleanupTask(task);

    emit downloadCompletedChanged(
        requestId,
        destinationPath
    );
}

void core::network::HttpDownloadService::cleanupTask(DownloadTask *task)
{
    if (!task) {
        return;
    }

    m_tasks.remove(task->request_id);
    if (task->reply) {
        task->reply->deleteLater();
        task->reply = nullptr;
    }

    if (task->file) {
        task->file->deleteLater();
        task->file = nullptr;
    }

    delete task;
}
