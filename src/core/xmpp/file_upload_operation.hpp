#ifndef CORE_XMPP_FILE_UPLOAD_OPERATION_HPP
#define CORE_XMPP_FILE_UPLOAD_OPERATION_HPP

#include <QFile>
#include <QFuture>
#include <QFutureWatcher>
#include <QNetworkAccessManager>
#include <QPointer>
#include <QPromise>
#include <QUrl>
#include <functional>
#include <memory>

namespace core {
namespace xmpp {

struct UploadedFile {
    QUrl url;
    QString error;
};

// Lives on the XMPP thread from slot negotiation through HTTP completion.
// Cancel the returned future from any thread; transport cleanup runs here.
class FileUploadOperation final : public QObject
{
public:
    explicit FileUploadOperation(std::shared_ptr<QPromise<UploadedFile>> promise,
                                 QObject *parent = nullptr);
    ~FileUploadOperation() override;

    bool open(const QString &path);
    bool isStopped() const;
    void put(const QNetworkRequest &request, const QUrl &downloadUrl,
             std::function<void(qint64, qint64)> progress);
    void finish(const UploadedFile &result);

private:
    void cancel();
    void stopTransport();

    std::shared_ptr<QPromise<UploadedFile>> m_promise;
    QFutureWatcher<UploadedFile> m_watcher;
    QFile m_file;
    QNetworkAccessManager m_network;
    QPointer<QNetworkReply> m_reply;
    bool m_done = false;
};

// Retains input order and propagates cancellation to every unfinished child.
void watchFileUploads(const QList<QFuture<UploadedFile>> &uploads,
                      std::shared_ptr<QPromise<QList<UploadedFile>>> promise,
                      QObject *parent);

} // namespace xmpp
} // namespace core

#endif // CORE_XMPP_FILE_UPLOAD_OPERATION_HPP
