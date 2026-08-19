#include "session_repository_impl.hpp"

#include "local/login_response_store.hpp"


#include <utility>

namespace data {
namespace repository {

SessionRepositoryImpl::SessionRepositoryImpl(
    std::shared_ptr<core::xmpp::XmppManager> xmpp,
    QObject *parent)
    : domain::repository::SessionRepository(parent),
      m_xmpp(std::move(xmpp)),
      m_user(local::LoginResponseStore::getLocalUser())
{
    if (!m_xmpp || !m_user.has_value()) {
        return;
    }

}

bool SessionRepositoryImpl::sessionChecked() const
{
    if (!m_xmpp || !m_user.has_value()) {
        return false;
    }

    m_xmpp->connectToServer(
        m_user->xmpp_jid,
        m_user->password,
        m_user->xmpp_host,
        m_user->xmpp_port);
    // emit sessionChanged(false);
    return true;
}

bool SessionRepositoryImpl::logout()
{

    bool logout = data::local::LoginResponseStore::logout ();
    qDebug() << "Repo:"<<logout;
    if (logout) {
        if (m_xmpp) {
            m_xmpp->closeConnection();
        }
        m_user.reset();
        emit sessionChanged(false);
        return true;
    }
    return false;

}

bool SessionRepositoryImpl::isSession() const
{
    return m_user.has_value();
}


} // namespace repository
} // namespace data
