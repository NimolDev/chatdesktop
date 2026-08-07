#ifndef FEATURES_AUTHENTICATION_DATA_LOGIN_REPOSITORY_IMPL_HPP
#define FEATURES_AUTHENTICATION_DATA_LOGIN_REPOSITORY_IMPL_HPP

#include "domain/login_repository.hpp"
#include "xmpp/xmpp_manager.hpp"

namespace data {

class LoginRepositoryImpl final : public domain::LoginRepository
{
    Q_OBJECT
public:
    explicit LoginRepositoryImpl(
        std::shared_ptr<core::xmpp::XmppManager > xmpp_client, // not onwer ship
        QObject *parent = nullptr
        );
    // LoginRepository interface
public:
    void login(domain::LoginRequest request) override;

private slots:
    void connectionStateChanged();
    // void connectionFailed()

private:
    void mapConnected();
    std::shared_ptr<core::xmpp::XmppManager> m_xmppClient;

    core::xmpp::XmppManager::ConnectionState m_currentState;

};





} // namespace data


#endif // FEATURES_AUTHENTICATION_DATA_LOGIN_REPOSITORY_IMPL_HPP
