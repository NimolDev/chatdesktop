#ifndef FEATURES_CHAT_DATA_MAPPING_MESSAGE_PAYLOAD_MAPPER_HPP
#define FEATURES_CHAT_DATA_MAPPING_MESSAGE_PAYLOAD_MAPPER_HPP

#include "data/dto/message_payload_dto.hpp"
#include "domain/entity/messag_payload.hpp"

namespace data {
namespace mapping {

[[nodiscard]] inline dto::ContentDto toDto(const domain::entity::Content &content)
{
    return {
        .type = content.type,
        .text = content.text,
        .media_url = content.media_url,
        .fileSize = content.file_size
    };
}

[[nodiscard]] inline dto::PayloadDto toDto(const domain::entity::Payload &payload)
{
    return {
        .message_id = payload.message_id,
        .sender_id = payload.sender_id,
        .receiver_id = payload.receiver_id,
        .reply_message_id = payload.reply_message_id,
        .timestamp = payload.timestamp,
        .content = toDto(payload.content)
    };
}

} // namespace mapping
} // namespace data

#endif // FEATURES_CHAT_DATA_MAPPING_MESSAGE_PAYLOAD_MAPPER_HPP
