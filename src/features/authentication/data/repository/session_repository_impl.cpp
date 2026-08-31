#include "session_repository_impl.hpp"

#include "constants/app_constants.hpp"
#include "local/login_response_store.hpp"
#include "utils/token_validator.hpp"
#include "dto/refresh_token_response_dto.hpp"
#include <utility>


namespace data {
namespace repository {

SessionRepositoryImpl::SessionRepositoryImpl(
    std::shared_ptr<core::xmpp::XmppManager> xmpp,
    std::shared_ptr<core::network::NetworkClient> network,
    QObject *parent)
    : domain::repository::SessionRepository(parent),
    m_xmpp(std::move(xmpp)),
    m_network(std::move (network)),
    m_user(local::LoginResponseStore::getLocalUser())
{
    if (!m_xmpp || !m_user.has_value()) {
        return;
    }
}


QFuture<bool> SessionRepositoryImpl::sessionChecked()
{
    if (!m_xmpp || !m_user.has_value()) {
        // return QtFuture::makeReadyFuture(false);
        return QtFuture::makeReadyValueFuture (false);
    }
    // qDebug() << "token: " << m_user->access_token;
    QString refresh_token = m_user->refresh_token;
    QString access_token = m_user->access_token;

    const bool isExpired = core::utils::TokenValidator::isTokenExpired (access_token);
    if (isExpired) {
        return refreshToken ();
    }

    // emit sessionChanged(false);
    return QtFuture::makeReadyValueFuture (true);
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

void SessionRepositoryImpl::connectXmpp()
{
    if (!m_xmpp || !m_user) {
        return ;
    }
    m_xmpp->connectToServer(
        m_user->xmpp_jid,
        m_user->password,
        m_user->xmpp_host,
        m_user->xmpp_port);
}

QFuture<bool> SessionRepositoryImpl::refreshToken()
{
    QJsonObject object;
    object.insert (QStringLiteral ("refresh_token"), m_user->refresh_token);
    core::network::NetworkRequest request;
    request.url = QUrl(core::constants::AppConstants::refreshToken());
    request.method = core::network::HttpMethod::Post;
    request.requires_authentication = false;
    request.setJsonBody(object);

    return m_network->send(std::move(request))
        .then(this, [this] (const core::network::NetworkResponse &response) {
            if (!response.isSuccess ()) {
                qWarning() << "Refresh Token request failed, status:"<< response.status_code;
                return false;
            }
            QString error;
            auto document = core::network::JsonSerializer::parse (response.body, &error);
            if (!document.has_value ()) {
                qWarning() << "Refresh Token response parse error:"<< error;
                return false;
            }
            std::optional<data::dto::TokenResponseDto> token = core::network::JsonSerializer::fromObject<data::dto::TokenResponseDto> (
                document->object (),
                &error);
            if (!token.has_value ()) {
                qWarning() << "Token response mapping failed:"<< error;
                return false;
            }
            bool isSuccess = data::local::LoginResponseStore::updateToken (token.value ());
            if (isSuccess) {
                m_user->access_token = token->access_token;
                m_user->refresh_token = token->refresh_token;
                m_xmpp->connectToServer(
                    m_user->xmpp_jid,
                    m_user->password,
                    m_user->xmpp_host,
                    m_user->xmpp_port);
                qDebug() << "Save new token successful";
            }
            return isSuccess;
        });
}

bool SessionRepositoryImpl::isSession() const
{
    return m_user.has_value();
}

} // namespace repository
} // namespace data


