#ifndef FEATURES_CHAT_DATA_REPOSITORY_MESSAGE_REPOSITORY_IMPL_HPP
#define FEATURES_CHAT_DATA_REPOSITORY_MESSAGE_REPOSITORY_IMPL_HPP

#include "domain/repository/message_repository.hpp"
#include "network/network_client.hpp"

namespace data {
namespace repository {

class MessageRepositoryImpl final : public domain::repository::MessageRepository
{
public:

   explicit MessageRepositoryImpl(std::shared_ptr<core::network::NetworkClient> network);

    // MessageRepository interface
public:
    QFuture<domain::entity::Message > fetchMessageById(QString user_id) override;

private:
    std::shared_ptr<core::network::NetworkClient> m_network;
};

} // namespace repository
} // namespace data



#endif // FEATURES_CHAT_DATA_REPOSITORY_MESSAGE_REPOSITORY_IMPL_HPP
