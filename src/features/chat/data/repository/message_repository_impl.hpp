#ifndef FEATURES_CHAT_DATA_REPOSITORY_MESSAGE_REPOSITORY_IMPL_HPP
#define FEATURES_CHAT_DATA_REPOSITORY_MESSAGE_REPOSITORY_IMPL_HPP

#include "xmpp/xmpp_manager.hpp"
#include "domain/repository/message_repository.hpp"
#include "network/network_client.hpp"


namespace data {
namespace repository {

class MessageRepositoryImpl final : public domain::repository::MessageRepository
{
public:

   explicit MessageRepositoryImpl(
        std::shared_ptr<core::xmpp::XmppManager> xmpp,
        std::shared_ptr<core::network::NetworkClient> network,
         QObject *parent = nullptr
        );

    // MessageRepository interface
public:
   QFuture<QList<domain::entity::Payload> > fetchMessageById(QString user_id) override;
   void sendMessage(domain::entity::Payload &payload) override;
   QString currentUserId() const override;

private:
    std::shared_ptr<core::network::NetworkClient> m_network;
    std::shared_ptr<core::xmpp::XmppManager> m_xmpp;
    QString m_currentUserId;

private:
    void saveMessage(QString reciever_id, QString body);
private slots:
    void onMessageReceived(const core::xmpp::Message &message);

};

} // namespace repository
} // namespace data



#endif // FEATURES_CHAT_DATA_REPOSITORY_MESSAGE_REPOSITORY_IMPL_HPP
