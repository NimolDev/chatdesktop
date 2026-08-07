#include "login_repository_impl.hpp"

namespace data {

LoginRepositoryImpl::LoginRepositoryImpl(
    std::shared_ptr<core::xmpp::XmppManager> xmpp_client,
    QObject *parent)
    : domain::LoginRepository(parent),
    m_xmppClient(std::move (xmpp_client))

{

    connect (
        m_xmppClient.get (),
        &core::xmpp::XmppManager::connectionStateChanged,
        this,
        &LoginRepositoryImpl::connectionStateChanged
        );

    connect (
        m_xmppClient.get (),
        &core::xmpp::XmppManager::connectionFailed,
        this,
        &domain::LoginRepository::loginFailed
        );
}

void LoginRepositoryImpl::login(domain::LoginRequest request)
{
    m_xmppClient->connectToServer (request.user_name, request.password, "", 0000);
}

void LoginRepositoryImpl::connectionStateChanged()
{

    if (m_currentState == m_xmppClient->connectionState ()) { return; }

    m_currentState = m_xmppClient->connectionState ();
    switch (m_xmppClient->connectionState ()) {
    case core::xmpp::XmppManager::ConnectionState::Connecting:
        emit connecting ();
        break;
    case core::xmpp::XmppManager::ConnectionState::Connected:
        mapConnected ();
        break;
    case core::xmpp::XmppManager::ConnectionState::Disconnected:
        emit disConnented ();
        break;
    }
}

void LoginRepositoryImpl::mapConnected()
{
    const QString jid = m_xmppClient->currentJid();

    domain::LoginResponse response;
    response.user_name = jid.section('@', 0, 0);
    response.user_jid = jid.section ("/",0,0);

    emit loginSucceeded(response);
}


} // namespace data