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

    QFuture<QList<domain::entity::Payload>> execute(QString user_id);

signals:
    void onMessagedReceived(const domain::entity::Payload &payload);
    void messageSent(const domain::entity::Payload &payload);

private:
    std::shared_ptr<domain::repository::MessageRepository> m_repository;
    QString m_userListener;
};


} // namespace usecase
} // namespace domain



#endif // FEATURES_CHAT_DOMAIN_USECASE_MESSAGE_USECASE_HPP
