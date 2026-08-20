#include "message_repository_impl.hpp"
#include "constants/app_constants.hpp"
#include "network/json_serializer.hpp"
#include "dto/message_dto.hpp"

namespace data {
namespace repository {

MessageRepositoryImpl::MessageRepositoryImpl(std::shared_ptr<core::network::NetworkClient> network)
    : m_network(std::move (network))
{}

QFuture<domain::entity::Message> MessageRepositoryImpl::fetchMessageById(QString user_id)
{
    return m_network ->get (core::constants::AppConstants::messages (user_id))
        .then([](const core::network::NetworkResponse &response) ->domain::entity::Message {
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
            domain::entity::Message messages = dto->toDomain ();
            return messages;
        });
}


} // namespace repository
} // namespace data

