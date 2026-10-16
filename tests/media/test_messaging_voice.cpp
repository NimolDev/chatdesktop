#include "presentation/viewmodel/messaging_vm.hpp"
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QFile>
#include <QPromise>
#include "data/mapping/message_payload_mapper.hpp"
#include <QtTest>

class FakeMessageRepository final : public domain::repository::MessageRepository
{
public:
    QFuture<domain::entity::MessageResponse> fetchMessageById(QString, int) override
    {
        return QtFuture::makeReadyValueFuture(response);
    }
    void sendMessage(domain::entity::Payload &) override { ++sent; }
    QString currentUserId() const override { return QStringLiteral("self"); }
    domain::entity::MessageResponse response{};
    int sent = 0;
    int voiceSends = 0;
    domain::entity::Payload voicePayload;
    QUrl voiceFile;
    std::shared_ptr<QPromise<QString>> voiceResult;
    QFuture<QString> sendVoice(domain::entity::Payload payload, const QUrl &file) override
    {
        ++voiceSends;
        voicePayload = payload;
        voiceFile = file;
        voiceResult = std::make_shared<QPromise<QString>>();
        voiceResult->start();
        return voiceResult->future();
    }
};

class MessagingVoiceTest : public QObject
{
    Q_OBJECT
private slots:
    void sendBeforeCaptureDoesNotLeaveUploadBusy()
    {
        auto repository = std::make_shared<FakeMessageRepository>();
        MessagingViewModel vm(std::make_shared<domain::usecase::MessageUsecase>(repository),
                              std::make_shared<domain::usecase::SendMessageUsecase>(repository));
        QVERIFY(vm.metaObject()->indexOfMethod("sendVoice(qint64)") >= 0);
        QCOMPARE(vm.voiceController()->metaObject()->indexOfMethod("sendVoice(qint64)"), -1);
        vm.fetchMessage("chat-one");
        QTRY_VERIFY(!vm.isLoading());
        auto *recorder = vm.findChild<core::media::VoiceRecorder *>();
        QVERIFY(recorder);
        connect(recorder, &core::media::VoiceRecorder::stateChanged, &vm, [&] {
            if (recorder->state() == core::media::VoiceRecorder::Starting) {
                vm.sendVoice(0);
            }
        });
        QTemporaryDir directory;
        vm.voiceController()->startVoiceRecording(QUrl::fromLocalFile(directory.path()));
        QVERIFY(!vm.isSendingVoice());
        QVERIFY(!vm.voiceController()->isVoiceRecordingBusy());
        QCOMPARE(repository->voiceSends, 0);
    }

    void explicitSendRetainsFailedRecordingForRetry()
    {
        auto repository = std::make_shared<FakeMessageRepository>();
        MessagingViewModel vm(std::make_shared<domain::usecase::MessageUsecase>(repository),
                              std::make_shared<domain::usecase::SendMessageUsecase>(repository));
        vm.fetchMessage("chat-one");
        QTRY_VERIFY(!vm.isLoading());
        auto *recorder = vm.findChild<core::media::VoiceRecorder *>();
        QVERIFY(recorder);
        QTemporaryDir directory;
        QFile audio(directory.filePath("voice.m4a"));
        QVERIFY(audio.open(QIODevice::WriteOnly));
        audio.write("test audio bytes");
        audio.close();
        const auto url = QUrl::fromLocalFile(audio.fileName());
        connect(recorder, &core::media::VoiceRecorder::stateChanged, &vm, [&] {
            if (recorder->state() == core::media::VoiceRecorder::Starting) {
                recorder->cancel();
                emit recorder->previewReady(url, 500);
                QCOMPARE(repository->voiceSends, 0);
                emit recorder->recordingReady(url, 1200);
            }
        });
        vm.voiceController()->startVoiceRecording(QUrl::fromLocalFile(directory.path()));
        QCOMPARE(repository->voiceSends, 0);
        vm.sendVoice(0);
        QVERIFY(vm.isSendingVoice());
        QCOMPARE(repository->voiceSends, 1);
        QCOMPARE(repository->sent, 0);
        QCOMPARE(repository->voicePayload.receiver_id, QString("chatone"));
        QCOMPARE(repository->voicePayload.content.type, domain::entity::MessageType::Voice);
        QCOMPARE(repository->voicePayload.content.duration, 1200);
        QCOMPARE(repository->voicePayload.content.file_size, audio.size());
        QVERIFY(repository->voicePayload.content.media_url.isEmpty());
        const auto dto = data::mapping::toDto(repository->voicePayload);
        QCOMPARE(dto.toJson()["content"].toObject()["duration"].toInteger(), 1200);
        vm.sendVoice(0);
        QCOMPARE(repository->voiceSends, 1);
        QCOMPARE(repository->sent, 0);
        repository->voiceResult->addResult(QString("Upload failed"));
        repository->voiceResult->finish();
        QTRY_VERIFY(!vm.isSendingVoice());
        QCOMPARE(vm.voiceController()->voiceRecordingFile(), url);
        QCOMPARE(vm.voiceSendError(), QString("Upload failed"));
        QSignalSpy sent(&vm, &MessagingViewModel::voiceMessageSent);
        vm.sendVoice(0);
        QCOMPARE(repository->voiceSends, 2);
        repository->voiceResult->addResult(QString());
        repository->voiceResult->finish();
        QTRY_COMPARE(sent.count(), 1);
        QVERIFY(!vm.isSendingVoice());
        QVERIFY(vm.voiceController()->voiceRecordingFile().isEmpty());
    }

