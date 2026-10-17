#include "file_upload_operation.hpp"

#include <QNetworkReply>
#include <QTimer>

namespace core {
namespace xmpp {

FileUploadOperation::FileUploadOperation(
    std::shared_ptr<QPromise<UploadedFile>> promise, QObject *parent)
    : QObject(parent), m_promise(std::move(promise)),
      m_watcher(this), m_file(this), m_network(this)
{
    connect(&m_watcher, &QFutureWatcher<UploadedFile>::canceled,
            this, [this] { cancel(); });
    m_watcher.setFuture(m_promise->future());
    QTimer::singleShot(120000, this, [this] {
        finish({{}, tr("File upload timed out. Please retry.")});
    });
}

FileUploadOperation::~FileUploadOperation()
{
    if (!m_done) {
        m_done = true;
        m_promise->future().cancel();
        stopTransport();
        m_promise->finish();
    }
}

bool FileUploadOperation::open(const QString &path)
{
    if (isStopped()) {
        return false;
    }
    m_file.setFileName(path);
    if (!m_file.open(QIODevice::ReadOnly)) {
        finish({{}, m_file.errorString()});
        return false;
    }
    return true;
}

bool FileUploadOperation::isStopped() const
{
    return m_done || m_promise->isCanceled();
}

void FileUploadOperation::stopTransport()
{
    if (m_reply && m_reply->isRunning()) {
        // abort() can emit finished synchronously; m_done is set first.
        m_reply->abort();
    }
    m_file.close();
}

void FileUploadOperation::cancel()
{
    if (m_done) {
        return;
    }
    m_done = true;
    stopTransport();
    m_promise->finish();
    deleteLater();
}

void FileUploadOperation::finish(const UploadedFile &result)
{
    if (m_done) {
        return;
    }
    m_done = true;
    stopTransport();
    if (!m_promise->isCanceled()) {
        m_promise->addResult(result);
    }
    m_promise->finish();
    deleteLater();
}

void FileUploadOperation::put(const QNetworkRequest &request, const QUrl &downloadUrl,
                              std::function<void(qint64, qint64)> progress)
{
    if (isStopped()) {
        return;
    }
    // Report the initial state before Qt emits its first batched byte update.
    progress(100, m_file.size());
    if (isStopped()) {
        return;
    }
    m_network.setTransferTimeout(60000);
    m_reply = m_network.put(request, &m_file);
    connect(m_reply, &QNetworkReply::uploadProgress, this,
            [this, progress](qint64 sent, qint64 total) {
        if (!isStopped()) {
            progress(sent, total);
        }
    });
    connect(m_reply, &QNetworkReply::finished, this, [this, downloadUrl] {
        if (isStopped()) {
            return;
        }
        const int status = m_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (m_reply->error() != QNetworkReply::NoError) {
            finish({{}, m_reply->errorString()});
        } else if (status < 200 || status >= 300) {
            finish({{}, tr("File upload failed (HTTP %1).").arg(status)});
        } else {
            finish({downloadUrl, {}});
        }
    });
}

void watchFileUploads(const QList<QFuture<UploadedFile>> &uploads,
                      std::shared_ptr<QPromise<QList<UploadedFile>>> promise,
                      QObject *parent)
{
    auto *operation = new QObject(parent);
    auto *cancellation = new QFutureWatcher<QList<UploadedFile>>(operation);
    const auto cancelChildren = [uploads] {
        for (auto upload : uploads) {
            if (!upload.isFinished()) {
                upload.cancel();
            }
        }
    };
    QObject::connect(cancellation, &QFutureWatcher<QList<UploadedFile>>::canceled,
                     operation, cancelChildren);
    // Parent teardown must not leave transfers or promises pending.
    QObject::connect(operation, &QObject::destroyed, [promise, cancelChildren] {
        if (!promise->future().isFinished()) {
            promise->future().cancel();
            cancelChildren();
            promise->finish();
        }
    });
    cancellation->setFuture(promise->future());
    if (promise->isCanceled()) {
        cancelChildren();
    }
    if (uploads.isEmpty()) {
        promise->addResult(QList<UploadedFile>{});
        promise->finish();
        operation->deleteLater();
        return;
    }
    auto results = std::make_shared<QList<UploadedFile>>(uploads.size());
    auto remaining = std::make_shared<qsizetype>(uploads.size());
    for (qsizetype index = 0; index < uploads.size(); ++index) {
        auto *watcher = new QFutureWatcher<UploadedFile>(operation);
        QObject::connect(watcher, &QFutureWatcher<UploadedFile>::finished, operation,
                         [watcher, operation, promise, results, remaining, index] {
            if (watcher->future().isCanceled()) {
                (*results)[index] = {{}, QObject::tr("File upload canceled.")};
            } else {
                (*results)[index] = watcher->result();
            }
            watcher->deleteLater();
            if (--(*remaining) == 0) {
                if (!promise->isCanceled()) {
                    promise->addResult(*results);
                }
                promise->finish();
                operation->deleteLater();
            }
        });
        watcher->setFuture(uploads[index]);
    }
}

} // namespace xmpp
} // namespace core
