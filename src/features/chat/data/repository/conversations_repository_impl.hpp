#ifndef FEATURES_CHAT_DATA_CONVERSATIONS_REPOSITORY_IMPL_HPP
#define FEATURES_CHAT_DATA_CONVERSATIONS_REPOSITORY_IMPL_HPP

#include "domain/repository/conversation_list_repository.hpp"
#include "network/network_client.hpp"

#include <QObject>
#include <QPromise>
#include <QQueue>
#include <functional>

namespace data {
namespace repository {



class ConversationsRepositoryImpl final : public QObject, public domain::repository::ConversationListRepository
{
public:
    explicit ConversationsRepositoryImpl(std::shared_ptr<core::network::NetworkClient> network);

    QFuture<QList<domain::entity::ConversationList>> fetchConversations() override;

private:
    using Conversations = QList<domain::entity::ConversationList>;
    void fetchAttempt(const std::shared_ptr<QPromise<Conversations>> &promise, const QString &user_id);
    void retryPending();
private:
    std::shared_ptr<core::network::NetworkClient> m_network;
    QQueue<std::function<void()>> m_queue;
};

} // namespace repository
} // namespace data

#endif // FEATURES_CHAT_DATA_CONVERSATIONS_REPOSITORY_IMPL_HPP
