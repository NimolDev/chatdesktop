#ifndef FEATURES_CHAT_DOMAIN_USECASE_MESSAGE_USECASE_HPP
#define FEATURES_CHAT_DOMAIN_USECASE_MESSAGE_USECASE_HPP

#include <QObject>
#include <QFutureWatcher>

#include "repository/message_repository.hpp"


namespace domain {
namespace usecase {

class MessageUsecase : public QObject
{
    Q_OBJECT
public:
   explicit MessageUsecase(
        std::shared_ptr<domain::repository::MessageRepository> repository,
        QObject *parent = nullptr
        );

    QFuture<domain::entity::MessageResponse> execute(QString user_id, int page);

signals:
    void onMessagedReceived(const domain::entity::MessageItem &payload);
    void messageSent(const domain::entity::MessageItem &payload);

private:
    std::shared_ptr<domain::repository::MessageRepository> m_repository;
    QString m_userListener;
};


} // namespace usecase
} // namespace domain



#endif // FEATURES_CHAT_DOMAIN_USECASE_MESSAGE_USECASE_HPP
