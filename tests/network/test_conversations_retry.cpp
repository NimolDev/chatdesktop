#include "constants/app_constants.hpp"
#include "repository/conversations_repository_impl.hpp"
#include "network/network_monitor.hpp"
#include "storage/cache_storage.hpp"
#include "storage/user_data.hpp"
#include "presentation/viewmodel/conversations_vm.hpp"
#include <stdexcept>
#include <QTcpServer>
#include <QTcpSocket>
#include <QtTest>

// Keep this test independent of generated application secrets and endpoints.
namespace core {
namespace constants {
QString AppConstants::conversations()
{
    return QStringLiteral("conversations");
}
} // namespace constants
} // namespace core

namespace {
std::optional<core::storage::CurrentUser> currentUser;
QString savedPayload;
QString savedLogicalName;
QString savedSecret;
int saves = 0;
bool failCache = false;
QString cachedPayload;
} // namespace

// Fake the storage boundary: no real session or user cache is touched.
namespace core {
namespace storage {
std::optional<CurrentUser> UserData::getCurrentUser()
{
    return currentUser;
}
QString CacheStorage::getUser(const QString &logical_name, const QString &secret)
{
    if (logical_name != currentUser->user_id || secret != currentUser->user_id) {
        throw std::runtime_error("Wrong cache key");
    }
    if (failCache) {
        throw std::runtime_error("Cache unavailable");
    }
    return cachedPayload;
}
CacheStorage::CacheStorage() = default;
CacheStorage &CacheStorage::instance()
{
    static CacheStorage instance;
    return instance;
}
bool CacheStorage::save(const QString &payload, const QString &logical_name, const QString &secret)
{
    ++saves;
    if (failCache) {
        throw std::runtime_error("Cache unavailable");
    }
    savedPayload = payload;
    savedLogicalName = logical_name;
    savedSecret = secret;
    return true;
}
} // namespace storage
} // namespace core

class FailFirstResponses final : public core::network::INetworkInterceptor
{
public:
    int remaining = 2;
    int responses = 0;
    void interceptResponse(const core::network::NetworkRequest &,
                           core::network::NetworkResponse &response) override
    {
        ++responses;
        if (remaining > 0) {
            --remaining;
            response.error = core::network::NetworkError::UnknownNetworkError;
            response.body.clear();
        }
    }
};

class StreamingRepository final : public domain::repository::ConversationListRepository
{
public:
    QPromise<QList<domain::entity::ConversationList>> promise;
    QFuture<QList<domain::entity::ConversationList>> fetchConversations() override
    {
        promise.start();
        promise.addResult(QList<domain::entity::ConversationList>{{"cached", "Cached"}});
        return promise.future();
    }
};

class ConversationsRetryTest : public QObject
{
    Q_OBJECT
private slots:
    void modelDisplaysEachResult()
    {
        auto repository = std::make_shared<StreamingRepository>();
        auto usecase = std::make_shared<domain::usecase::ConversationsUsecase>(repository);
        ConversationsVM model(usecase);
        model.fetchConversations();
        QTRY_COMPARE(model.rowCount({}), 1);
        QCOMPARE(model.data(model.index(0), ConversationsVM::UuidRole).toString(), QStringLiteral("cached"));
        QVERIFY(!repository->promise.future().isFinished());
        QVERIFY(!model.isLoading());
        repository->promise.addResult(QList<domain::entity::ConversationList>{{"fresh", "Fresh"}});
        repository->promise.finish();
        QTRY_COMPARE(model.data(model.index(0), ConversationsVM::UuidRole).toString(), QStringLiteral("fresh"));
    }

    void retryCompletesOriginalFuture_data()
    {
        QTest::addColumn<QString>("userId");
        QTest::addColumn<bool>("cacheFailure");
        QTest::addColumn<QString>("cache");
        QTest::addColumn<bool>("changed");
        QTest::newRow("saved-for-starting-user") << QStringLiteral("account-1") << false << QString{} << true;
        QTest::newRow("no-user-skips-cache") << QString{} << false << QString{} << true;
        QTest::newRow("cache-failure-still-returns-data") << QStringLiteral("account-1") << true << QString{} << true;
        QTest::newRow("cache-before-retry") << QStringLiteral("account-1") << false
            << QStringLiteral(R"({"users":[{"uuid":"cached","name":"Cached"}]})") << true;
        QTest::newRow("corrupt-cache") << QStringLiteral("account-1") << false << QStringLiteral("broken") << true;
        QTest::newRow("unchanged") << QStringLiteral("account-1") << false
            << QStringLiteral(R"({ "users": [{"name":"Retry result","uuid":"user-1"}], "metadata":"ignored" })") << false;
        QTest::newRow("renamed") << QStringLiteral("account-1") << false
            << QStringLiteral(R"({"users":[{"uuid":"user-1","name":"Old name"}]})") << true;
        QTest::newRow("empty-cache") << QStringLiteral("account-1") << false
            << QStringLiteral(R"({"users":[]})") << true;
        QTest::newRow("removed-conversation") << QStringLiteral("account-1") << false
            << QStringLiteral(R"({"users":[{"uuid":"user-1","name":"Retry result"},{"uuid":"removed","name":"Removed"}]})") << true;
    }

