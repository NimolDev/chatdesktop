#ifndef FEATURES_CHAT_DATA_CONVERSATIONS_REPOSITORY_IMPL_HPP
#define FEATURES_CHAT_DATA_CONVERSATIONS_REPOSITORY_IMPL_HPP

#include "domain/repository/conversation_list_repository.hpp"
#include "network/network_client.hpp"


namespace data {
namespace repository {



class ConversationsRepositoryImpl final : public domain::repository::ConversationListRepository
{
public:
    explicit ConversationsRepositoryImpl(std::shared_ptr<core::network::NetworkClient> network);

    QFuture<QList<domain::entity::ConversationList>> fetchConversations() override;

private:
    std::shared_ptr<core::network::NetworkClient> m_network;
};

} // namespace repository
} // namespace data

#endif // FEATURES_CHAT_DATA_CONVERSATIONS_REPOSITORY_IMPL_HPP
