#include "file_upload_operation.hpp"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QSignalSpy>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryFile>
#include <QtTest>
#include <thread>

using core::xmpp::FileUploadOperation;
using core::xmpp::UploadedFile;
using core::xmpp::watchFileUploads;

class FileUploadTest : public QObject
{
    Q_OBJECT
    QTcpServer m_server;
    QTemporaryFile m_file;
    QList<QPointer<QTcpSocket>> m_sockets;
    int m_requests = 0;

    struct Upload {
        QFuture<UploadedFile> future;
        QPointer<FileUploadOperation> operation;
    };

    Upload create(QObject *parent = nullptr)
    {
        auto promise = std::make_shared<QPromise<UploadedFile>>();
        promise->start();
        auto *operation = new FileUploadOperation(promise, parent ? parent : this);
        operation->open(m_file.fileName());
        return {promise->future(), operation};
    }

    QUrl url(const QString &path = QStringLiteral("/slow")) const
    {
        return QUrl(QStringLiteral("http://127.0.0.1:%1%2").arg(m_server.serverPort()).arg(path));
    }

    void put(const Upload &upload, const QString &path = QStringLiteral("/slow"))
    {
        upload.operation->put(QNetworkRequest(url(path)), url("/download"),
                              [](qint64, qint64) {});
    }

    QFuture<QList<UploadedFile>> batch(const QList<QFuture<UploadedFile>> &uploads,
                                      QObject *parent = nullptr)
    {
        auto promise = std::make_shared<QPromise<QList<UploadedFile>>>();
        promise->start();
        watchFileUploads(uploads, promise, parent ? parent : this);
        return promise->future();
    }

private slots:
    void initTestCase()
    {
        QVERIFY(m_file.open());
        QCOMPARE(m_file.write(QByteArray(8 * 1024 * 1024, 'x')), qint64(8 * 1024 * 1024));
        m_file.flush();
        QVERIFY(m_server.listen(QHostAddress::LocalHost));
        connect(&m_server, &QTcpServer::newConnection, this, [this] {
            while (m_server.hasPendingConnections()) {
                auto *socket = m_server.nextPendingConnection();
                m_sockets.append(socket);
                connect(socket, &QTcpSocket::readyRead, socket, [this, socket] {
                    if (socket->property("stalled").toBool()) {
                        return;
                    }
                    auto bytes = socket->property("bytes").toByteArray() + socket->readAll();
                    socket->setProperty("bytes", bytes);
                    const auto headerEnd = bytes.indexOf("\r\n\r\n");
                    if (headerEnd < 0) {
                        return;
                    }
                    if (!socket->property("counted").toBool()) {
                        ++m_requests;
                        socket->setProperty("counted", true);
                    }
                    if (bytes.startsWith("PUT /slow")) {
                        socket->setReadBufferSize(1024);
                        socket->setProperty("stalled", true);
                        return;
                    }
                    if (bytes.size() - headerEnd - 4 < m_file.size()) {
                        return;
                    }
                    socket->setProperty("stalled", true);
                    socket->write("HTTP/1.1 201 Created\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");
                    socket->disconnectFromHost();
                });
            }
        });
    }

    void cleanup()
    {
        for (auto socket : std::as_const(m_sockets)) {
            if (socket) {
                delete socket;
            }
        }
        m_sockets.clear();
        m_requests = 0;
    }

    void cancelBeforeSlotPreventsLatePut()
    {
        auto upload = create();
        bool delivered = false;
        auto continuation = upload.future.then([&delivered](const UploadedFile &) { delivered = true; });
        upload.future.cancel();
        // Simulate a slot response racing ahead of the queued canceled signal.
        put(upload);
        QTRY_VERIFY(upload.future.isFinished());
        QVERIFY(upload.future.isCanceled());
        QCOMPARE(upload.future.resultCount(), 0);
        QTRY_VERIFY(upload.operation.isNull());
        QCOMPARE(m_requests, 0);
        QVERIFY(!delivered);
        QVERIFY(continuation.isCanceled());
    }

    void cancelActiveHttpFromAnotherThread()
    {
        auto upload = create();
        put(upload);
        QTRY_COMPARE(m_requests, 1);
        QVERIFY(!upload.future.isFinished());
        // Observe the actual reply abort rather than only the future's flag.
        auto *reply = upload.operation->findChild<QNetworkReply *>();
        QVERIFY(reply);
        bool aborted = false;
        connect(reply, &QNetworkReply::finished, this, [reply, &aborted] {
            aborted = reply->error() == QNetworkReply::OperationCanceledError;
        });
        std::thread canceler([future = upload.future]() mutable { future.cancel(); });
        canceler.join();
        QTRY_VERIFY(upload.future.isFinished());
        QTRY_VERIFY(upload.operation.isNull());
        QVERIFY(aborted);
        QVERIFY(upload.future.isCanceled());
        QCOMPARE(upload.future.resultCount(), 0);
    }

    void successStillReturnsUrl()
    {
        auto upload = create();
        put(upload, "/success");
        QTRY_VERIFY_WITH_TIMEOUT(upload.future.isFinished(), 10000);
        QVERIFY(!upload.future.isCanceled());
        QCOMPARE(upload.future.result().url, url("/download"));
        QVERIFY(upload.future.result().error.isEmpty());
    }

    void cancelBatchAbortsActiveAndPendingUploadsOnly()
    {
        auto active = create();
        auto pending = create();
        auto independent = create();
        put(active);
        QTRY_COMPARE(m_requests, 1);
        auto future = batch({active.future, pending.future});
        future.cancel();
        QTRY_VERIFY(future.isFinished());
        QTRY_VERIFY(active.operation.isNull());
        QTRY_VERIFY(pending.operation.isNull());
        QVERIFY(active.future.isCanceled());
        QVERIFY(pending.future.isCanceled());
        QVERIFY(!independent.future.isCanceled());
        QVERIFY(!independent.future.isFinished());
        independent.operation->finish({url("/independent"), {}});
        QCOMPARE(independent.future.result().url, url("/independent"));
        QCOMPARE(future.resultCount(), 0);
    }

    void batchPreservesOrder()
    {
        auto first = create();
        auto second = create();
        auto future = batch({first.future, second.future});
        second.operation->finish({url("/second"), {}});
        first.operation->finish({url("/first"), {}});
        QTRY_VERIFY(future.isFinished());
        QCOMPARE(future.result().size(), 2);
        QCOMPARE(future.result()[0].url, url("/first"));
        QCOMPARE(future.result()[1].url, url("/second"));
    }

    void parentDestructionFinishesPendingUploads()
    {
        auto *owner = new QObject;
        auto first = create(owner);
        auto second = create(owner);
        auto future = batch({first.future, second.future}, owner);
        delete owner;
        QVERIFY(first.future.isFinished());
        QVERIFY(second.future.isFinished());
        QVERIFY(future.isFinished());
        QVERIFY(future.isCanceled());
    }

    void emptyBatchFinishes()
    {
        auto future = batch({});
        QVERIFY(future.isFinished());
        QVERIFY(future.result().isEmpty());
    }
};

QTEST_GUILESS_MAIN(FileUploadTest)
#include "test_file_upload.moc"
