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
        [this](const domain::entity::MessageItem &payload) {
            const QString id = QString(payload.sender_id).remove(QLatin1Char('-'));
             if (id == m_userListener) {
                domain::entity::MessageItem newPayload = payload;
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
        [this](const domain::entity::MessageItem &payload) {
            domain::entity::MessageItem newPayload = payload;
            newPayload.is_mine = payload.sender_id == m_repository->currentUserId ();
            emit messageSent (newPayload);
        }
        );
}

QFuture<entity::MessageResponse> MessageUsecase::execute(QString user_id, int page)
{
    m_userListener = QString(user_id).remove(QLatin1Char('-'));
    // Access the QObject-backed repository before entering the worker thread.
    const QString currentUserId = QString(m_repository->currentUserId())
                                      .remove(QLatin1Char('-'));

    return m_repository->fetchMessageById(user_id, page)
        .then(QtFuture::Launch::Async,
              [currentUserId](entity::MessageResponse payloads) {
            for (entity::MessageItem &payload : payloads.messages) {
                const QString senderId = QString(payload.sender_id)
                                             .remove(QLatin1Char('-'));
                payload.is_mine = senderId == currentUserId;
            }
            std::stable_sort(
                payloads.messages.begin (),
                payloads.messages.end(),
                [](const domain::entity::MessageItem &left,
                   const domain::entity::MessageItem &right) {
                    bool leftOk = false;
                    bool rightOk = false;
                    const qint64 leftEpoch = left.body.timestamp.toLongLong(&leftOk);
                    const qint64 rightEpoch = right.body.timestamp.toLongLong(&rightOk);

                    // Keep valid timestamps before invalid values while preserving
                    // the original order of invalid entries (stable_sort).
                    if (leftOk != rightOk)
                        return leftOk;
                    if (!leftOk)
                        return false;

                    return leftEpoch < rightEpoch;
                }
                );
            return payloads;
        });
}

} // namespace usecase
} // namespace domain
