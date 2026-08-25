#ifndef FEATURES_CHAT_DOMAIN_USECASE_SEND_MESSAGE_HPP
#define FEATURES_CHAT_DOMAIN_USECASE_SEND_MESSAGE_HPP

#include <QObject>
#include "entity/messag_payload.hpp"
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

    void execute(const QString &receiver_id, const QString &msg);
signals:


private:
   std::shared_ptr<domain::repository::MessageRepository> m_repository;
};


} // namespace usecase
} // namespace domain


#endif // FEATURES_CHAT_DOMAIN_USECASE_SEND_MESSAGE_HPP
