#ifndef FEATURES_AUTHENTICATION_DATA_LOGIN_REPOSITORY_IMPL_HPP
#define FEATURES_AUTHENTICATION_DATA_LOGIN_REPOSITORY_IMPL_HPP

#include <QNetworkAccessManager>

#include "repository/login_repository.hpp"
#include "xmpp/xmpp_manager.hpp"
#include "network/network_client.hpp"



namespace data {

class LoginRepositoryImpl final : public domain::repository::LoginRepository
{
    Q_OBJECT
public:
    explicit LoginRepositoryImpl(
        std::shared_ptr<core::xmpp::XmppManager > xmpp_client, // not onwer ship
        std::shared_ptr<core::network::NetworkClient> network_client,
        QObject *parent = nullptr
        );

public:
    void login(domain::entity::LoginRequest request) override;

private slots:
    void connectionStateChanged();
    // void connectionFailed()

private:
    void mapConnected();
    std::shared_ptr<core::xmpp::XmppManager> m_xmppClient;
    std::shared_ptr<core::network::NetworkClient> m_network;

    core::xmpp::XmppManager::ConnectionState m_currentState;

};


} // namespace data


#endif // FEATURES_AUTHENTICATION_DATA_LOGIN_REPOSITORY_IMPL_HPP
