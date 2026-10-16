#ifndef FEATURES_CHAT_DOMAIN_REPOSITORY_MESSAGE_REPOSITORY_HPP
#define FEATURES_CHAT_DOMAIN_REPOSITORY_MESSAGE_REPOSITORY_HPP

#include <QObject>
#include <QFuture>
#include <QUrl>
#include "attachment_model.hpp"
#include "upload_cancellation.hpp"


// #include "domain/entity/messags.hpp"
#include "domain/entity/messag_payload.hpp"


namespace domain {
namespace repository {


class MessageRepository : public QObject
{
    Q_OBJECT

public:
    explicit MessageRepository(QObject *parent = nullptr) : QObject(parent) {};
    virtual ~MessageRepository() = default;
    virtual QFuture<domain::entity::MessageResponse>fetchMessageById(QString user_id, int page) = 0;
    virtual QFuture<QString> sendMessage(domain::entity::Payload &payload) = 0;
    virtual QFuture<QString> sendVoice(domain::entity::Payload &payload, const QUrl &file) = 0;
    virtual QFuture<QString> sendImage(domain::entity::Payload &payload, const QList<domain::AttachmentItem> &attachments, UploadCancellationPtr cancellation = {}) = 0;
    virtual QFuture<QString> sendFile(domain::entity::Payload &payload, const QUrl &file) = 0;
    virtual QFuture<QString> sendFiles(domain::entity::Payload &payload, const QList<domain::AttachmentItem> &attachments, UploadCancellationPtr cancellation = {}) = 0;
    virtual QFuture<bool> cancelFileUpload(const QString &upload_id) = 0;

public:
    virtual QString currentUserId() const  = 0;

signals:
    void messageReceived(const domain::entity::MessageItem &payload);
    void messageSent(const domain::entity::MessageItem &payload);
    void fileUploadStated(const QString &id, const QString &file_path);
    void fileUploadProgress(const QString &id, double progress);
    void fileUploadFinish(const QString &id);
    void fileUploadCanceled(const QString &id);
};

} // namespace repository
} // namespace domain



#endif // FEATURES_CHAT_DOMAIN_REPOSITORY_MESSAGE_REPOSITORY_HPP
