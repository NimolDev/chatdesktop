#include "network/network_client.hpp"
#include <QTcpServer>
#include <QTcpSocket>
#include <QtTest>

class FastRetry final : public core::network::INetworkInterceptor
{
public:
    void interceptRequest(core::network::NetworkRequest &request) override
    {
        request.retry_policy.initial_delay_ms = 1;
        request.retry_policy.use_jitter = false;
    }
};

class NetworkRetryTest : public QObject
{
    Q_OBJECT
private slots:
    void configuredRetries_data()
    {
        QTest::addColumn<int>("retries");
        QTest::addColumn<int>("status");
        QTest::addColumn<int>("expectedAttempts");
        QTest::newRow("three-retries") << 3 << 503 << 4;
        QTest::newRow("disabled") << 0 << 503 << 1;
        QTest::newRow("negative-disabled") << -1 << 503 << 1;
        QTest::newRow("unauthorized-not-retried") << 3 << 401 << 1;
    }

    void configuredRetries()
    {
        QFETCH(int, retries);
        QFETCH(int, status);
        QFETCH(int, expectedAttempts);
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost));
        int requests = 0;
        connect(&server, &QTcpServer::newConnection, this, [&] {
            while (server.hasPendingConnections()) {
                auto *socket = server.nextPendingConnection();
                connect(socket, &QTcpSocket::readyRead, socket, [&, socket] {
                    const QByteArray received = socket->property("received").toByteArray() + socket->readAll();
                    socket->setProperty("received", received);
                    if (!received.contains("\r\n\r\n") || socket->property("answered").toBool()) {
                        return;
                    }
                    socket->setProperty("answered", true);
                    ++requests;
                    socket->write("HTTP/1.1 " + QByteArray::number(status)
                        + " Test\r\nContent-Length: 2\r\nConnection: close\r\n\r\n{}");
                    socket->disconnectFromHost();
                });
                connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
            }
        });
        core::network::NetworkClient client(QUrl(QStringLiteral("http://127.0.0.1:%1/").arg(server.serverPort())));
        client.addInterceptor(std::make_shared<FastRetry>());
        auto future = client.get("conversations", retries);
        QTRY_VERIFY_WITH_TIMEOUT(future.isFinished(), 5000);
        QCOMPARE(requests, expectedAttempts);
        QCOMPARE(future.result().attempt_count, expectedAttempts);
        QCOMPARE(future.result().status_code, status);
    }
};

QTEST_GUILESS_MAIN(NetworkRetryTest)
#include "test_network_retry.moc"