    void voicePlaybackUsesEachUploadedUrl()
    {
        auto repository = std::make_shared<FakeMessageRepository>();
        for (int index = 0; index < 2; ++index) {
            domain::entity::MessageItem item{};
            item.id = QString::number(index);
            item.body.content.type = domain::entity::MessageType::Voice;
            item.body.content.media_url = QStringLiteral("https://upload.example/audio-%1.m4a").arg(index);
            item.body.timestamp = QStringLiteral("1700000000");
            repository->response.messages.append(item);
        }
        MessagingViewModel vm(std::make_shared<domain::usecase::MessageUsecase>(repository),
                              std::make_shared<domain::usecase::SendMessageUsecase>(repository));
        vm.fetchMessage("chat-one");
        QTRY_VERIFY(!vm.isLoading());
        QCOMPARE(vm.rowCount(), 2);
        for (int index = 0; index < 2; ++index) {
            QCOMPARE(vm.data(vm.index(index), MessagingViewModel::FilePathRole).toUrl(),
                     QUrl(repository->response.messages[index].body.content.media_url));
        }
    }

    void requiresConversation()
    {
        auto repository = std::make_shared<FakeMessageRepository>();
        MessagingViewModel vm(std::make_shared<domain::usecase::MessageUsecase>(repository),
                              std::make_shared<domain::usecase::SendMessageUsecase>(repository));
        QSignalSpy failures(vm.voiceController(), &VoiceMessageController::voiceRecordingFailed);
        vm.voiceController()->startVoiceRecording();
        QCOMPARE(failures.count(), 1);
        QVERIFY(!vm.voiceController()->voiceRecordingError().isEmpty());
        QVERIFY(!vm.voiceController()->isVoiceRecordingBusy());
    }

    void completionKeepsRecipientAndDoesNotSendText()
    {
        auto repository = std::make_shared<FakeMessageRepository>();
        MessagingViewModel vm(std::make_shared<domain::usecase::MessageUsecase>(repository),
                              std::make_shared<domain::usecase::SendMessageUsecase>(repository));
        vm.fetchMessage("chat-one");
        QTRY_VERIFY(!vm.isLoading());
        auto *recorder = vm.findChild<core::media::VoiceRecorder *>();
        QVERIFY(recorder);
        const QUrl file("file:///tmp/finalized-voice.m4a");
        QSignalSpy ready(vm.voiceController(), &VoiceMessageController::voiceRecordingReady);
        // Supply a completed backend result without opening a physical microphone.
        connect(recorder, &core::media::VoiceRecorder::stateChanged, &vm, [&] {
            if (recorder->state() == core::media::VoiceRecorder::Starting) {
                recorder->cancel();
                const QUrl preview("file:///tmp/preview-1.m4a");
                emit recorder->previewReady(preview, 600);
                QCOMPARE(vm.voiceController()->voiceRecordingFile(), preview);
                QCOMPARE(ready.count(), 0);
                emit recorder->recordingReady(file, 1200);
            }
        });
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        vm.voiceController()->startVoiceRecording(QUrl::fromLocalFile(directory.path()));
        QCOMPARE(ready.count(), 1);
        QCOMPARE(ready.first().at(0).toString(), QString("chatone"));
        QCOMPARE(ready.first().at(1).toUrl(), file);
        QCOMPARE(ready.first().at(2).toLongLong(), 1200);
        QCOMPARE(vm.voiceController()->voiceRecordingFile(), file);
        QCOMPARE(repository->sent, 0);
        vm.fetchMessage("chat-two");
        QTRY_VERIFY(!vm.isLoading());
        emit recorder->recordingReady(file, 1200);
        QCOMPARE(ready.count(), 1);
        QVERIFY(vm.voiceController()->voiceRecordingFile().isEmpty());
    }

    void resetCancelsPendingPermissionAndForwardsErrors()
    {
        auto repository = std::make_shared<FakeMessageRepository>();
        MessagingViewModel vm(std::make_shared<domain::usecase::MessageUsecase>(repository),
                              std::make_shared<domain::usecase::SendMessageUsecase>(repository));
        vm.fetchMessage("chat-one");
        QTRY_VERIFY(!vm.isLoading());
        auto *recorder = vm.findChild<core::media::VoiceRecorder *>();
        QVERIFY(recorder);
        QSignalSpy canceled(vm.voiceController(), &VoiceMessageController::voiceRecordingCanceled);
        connect(vm.voiceController(), &VoiceMessageController::voiceRecordingStateChanged, &vm, [&] {
            if (vm.voiceController()->isVoiceRecordingBusy()) {
                vm.resetModel();
            }
        });
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        vm.voiceController()->startVoiceRecording(QUrl::fromLocalFile(directory.path()));
        QCOMPARE(canceled.count(), 1);
        QVERIFY(!vm.voiceController()->isVoiceRecordingBusy());
        QSignalSpy failures(vm.voiceController(), &VoiceMessageController::voiceRecordingFailed);
        emit recorder->errorOccurred("Microphone unavailable");
        QCOMPARE(failures.count(), 1);
        QCOMPARE(vm.voiceController()->voiceRecordingError(), QString("Microphone unavailable"));
    }
};
QTEST_GUILESS_MAIN(MessagingVoiceTest)
#include "test_messaging_voice.moc"
