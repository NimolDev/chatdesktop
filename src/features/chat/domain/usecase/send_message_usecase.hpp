#ifndef FEATURES_CHAT_DOMAIN_USECASE_SEND_MESSAGE_HPP
#define FEATURES_CHAT_DOMAIN_USECASE_SEND_MESSAGE_HPP

#include <QObject>
#include "attachment_model.hpp"
#include "repository/message_repository.hpp"

namespace domain {
namespace usecase {

class SendMessageUsecase : public QObject
{
    Q_OBJECT
public:
    explicit SendMessageUsecase(
        std::shared_ptr<domain::repository::MessageRepository> reposity,
        QObject *parent = nullptr
        );

    QFuture<QString> execute(const QString &receiver_id, const QString &msg);
    QFuture<QString> sendVoice(const QString &receiver, const QUrl &file, qint64 duration, const QString &messageId = {});
    QFuture<QString> sendImage(const QString &receiver_id, const QList<domain::AttachmentItem> &attachments, const QString &caption,
                               repository::UploadCancellationPtr cancellation = {});
    QFuture<QString> sendFiles(const QString &receiver_id, const QList<domain::AttachmentItem> &attachments, const QString &caption,
                               repository::UploadCancellationPtr cancellation = {});
    QFuture<bool> cancelUpload(const QString &upload_id);
signals:
    void fileUploadStated(const QString &id, const QString &file_path);
    void fileUploadProgress(const QString &id, double progress);
    void fileUploadFinish(const QString &id);
    void fileUploadCanceled(const QString &id);

private:
   std::shared_ptr<domain::repository::MessageRepository> m_repository;
};


} // namespace usecase
} // namespace domain


#endif // FEATURES_CHAT_DOMAIN_USECASE_SEND_MESSAGE_HPP
