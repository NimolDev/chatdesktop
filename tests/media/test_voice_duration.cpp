#include "presentation/viewmodel/messaging_vm.hpp"
#include "data/mapping/message_payload_mapper.hpp"
#include <QTemporaryDir>
#include <QFile>
#include <QtTest>

class DurationRepository final : public domain::repository::MessageRepository
{
public:
    QFuture<domain::entity::MessageResponse> fetchMessageById(QString, int) override
    {
        return QtFuture::makeReadyValueFuture(domain::entity::MessageResponse{});
    }
    void sendMessage(domain::entity::Payload &) override { ++textSends; }
    QFuture<QString> sendVoice(domain::entity::Payload message, const QUrl &) override
    {
        payload = message;
        ++sends;
        return QtFuture::makeReadyValueFuture(QString());
    }
    QString currentUserId() const override { return QStringLiteral("self"); }
    domain::entity::Payload payload;
    int sends = 0;
    int textSends = 0;
};

class VoiceDurationTest : public QObject
{
    Q_OBJECT
private slots:
    void finalizedDurationOverridesPreviewDuration_data()
    {
        QTest::addColumn<qint64>("previewDuration");
        QTest::newRow("unloaded preview") << qint64(0);
        QTest::newRow("stale paused preview") << qint64(500);
    }

    void finalizedDurationOverridesPreviewDuration()
    {
        QFETCH(qint64, previewDuration);
        auto repository = std::make_shared<DurationRepository>();
        MessagingViewModel vm(std::make_shared<domain::usecase::MessageUsecase>(repository),
                              std::make_shared<domain::usecase::SendMessageUsecase>(repository));
        vm.fetchMessage("chat-one");
        QTRY_VERIFY(!vm.isLoading());
        auto *recorder = vm.findChild<core::media::VoiceRecorder *>();
        QVERIFY(recorder);
        QTemporaryDir directory;
        QFile audio(directory.filePath("duration-test.m4a"));
        QVERIFY(audio.open(QIODevice::WriteOnly));
        audio.write("recorded audio");
        audio.close();
        // Inject a finalized recording without opening the microphone.
        connect(recorder, &core::media::VoiceRecorder::stateChanged, &vm, [&] {
            if (recorder->state() == core::media::VoiceRecorder::Starting) {
                recorder->cancel();
                emit recorder->recordingReady(QUrl::fromLocalFile(audio.fileName()), 2345);
            }
        });
        vm.voiceController()->startVoiceRecording(QUrl::fromLocalFile(directory.path()));
        QCOMPARE(repository->sends, 0);
        vm.sendVoice(previewDuration);
        QCOMPARE(repository->sends, 1);
        QCOMPARE(repository->textSends, 0);
        QCOMPARE(repository->payload.content.duration, 2345);
        const auto dto = data::mapping::toDto(repository->payload);
        QCOMPARE(dto.toJson()["content"].toObject()["duration"].toInteger(), 2345);
    }

    void doesNotSendWithoutRecording()
    {
        auto repository = std::make_shared<DurationRepository>();
        MessagingViewModel vm(std::make_shared<domain::usecase::MessageUsecase>(repository),
                              std::make_shared<domain::usecase::SendMessageUsecase>(repository));
        vm.sendVoice(0);
        QCOMPARE(repository->sends, 0);
    }
};
QTEST_GUILESS_MAIN(VoiceDurationTest)
#include "test_voice_duration.moc"
