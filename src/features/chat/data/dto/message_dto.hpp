#ifndef MESSAGE_DTO_H
#define MESSAGE_DTO_H

#include<QString>
#include<QList>
#include "network/json_serializer.hpp"
#include "domain/entity/messags.hpp"

namespace data {
namespace dto {

struct Item
{
    QString id;
    QString sender_id;
    QString recipient_id;
    QString body;
    QString sent_at;


   static std::optional<Item> fromJson(
        const QJsonObject &object,
        QString *error_message = nullptr
        ) {
        Item item;
        item.id = object.value (QStringLiteral ("id")).toString ();
        item.sender_id = object.value (QStringLiteral ("sender_id")).toString ();
        item.recipient_id = object.value (QStringLiteral ("recipient_id")).toString ();
        item.body = object.value (QStringLiteral ("body")).toString ();
        item.sent_at = object.value (QStringLiteral ("sent_at")).toString ();
        return item;
    }

    domain::entity::Item toDomain() {
        return {
            .id = id,
            .sender_id = sender_id,
            .recipient_id = recipient_id,
            .body = body,
            .sent_at = sent_at
        };
    }
};

struct Message
{
    QList<Item> messages;

    static std::optional<Message> fromJson(
        const QJsonObject &object,
        QString *error_message = nullptr
        ) {
        if (!object.contains (QStringLiteral ("messages"))
            || !object.value (QStringLiteral ("messages")).isArray ()) {
            if (error_message != nullptr) {
                *error_message = QStringLiteral ("Missing or Invalide [Messages]");
            }
            return std::nullopt;
        }
        QString error;
        auto message = core::network::JsonSerializer::fromArray<Item> (
            object.value (QStringLiteral ("messages")).toArray (),
            &error
            );
        if (!message.has_value ()) {
            if (error_message != nullptr) {
                *error_message = QStringLiteral ("Invalide [messages]: %1").arg (error);
            }
            return std::nullopt;
        }

        Message msg;
        msg.messages = std::move (message.value ());
        return msg;
    }

    domain::entity::Message toDomain() {
        domain::entity::Message msg;
        for (auto item: std::as_const(messages)) {
            msg.messages.append (item.toDomain());
        }
        return msg;
    }

};


} // namespace dto
} // namespace name

#endif // MESSAGE_DTO_H
