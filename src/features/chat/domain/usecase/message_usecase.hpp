#ifndef FEATURES_CHAT_DOMAIN_USECASE_MESSAGE_USECASE_HPP
#define FEATURES_CHAT_DOMAIN_USECASE_MESSAGE_USECASE_HPP

#include "repository/message_repository.hpp"

namespace domain {
namespace usecase {

class MessageUsecase
{
public:
   explicit MessageUsecase(std::shared_ptr<domain::repository::MessageRepository> repository);

    QFuture<domain::entity::Message> execute(QString user_id);
private:
    std::shared_ptr<domain::repository::MessageRepository> m_repository;
};


} // namespace usecase
} // namespace domain



#endif // FEATURES_CHAT_DOMAIN_USECASE_MESSAGE_USECASE_HPP
