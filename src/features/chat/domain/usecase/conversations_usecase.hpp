#ifndef FEATURES_CHAT_DOMAIN_USECASE_CONVERSATIONS_USECASE_HPP
#define FEATURES_CHAT_DOMAIN_USECASE_CONVERSATIONS_USECASE_HPP

#include "domain/repository/conversation_list_repository.hpp"

namespace domain {
namespace usecase {

class ConversationsUsecase
{
public:
    explicit ConversationsUsecase(std::shared_ptr<domain::repository::ConversationListRepository> repository);

    QFuture<QList<domain::entity::ConversationList>> execute();

private:
    std::shared_ptr<domain::repository::ConversationListRepository> m_repository;
};


} // namespace usecase
} // namespace domain

#endif // FEATURES_CHAT_DOMAIN_USECASE_CONVERSATIONS_USECASE_HPP
