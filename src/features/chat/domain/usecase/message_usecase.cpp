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
    connect(m_repository.get(), &repository::MessageRepository::messageSendFinished,
            this, &MessageUsecase::messageSendFinished);
    connect(m_repository.get(), &repository::MessageRepository::messageUpdated,
            this, &MessageUsecase::messageUpdated);
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
    const QString current_userid = QString(m_repository->currentUserId())
                                      .remove(QLatin1Char('-'));


    auto promise = std::make_shared<QPromise<entity::MessageResponse>> ();
    promise->start ();
    auto result_future = promise->future ();

    auto repo_future = m_repository->fetchMessageById (user_id, page);

    auto *watcher = new QFutureWatcher<entity::MessageResponse>(this);
    connect(watcher,
            &QFutureWatcher<entity::MessageResponse>::resultReadyAt,
            this,
            [promise, watcher, current_userid](int index) {
                auto payloads = watcher->resultAt (index);
                for (entity::MessageItem &payload : payloads.messages) {
                    const QString senderId = QString(payload.sender_id)
                    .remove(QLatin1Char('-'));
                    payload.is_mine = senderId == current_userid;
                }
                qDebug() << "payload: "<< payloads.total;
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
                        if (leftOk != rightOk) {
                            return leftOk;
                        }
                        if (!leftOk) {
                            return false;
                        }

                        return leftEpoch < rightEpoch;
                    }
                    );
                promise->addResult (std::move (payloads));
            });

    connect(watcher,
            &QFutureWatcher<entity::MessageResponse>::finished,
            this,
            [promise, watcher]() {
                if (watcher->future ().isCanceled ()) {
                    promise->future().cancel();
                } else if (watcher->future().resultCount() == 0) {
                    promise->addResult(entity::MessageResponse{});
                }
                qDebug() << "Finished watcher: ";
                promise->finish ();
                watcher->deleteLater ();
            });

    connect(watcher, &QObject::destroyed, [promise] {
        if (!promise->future().isFinished()) {
            promise->future().cancel();
            promise->finish();
        }
    });
    watcher->setFuture (repo_future);
    return result_future;
}

} // namespace usecase
} // namespace domain
