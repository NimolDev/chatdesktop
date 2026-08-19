#ifndef FEATURES_CHAT_DOMAIN_REPOSITORY_CONVERSATION_LIST_REPOSITORY_HPP
#define FEATURES_CHAT_DOMAIN_REPOSITORY_CONVERSATION_LIST_REPOSITORY_HPP

#include <QFuture>
#include <QList>
#include "domain/entity/conversation_list.hpp"

namespace domain {
namespace repository {

class ConversationListRepository
{
public:

    virtual ~ConversationListRepository() = default;
    virtual  QFuture<QList<domain::entity::ConversationList>> fetchConversations() = 0;
};

} // namespace repository
} // namespace domain



#endif // FEATURES_CHAT_DOMAIN_REPOSITORY_CONVERSATION_LIST_REPOSITORY_HPP
