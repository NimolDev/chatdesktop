#ifndef FEATURES_CHAT_DATA_REPOSITORY_MESSAGE_REPOSITORY_IMPL_HPP
#define FEATURES_CHAT_DATA_REPOSITORY_MESSAGE_REPOSITORY_IMPL_HPP

#include "xmpp/xmpp_manager.hpp"
#include "domain/repository/message_repository.hpp"
#include "network/network_client.hpp"

#include <QQueue>
#include <functional>

namespace data {
namespace repository {

class MessageRepositoryImpl final : public domain::repository::MessageRepository
{
public:

    explicit MessageRepositoryImpl(
        std::shared_ptr<core::xmpp::XmppManager> xmpp,
        std::shared_ptr<core::network::NetworkClient> network,
        QObject *parent = nullptr
        );

    // MessageRepository interface
public:

    // using SendFunctiion = std::function<QFuture<bool>(core::xmpp::UploadedFile &)>;

    QFuture<domain::entity::MessageResponse > fetchMessageById(QString user_id, int page) override;
    QFuture<QString> sendMessage(domain::entity::Payload &payload) override;
    QString currentUserId() const override;
    QFuture<QString> sendVoice(
        domain::entity::Payload &payload,
        const QUrl &file) override;
    QFuture<QString> sendImage(
        domain::entity::Payload &payload,
        const QList<domain::AttachmentItem> &attachments,
        domain::repository::UploadCancellationPtr cancellation = {}) override;
    QFuture<QString> sendFile(
        domain::entity::Payload &payload,
        const QUrl &file) override;
    QFuture<QString> sendFiles(
        domain::entity::Payload &payload,
        const domain::AttachmentItem &attachment,
        domain::repository::UploadCancellationPtr cancellation = {}) override;
    QFuture<bool> cancelFileUpload(const QString &upload_id) override;

private:

    struct FileUploadTask
    {
        QString id;
        QString file_path;

        domain::repository::UploadCancellationPtr cancellation;
        QFuture<core::xmpp::UploadedFile> future;
    };
    QHash<QString, std::shared_ptr<FileUploadTask>> m_uploadTasks;

    std::shared_ptr<core::network::NetworkClient> m_network;
    std::shared_ptr<core::xmpp::XmppManager> m_xmpp;
    QString m_currentUserId;



    struct SendQueueEntry
    {
        std::function<QFuture<QString>()> start;
        domain::repository::UploadCancellationPtr cancellation;
        std::shared_ptr<FileUploadTask> task;
        std::shared_ptr<QPromise<QString>> promise;

    };
    QQueue<SendQueueEntry> m_queue;
    QFuture<QString> enqueue(std::function<QFuture<QString>()> start,
                             domain::repository::UploadCancellationPtr cancellation = {},
                             std::shared_ptr<FileUploadTask> task = {});
    QFuture<QString> sendMessageNow(domain::entity::Payload payload);
    void nextProcess();
    bool m_processing = false;

private:
    void saveMessage(QString reciever_id, QString body);
private slots:
    void onMessageReceived(const core::xmpp::Message &message);
};

} // namespace repository
} // namespace data



#endif // FEATURES_CHAT_DATA_REPOSITORY_MESSAGE_REPOSITORY_IMPL_HPP
