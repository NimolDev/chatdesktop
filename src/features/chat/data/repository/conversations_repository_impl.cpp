#include "conversations_repository_impl.hpp"
#include "constants/app_constants.hpp"
#include "network/json_serializer.hpp"
#include "dto/conversations_dto.hpp"
#include <QList>

namespace data {
namespace repository {

ConversationsRepositoryImpl::ConversationsRepositoryImpl(
    std::shared_ptr<core::network::NetworkClient> network
    )
    : m_network(std::move (network))
{
}

QFuture<QList<domain::entity::ConversationList>> ConversationsRepositoryImpl::fetchConversations()
{
    return m_network->get (core::constants::AppConstants::conversations ())
        .then (QtFuture::Launch::Async,
               [](const core::network::NetworkResponse &response)
                   -> QList<domain::entity::ConversationList> {
            if (!response.isSuccess ()) {
                qWarning() << "Conversatioin request failed, status:"<< response.status_code;
                return {};
            }
            QString error;
            auto document = core::network::JsonSerializer::parse (response.body, &error);
            if (!document.has_value ()) {
                qWarning() << "Conversatioin parse error:"<<error;
                return {};
            }
            auto dto = core::network::JsonSerializer::fromObject<data::dto::ConversationDto> (document->object (), &error);

            if (!dto.has_value ()) {
                qWarning() << "Conversation mapping error: "<<error;
                return {};
            }
            QList<domain::entity::ConversationList> conversations = dto->toDomain ();
            qDebug() << "Conversation response:"<<conversations.size ();
            return conversations;
        });
}

} // namespace repository
} // namespace data
