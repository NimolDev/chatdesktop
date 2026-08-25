#include "send_message_usecase.hpp"
#include <QUuid>
#include <QtCore/qdatetime.h>
namespace domain {
namespace usecase {

SendMessageUsecase::SendMessageUsecase(
    std::shared_ptr<repository::MessageRepository> reposity,
    QObject *parent)
    : m_repository(std::move (reposity)),
    QObject(parent)
{
}

void domain::usecase::SendMessageUsecase::execute(const QString &receiver_id, const QString &msg)
{
    domain::entity::Payload payload;
    payload.sender_id = m_repository->currentUserId ();
    payload.receiver_id = receiver_id;
    payload.message_id = QUuid::createUuid ().toString (QUuid::WithoutBraces);
    payload.reply_message_id = NULL;
    payload.timestamp = QString::number(QDateTime::currentSecsSinceEpoch());
    payload.content = {
        .type = domain::entity::MessageType::Text,
        .text = msg,
        .media_url = 0,
        .file_size = 0
    };
    m_repository->sendMessage (payload);
}

} // namespace usecase
} // namespace domain

