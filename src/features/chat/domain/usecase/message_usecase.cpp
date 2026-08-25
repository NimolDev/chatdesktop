#include "message_usecase.hpp"

namespace domain {
namespace usecase {

MessageUsecase::MessageUsecase(
    std::shared_ptr<repository::MessageRepository> repository,
    QObject *parent
    )
    : m_repository(std::move (repository)),
    QObject(parent)
{
    connect(
        m_repository.get (),
        &domain::repository::MessageRepository::messageReceived,
        this,
        [this](const domain::entity::Payload &payload) {
            const QString id = QString(payload.sender_id).remove(QLatin1Char('-'));
            qDebug() << "Message from: "<< payload.sender_id;
             if (id == m_userListener) {
                domain::entity::Payload newPayload = payload;
                const QString currentUserId = QString(m_repository->currentUserId())
                                                  .remove(QLatin1Char('-'));
                newPayload.is_mine = id == currentUserId;
                emit onMessagedReceived (newPayload);
             }
        }
        );
    connect (
        m_repository.get (),
        &domain::repository::MessageRepository::messageSent,
        this,
        [this](const domain::entity::Payload &payload) {
            domain::entity::Payload newPayload = payload;
            newPayload.is_mine = payload.sender_id == m_repository->currentUserId ();
            emit messageSent (newPayload);
        }
        );
}

QFuture<QList<entity::Payload>> MessageUsecase::execute(QString user_id)
{
    m_userListener = QString(user_id).remove(QLatin1Char('-'));

    return m_repository->fetchMessageById(user_id)
        .then(this, [this](QList<entity::Payload> payloads) {
            const QString currentUserId = QString(m_repository->currentUserId())
                                              .remove(QLatin1Char('-'));

            for (entity::Payload &payload : payloads) {
                const QString senderId = QString(payload.sender_id)
                                             .remove(QLatin1Char('-'));
                payload.is_mine = senderId == currentUserId;
            }

            return payloads;
        });
}

} // namespace usecase
} // namespace domain
