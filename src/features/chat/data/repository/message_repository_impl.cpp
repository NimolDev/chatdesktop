#include "message_repository_impl.hpp"
#include "constants/app_constants.hpp"
#include "network/json_serializer.hpp"
#include "dto/message_dto.hpp"
#include "data/mapping/message_payload_mapper.hpp"
#include "data/dto/message_store.hpp"

namespace data {
namespace repository {


MessageRepositoryImpl::MessageRepositoryImpl(
    std::shared_ptr<core::xmpp::XmppManager> xmpp,
    std::shared_ptr<core::network::NetworkClient> network,
    QObject *parent)
    : domain::repository::MessageRepository(parent),
    m_xmpp(std::move (xmpp)),
    m_network(std::move (network))
{

    const QString current_userid = m_xmpp.get ()->currentJid ().split ("@").first ();
    m_currentUserId = current_userid;
    connect(
        m_xmpp.get (),
        &core::xmpp::XmppManager::messageReceived,
        this,
        &MessageRepositoryImpl::onMessageReceived
        );

}

QFuture<QList<domain::entity::Payload>> MessageRepositoryImpl::fetchMessageById(QString user_id)
{
    QUrlQuery query;
    query.addQueryItem (QStringLiteral ("limit"), "100");
    return m_network ->get (core::constants::AppConstants::messages (user_id), query)
        .then(QtFuture::Launch::Async,
              [](const core::network::NetworkResponse &response)
                  -> QList<domain::entity::Payload> {
            if (!response.isSuccess ()) {
                qWarning() << "Message request failed, status:"<<response.status_code;
                return {};
            }

            QString error;
            auto document = core::network::JsonSerializer::parse (response.body, &error);
            if (!document.has_value ()) {
                qWarning() << "Message pares error: "<<error;
                return {};
            }

            auto dto = core::network::JsonSerializer::fromObject<data::dto::Message> (document->object (), &error);
            if (!dto.has_value ()) {
                qWarning() << "Message mapping error: "<< error;
                return {};
            }
             QList<domain::entity::Payload> payloads;
            for (const dto::Item &message : std::as_const (dto->messages)) {
                data::dto::PayloadDto dto = data::dto::PayloadDto::fromJsonString (message.body);
                // qDebug() << "Message response: "<< dto.sender_id;
                payloads.append (std::move (dto.toDomain ()));
            }
            return payloads;
        });
}

void MessageRepositoryImpl::sendMessage(domain::entity::Payload &payload)
{
    const QString current_userid = m_xmpp.get ()->currentJid ().split ("@").first ();

    auto dto = data::mapping::toDto (payload);
    QString jsonString = dto.toJsonString ();
    m_xmpp->sendMessage (payload.receiver_id, jsonString);
    saveMessage (payload.receiver_id, jsonString);
    emit messageSent(payload);
}

QString MessageRepositoryImpl::currentUserId() const
{
    return m_xmpp.get ()->currentJid ().split ("@").first ();
}

void MessageRepositoryImpl::saveMessage(QString receiver_id, QString body)
{
    dto::MesssageStore message;

    message.recipient_id = receiver_id;
    message.body = body;

    QJsonObject request = message.toJson ();
    m_network->post (core::constants::AppConstants::saveMessages (), request)
        .then ([this](const core::network::NetworkResponse &response) {
            if (!response.isSuccess ()) {
                qWarning() << "Login request failed, status:" << response.status_code;
                return;
            }
            QString error;
            auto document = core::network::JsonSerializer::parse (response.body, &error);

            if (!document.has_value ()) {
                qWarning() << "Save message response parse error:"<<error;
                return;
            }
            return;
        });
}

void MessageRepositoryImpl::onMessageReceived(const core::xmpp::Message &message)
{
    // qDebug() << "Message receive:"<<message.from;
    data::dto::PayloadDto dto = data::dto::PayloadDto::fromJsonString (message.body);

    domain::entity::Payload payload;
    payload = dto.toDomain ();
    emit messageReceived (payload);

}



} // namespace repository
} // namespace data
