#include "usecase/message_usecase.hpp"
#include <QPromise>
#include <QtTest>

using namespace domain;

class Repository final : public repository::MessageRepository
{
public:
    std::shared_ptr<QPromise<entity::MessageResponse>> fetch;
    QFuture<entity::MessageResponse> fetchMessageById(QString, int) override
    {
        fetch = std::make_shared<QPromise<entity::MessageResponse>>();
        fetch->start();
        return fetch->future();
    }
    QString currentUserId() const override { return "self-id"; }
    QFuture<QString> sendMessage(entity::Payload &) override { return {}; }
    QFuture<QString> sendVoice(entity::Payload &, const QUrl &) override { return {}; }
    QFuture<QString> sendFile(entity::Payload &, const QUrl &) override { return {}; }
    QFuture<QString> sendImage(entity::Payload &, const QList<AttachmentItem> &, repository::UploadCancellationPtr) override { return {}; }
    QFuture<QString> sendFiles(entity::Payload &, const AttachmentItem &, repository::UploadCancellationPtr) override { return {}; }
    QFuture<bool> cancelFileUpload(const QString &) override { return {}; }
};

class MessageFetchTest : public QObject
{
    Q_OBJECT
private slots:
    void sendCompletionKeepsIdentitySeparateFromError()
    {
        auto repo = std::make_shared<Repository>();
        usecase::MessageUsecase usecase(repo);
        QSignalSpy finished(&usecase, &usecase::MessageUsecase::messageSendFinished);
        emit repo->messageSendFinished("voice-id", {});
        QCOMPARE(finished.count(), 1);
        QCOMPARE(finished.first().at(0).toString(), QString("voice-id"));
        QVERIFY(finished.first().at(1).toString().isEmpty());
        emit repo->messageSendFinished("failed-id", "Disconnected");
        QCOMPARE(finished.last().at(1).toString(), QString("Disconnected"));
    }

    void uploadedVoiceUpdatePreservesPlaybackSource()
    {
        auto repo = std::make_shared<Repository>();
        usecase::MessageUsecase usecase(repo);
        QSignalSpy updated(&usecase, &usecase::MessageUsecase::messageUpdated);
        entity::MessageItem voice{};
        voice.id = "voice-upload";
        voice.body.content.type = entity::MessageType::Voice;
        voice.body.content.media_url = "https://example.test/upload.m4a";
        voice.body.content.localpath = "/local/encrypted-voice-cache";
        emit repo->messageUpdated(voice);
        QCOMPARE(updated.count(), 1);
        const auto result = qvariant_cast<entity::MessageItem>(updated.first().first());
        QCOMPARE(result.id, voice.id);
        QCOMPARE(result.body.content.media_url, voice.body.content.media_url);
        QCOMPARE(result.body.content.localpath, voice.body.content.localpath);
    }

    void cacheArrivesBeforeRefreshCompletes()
    {
        auto repo = std::make_shared<Repository>();
        usecase::MessageUsecase usecase(repo);
        auto future = usecase.execute("peer", 1);
        entity::MessageResponse cached{};
        cached.page = 1;
        entity::MessageItem newer{};
        newer.id = "newer";
        newer.sender_id = "selfid";
        newer.body.timestamp = "20";
        auto older = newer;
        older.id = "older";
        older.body.timestamp = "10";
        cached.messages = {newer, older};
        repo->fetch->addResult(cached);
        QTRY_COMPARE(future.resultCount(), 1);
        QVERIFY(!future.isFinished());
        QCOMPARE(future.resultAt(0).messages.first().id, QString("older"));
        QVERIFY(future.resultAt(0).messages.first().is_mine);
        auto refreshed = cached;
        refreshed.total = 3;
        repo->fetch->addResult(refreshed);
        QTRY_COMPARE(future.resultCount(), 2);
        QCOMPARE(future.resultAt(1).total, 3);
        repo->fetch->finish();
        QTRY_VERIFY(future.isFinished());
    }
    void failedRefreshKeepsCache()
    {
        auto repo = std::make_shared<Repository>();
        usecase::MessageUsecase usecase(repo);
        auto future = usecase.execute("peer", 1);
        entity::MessageResponse cached{};
        cached.page = 1;
        repo->fetch->addResult(cached);
        QTRY_COMPARE(future.resultCount(), 1);
        repo->fetch->finish();
        QTRY_VERIFY(future.isFinished());
        QCOMPARE(future.resultCount(), 1);
        QCOMPARE(future.result().page, 1);
    }
    void failureWithoutCacheCompletes()
    {
        auto repo = std::make_shared<Repository>();
        usecase::MessageUsecase usecase(repo);
        auto future = usecase.execute("peer", 1);
        repo->fetch->finish();
        QTRY_VERIFY(future.isFinished());
        QCOMPARE(future.resultCount(), 1);
        QCOMPARE(future.result().page, 0);
    }
};
QTEST_GUILESS_MAIN(MessageFetchTest)
#include "test_message_fetch.moc"
