#include "presentation/viewmodel/messaging_vm.hpp"
#include <QQmlContext>
#include <QQmlEngine>
#include <QtQuickTest/quicktest.h>

class PlaybackTestRepository final : public domain::repository::MessageRepository
{
public:
    QFuture<domain::entity::MessageResponse> fetchMessageById(QString, int) override
    {
        return QtFuture::makeReadyValueFuture(response);
    }
    QFuture<QString> sendMessage(domain::entity::Payload &) override
    {
        return QtFuture::makeReadyValueFuture(QString());
    }
    QFuture<QString> sendVoice(domain::entity::Payload &, const QUrl &) override
    {
        return QtFuture::makeReadyValueFuture(QString());
    }
    QFuture<QString> sendImage(domain::entity::Payload &, const QStringList &, domain::repository::UploadCancellationPtr = {}) override
    {
        return QtFuture::makeReadyValueFuture(QString());
    }
    QFuture<QString> sendFile(domain::entity::Payload &, const QUrl &) override
    {
        return QtFuture::makeReadyValueFuture(QString());
    }
    QFuture<QString> sendFiles(domain::entity::Payload &, const QStringList &, domain::repository::UploadCancellationPtr = {}) override
    {
        return QtFuture::makeReadyValueFuture(QString());
    }
    domain::entity::MessageResponse response{};
    QString currentUserId() const override { return QStringLiteral("test"); }
};

class AudioQmlSetup : public QObject
{
    Q_OBJECT
public slots:
    void applicationAvailable()
    {
        m_repository = std::make_shared<PlaybackTestRepository>();
        m_vm = std::make_unique<MessagingViewModel>(
            std::make_shared<domain::usecase::MessageUsecase>(m_repository),
            std::make_shared<domain::usecase::SendMessageUsecase>(m_repository));
    }
    void loadMessages(const QVariantList &rows)
    {
        m_repository->response = {};
        for (const auto &row : rows) {
            const auto values = row.toMap();
            domain::entity::MessageItem message{};
            message.id = values.value("id").toString();
            message.sender_id = QStringLiteral("test");
            message.body.timestamp = QStringLiteral("1791248400");
            domain::entity::Content content;
            content.type = static_cast<domain::entity::MessageType>(values.value("msgType").toInt());
            content.text = values.value("body").toString();
            content.media_url = values.value("filePath").toString();
            message.body.content = content;
            m_repository->response.messages.append(message);
        }
        m_vm->fetchMessage(QStringLiteral("conversation"));
    }
    void qmlEngineAvailable(QQmlEngine *engine)
    {
        engine->rootContext()->setContextProperty(QStringLiteral("testMessaging"), m_vm->voiceController());
        engine->rootContext()->setContextProperty(QStringLiteral("messageModel"), m_vm.get());
        engine->rootContext()->setContextProperty(QStringLiteral("messageFixture"), this);
    }
private:
    std::shared_ptr<PlaybackTestRepository> m_repository;
    std::unique_ptr<MessagingViewModel> m_vm;
};
QUICK_TEST_MAIN_WITH_SETUP(audio_playback, AudioQmlSetup)
#include "audio_qml_runner.moc"
