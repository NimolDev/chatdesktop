#ifndef FEATURES_AUTHENTICATION_DATA_LOGIN_REPOSITORY_IMPL_HPP
#define FEATURES_AUTHENTICATION_DATA_LOGIN_REPOSITORY_IMPL_HPP

#include <QNetworkAccessManager>

#include "dto/login_response_dto.hpp"
#include "dto/login_request_dto.hpp"
#include "repository/login_repository.hpp"
#include "xmpp/xmpp_manager.hpp"
#include "network/network_client.hpp"


namespace data {
namespace repository {

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
    void connectToXmpp() override;

private slots:
    void connectionStateChanged();
    // void connectionFailed()

private:

    std::shared_ptr<core::xmpp::XmppManager> m_xmppClient;
    std::shared_ptr<core::network::NetworkClient> m_network;

    core::xmpp::XmppManager::ConnectionState m_currentState;

    std::optional<data::dto::LoginResponseDTO> m_response;
    data::dto::LoginRequest m_request;


};




} // namespace repository
} // namespace data



#endif // FEATURES_AUTHENTICATION_DATA_LOGIN_REPOSITORY_IMPL_HPP
