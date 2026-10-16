#include "data/mapping/message_payload_mapper.hpp"
#include <QtGlobal>

#define CHECK(condition) do { if (!(condition)) { qFatal("Check failed: " #condition); } } while (false)
int main()
{
    for (const auto type : {domain::entity::MessageType::Text, domain::entity::MessageType::Image,
                            domain::entity::MessageType::File, domain::entity::MessageType::Voice}) {
        domain::entity::Payload payload;
        payload.content = {.type = type, .text = "Caption", .media_url = "https://example.com/media",
                           .file_name = "My File", .file_size = 123, .duration = 2345,
                           .localpath = "/private/local-file"};
        const auto dto = data::mapping::toDto(payload);
        data::dto::MessageItemDto message;
        message.id = "cached-message";
        message.sender_id = "sender";
        message.recipient_id = "recipient";
        message.body = dto;
        data::dto::MessageDto response{{message}, 2, 200, 250, 2};
        auto responseDocument = core::network::JsonSerializer::parse(QJsonDocument(response.toJson()).toJson());
        CHECK(responseDocument && responseDocument->isObject());
        auto decodedResponse = data::dto::MessageDto::fromJson(responseDocument->object());
        CHECK(decodedResponse && decodedResponse->messages.size() == 1);
        CHECK(decodedResponse->messages.first().id == message.id);
        auto domainResponse = decodedResponse->toDomain();
        CHECK(domainResponse.page == 2 && domainResponse.limit == 200);
        CHECK(domainResponse.total == 250 && domainResponse.total_pages == 2);
        response.messages.clear();
        auto emptyResponse = data::dto::MessageDto::fromJson(response.toJson());
        CHECK(emptyResponse && emptyResponse->messages.isEmpty());
        CHECK(emptyResponse->page == 2 && emptyResponse->total == 250);
        auto object = message.toJson();
        const auto document = core::network::JsonSerializer::parse(
            QJsonDocument(QJsonArray{object}).toJson());
        CHECK(document && document->isArray());
        auto cached = core::network::JsonSerializer::fromArray<data::dto::MessageItemDto>(document->array());
        CHECK(cached && cached->size() == 1);
        CHECK(cached->first().id == message.id);
        CHECK(cached->first().body.content.text == "Caption");
        CHECK(cached->first().toDomain().body.content.type == type);
        object["body"] = dto.toJsonString();
        CHECK(data::dto::MessageItemDto::fromJson(object).has_value());
        object["body"] = 123;
        CHECK(!data::dto::MessageItemDto::fromJson(object));
        CHECK(dto.toJson()["content"].isObject());
        CHECK(!dto.content.toJson().contains("localpath"));
        const auto parsed = data::dto::PayloadDto::fromJsonString(dto.toJsonString());
        CHECK(parsed.has_value());
        const auto restored = parsed->toDomain();
        CHECK(restored.content.type == type);
        CHECK(restored.content.text == "Caption");
        CHECK(restored.content.file_size == 123);
        CHECK(restored.content.file_name == "My File");
        if (type == domain::entity::MessageType::Voice) {
            CHECK(restored.content.duration == 2345);
        }
        auto legacy = dto.toJson();
        legacy["content"] = QJsonArray{dto.content.toJson()};
        CHECK(data::dto::PayloadDto::fromJson(legacy).toDomain().content.type == type);
        legacy["content"] = QJsonArray{dto.content.toJson(), dto.content.toJson()};
        CHECK(!data::dto::PayloadDto::fromJsonString(QString::fromUtf8(QJsonDocument(legacy).toJson())));
    }
}
