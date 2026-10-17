#include "presentation/viewmodel/messaging_vm.hpp"
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

class QueuedVoiceRepository final : public domain::repository::MessageRepository
{
public:
    QList<domain::entity::Payload> messages;
    QHash<QString, std::shared_ptr<QPromise<QString>>> sends;
    QFuture<domain::entity::MessageResponse> fetchMessageById(QString, int) override
    {
        return QtFuture::makeReadyValueFuture(domain::entity::MessageResponse{});
    }
    QString currentUserId() const override { return QStringLiteral("self"); }
    QFuture<QString> sendMessage(domain::entity::Payload &) override { return ready(); }
    QFuture<QString> sendFile(domain::entity::Payload &, const QUrl &) override { return ready(); }
    QFuture<QString> sendFiles(domain::entity::Payload &, const domain::AttachmentItem &,
                               domain::repository::UploadCancellationPtr) override { return ready(); }
    QFuture<QString> sendImage(domain::entity::Payload &, const QList<domain::AttachmentItem> &,
                               domain::repository::UploadCancellationPtr) override { return ready(); }
    QFuture<QString> sendVoice(domain::entity::Payload &payload, const QUrl &) override
    {
        messages.append(payload);
        auto promise = std::make_shared<QPromise<QString>>();
        promise->start();
        sends.insert(payload.message_id, promise);
        domain::entity::MessageItem item;
        item.id = payload.message_id;
        item.sender_id = payload.sender_id;
        item.recipient_id = payload.receiver_id;
        item.body = payload;
        emit messageSent(item);
        emit fileUploadStated(payload.message_id, payload.content.media_url);
        return promise->future();
    }
    QFuture<bool> cancelFileUpload(const QString &id) override
    {
        auto promise = sends.value(id);
        if (!promise) {
            return QtFuture::makeReadyValueFuture(false);
        }
        promise->future().cancel();
        promise->finish();
        emit fileUploadCanceled(id);
        return QtFuture::makeReadyValueFuture(true);
    }
    static QFuture<QString> ready() { return QtFuture::makeReadyValueFuture(QString{}); }
};

class VoiceUploadQueueTest : public QObject
{
    Q_OBJECT
private slots:
    void queuedVoicesReleaseRecorderAndKeepIndependentProgress()
    {
        auto repo = std::make_shared<QueuedVoiceRepository>();
        MessagingViewModel vm(std::make_shared<domain::usecase::MessageUsecase>(repo),
                              std::make_shared<domain::usecase::SendMessageUsecase>(repo));
        vm.fetchMessage("peer");
        QTRY_VERIFY(!vm.isLoading());
        auto *recorder = vm.findChild<core::media::VoiceRecorder *>();
        QVERIFY(recorder);
        QTemporaryDir directory;
        QFile audio(directory.filePath("voice.m4a"));
        QVERIFY(audio.open(QIODevice::WriteOnly));
        audio.write("recording bytes");
        audio.close();
        const auto file = QUrl::fromLocalFile(audio.fileName());
        connect(recorder, &core::media::VoiceRecorder::stateChanged, &vm, [&] {
            if (recorder->state() == core::media::VoiceRecorder::Starting) {
                recorder->cancel();
                emit recorder->recordingReady(file, 1200);
            }
        });
        QSignalSpy queued(&vm, &MessagingViewModel::voiceMessageQueued);
        for (int i = 0; i < 2; ++i) {
            vm.voiceController()->startVoiceRecording(QUrl::fromLocalFile(directory.path()));
            vm.sendVoice(1200);
            QVERIFY(!vm.isSendingVoice());
            QVERIFY(vm.voiceController()->voiceRecordingFile().isEmpty());
            QVERIFY(QFileInfo::exists(audio.fileName()));
        }
        QCOMPARE(queued.count(), 2);
        QCOMPARE(repo->messages.size(), 2);
        QCOMPARE(vm.rowCount(), 2);
        const auto first = repo->messages[0].message_id;
        const auto second = repo->messages[1].message_id;
        QVERIFY(first != second);
        QVERIFY(vm.data(vm.index(0), MessagingViewModel::IsUploadingRole).toBool());
        QVERIFY(vm.data(vm.index(1), MessagingViewModel::IsUploadingRole).toBool());
        emit repo->fileUploadProgress(first, 0.4);
        QCOMPARE(vm.data(vm.index(0), MessagingViewModel::UploadProgressRole).toDouble(), 0.4);
        QCOMPARE(vm.data(vm.index(1), MessagingViewModel::UploadProgressRole).toDouble(), 0.0);

        // A completed older upload must not discard the next recording preview.
        vm.voiceController()->startVoiceRecording(QUrl::fromLocalFile(directory.path()));
        const auto revision = vm.voiceController()->recordingRevision();
        repo->sends[first]->addResult(QString{});
        repo->sends[first]->finish();
        QTRY_VERIFY(!vm.data(vm.index(0), MessagingViewModel::IsUploadingRole).toBool());
        QCOMPARE(vm.voiceController()->recordingRevision(), revision);
        QCOMPARE(vm.voiceController()->voiceRecordingFile(), file);
        QCOMPARE(queued.count(), 2);
        QVERIFY(vm.data(vm.index(1), MessagingViewModel::IsUploadingRole).toBool());

        vm.cancelFileUpload(second);
        QTRY_COMPARE(vm.rowCount(), 1);
        QCOMPARE(vm.voiceController()->voiceRecordingFile(), file);
        QVERIFY(!vm.isSendingVoice());
    }
};

QTEST_GUILESS_MAIN(VoiceUploadQueueTest)
#include "test_voice_upload_queue.moc"