    void retryCompletesOriginalFuture()
    {
        QFETCH(QString, userId);
        QFETCH(bool, cacheFailure);
        QFETCH(QString, cache);
        QFETCH(bool, changed);
        cachedPayload = cache;
        currentUser = userId.isEmpty() ? std::nullopt
            : std::optional<core::storage::CurrentUser>({QStringLiteral("test"), userId});
        saves = 0;
        savedPayload.clear();
        failCache = cacheFailure;
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost));
        connect(&server, &QTcpServer::newConnection, this, [&]() {
            while (server.hasPendingConnections()) {
                auto *socket = server.nextPendingConnection();
                connect(socket, &QTcpSocket::readyRead, socket, [socket]() {
                    auto received = socket->property("received").toByteArray() + socket->readAll();
                    socket->setProperty("received", received);
                    if (!received.contains("\r\n\r\n") || socket->property("answered").toBool()) {
                        return;
                    }
                    socket->setProperty("answered", true);
                    const QByteArray body = R"({"users":[{"uuid":"user-1","name":"Retry result"}]})";
                    socket->write("HTTP/1.1 200 OK\r\nContent-Length: " + QByteArray::number(body.size())
                                  + "\r\nConnection: close\r\n\r\n" + body);
                    socket->disconnectFromHost();
                });
                connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
            }
        });
        auto network = std::make_shared<core::network::NetworkClient>(
            QUrl(QStringLiteral("http://127.0.0.1:%1/").arg(server.serverPort())));
        auto failures = std::make_shared<FailFirstResponses>();
        network->addInterceptor(failures);
        data::repository::ConversationsRepositoryImpl repository(network);
        auto future = repository.fetchConversations();
        const bool hasCache = cache.startsWith('{');
        QCOMPARE(future.resultCount(), hasCache ? 1 : 0);
        if (hasCache) {
            QCOMPARE(future.resultAt(0).size(),
                     QJsonDocument::fromJson(cache.toUtf8()).object().value("users").toArray().size());
        }
        QTRY_COMPARE(failures->responses, 1);
        QVERIFY(!future.isFinished());
        QCOMPARE(saves, 0);
        currentUser = core::storage::CurrentUser{QStringLiteral("other"), QStringLiteral("account-2")};
        auto &monitor = core::network::NetworkMonitor::instance();
        emit monitor.networkConnected();
        QTRY_COMPARE(failures->responses, 2);
        QVERIFY(!future.isFinished());
        emit monitor.networkConnected();
        QTRY_VERIFY(future.isFinished());
        QCOMPARE(future.resultCount(), hasCache && changed ? 2 : 1);
        QCOMPARE(saves, userId.isEmpty() || !changed ? 0 : 1);
        if (!userId.isEmpty() && !cacheFailure && changed) {
            QCOMPARE(savedLogicalName, userId);
            QCOMPARE(savedSecret, userId);
            QCOMPARE(QJsonDocument::fromJson(savedPayload.toUtf8()).object()
                         .value("users").toArray().first().toObject().value("uuid").toString(),
                     QStringLiteral("user-1"));
        }
        QCOMPARE(future.resultAt(future.resultCount() - 1).size(), 1);
        QCOMPARE(future.resultAt(future.resultCount() - 1).first().uuid, QStringLiteral("user-1"));
        QCOMPARE(future.resultAt(future.resultCount() - 1).first().name, QStringLiteral("Retry result"));
        emit monitor.networkConnected();
        QCOMPARE(failures->responses, 3);
    }
};
QTEST_GUILESS_MAIN(ConversationsRetryTest)
#include "test_conversations_retry.moc"
