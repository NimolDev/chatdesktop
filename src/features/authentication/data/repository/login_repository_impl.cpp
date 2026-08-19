#include "login_repository_impl.hpp"

#include "network/json_serializer.hpp"
#include <constants/app_constants.hpp>
#include "dto/login_response_dto.hpp"
#include "dto/login_request_dto.hpp"
#include "mapping/login_mapper.hpp"
#include "local/login_response_store.hpp"

using namespace core::constants;

namespace data {
namespace repository {


LoginRepositoryImpl::LoginRepositoryImpl(
    std::shared_ptr<core::xmpp::XmppManager> xmpp_client,
    std::shared_ptr<core::network::NetworkClient> network_client,
    QObject *parent)
    : domain::repository::LoginRepository(parent),
    m_network(std::move (network_client)),
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
        &LoginRepositoryImpl::loginFailed
        );
}

void LoginRepositoryImpl::login(domain::entity::LoginRequest  request)
{

    /*
     * Login to restApi before connect to XMPP Server
     * Api response and connect to xmpp
     */


    m_request = data::mapping::toDto (request);
    QJsonObject body = core::network::JsonSerializer::toObject (m_request);
    m_network->post (core::constants::AppConstants::login (), body)
        .then ([this, request] (const core::network::NetworkResponse &response) {
            if (!response.isSuccess ()) {
                qWarning() << "Login request failed, status:" << response.status_code;
                return;
            }
            QString error;
            auto document = core::network::JsonSerializer::parse (response.body, &error);

            if (!document.has_value ()) {
                qWarning() << "Login response parse error:" << error;
                return;
            }
            m_response = core::network::JsonSerializer::fromObject<data::dto::LoginResponseDTO> (document->object (), &error);
            if (!m_response.has_value ()) {
                qWarning() << "Login response mapping failed:" << error;
                return;
            }
            // // qDebug() << "Response: " << dto->access_token;
            // // qDebug() << response.body;

            // domain::entity::LoginResponse result = m_response->toDomain();
            qDebug() << "Response: " << m_response->access_token;

            // const domain::entity::LoginResponse result = m_response->toDomain();
            if (!data::local::LoginResponseStore::save(
                    m_response.value(),
                    m_request.password)) {
                qWarning() << "Login succeeded, but saving the response failed";
            } else {
                qDebug() << "Login response saved successfully";
            }

            // m_xmppClient->connectToServer (result.xmpp.jid, request.password, result.xmpp.host, result.xmpp.port );

            emit loginSucceded ();
            connectToXmpp ();
        });

}

void LoginRepositoryImpl::connectToXmpp()
{
    m_xmppClient->connectToServer (
        m_response->xmpp.jid,
        m_request.password,
        m_response->xmpp.host,
        m_response->xmpp.port);
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
        emit connected();
        break;
    case core::xmpp::XmppManager::ConnectionState::Disconnected:
        emit disConnented ();
        break;
    }
}


} // namespace repository
} // namespace data
