#ifndef FEATURES_CHAT_DATA_DTO_MESSAGE_PAYLOAD_DTO_HPP
#define FEATURES_CHAT_DATA_DTO_MESSAGE_PAYLOAD_DTO_HPP

#include <QString>
#include <QtTypes>
#include <QJsonObject>

#include "domain/entity/messag_payload.hpp"
// #include "page_response.hpp"
#include "network/json_serializer.hpp"

namespace data {
namespace dto {



using domain::entity::MessageType;

[[nodiscard]]
inline QString toString(MessageType type) {
    switch(type) {
    case MessageType::Text:
        return QStringLiteral ("text");
    case MessageType::Voice:
        return QStringLiteral ("voice");
    case MessageType::Video:
        return QStringLiteral ("video");
    case MessageType::Location:
        return QStringLiteral ("location");
    case MessageType::Image:
        return QStringLiteral ("image");
    case MessageType::File:
        return QStringLiteral ("file");
    }
    return QStringLiteral ("unkknown");
}
[[nodiscard]]
inline MessageType fromString(const QString &string)
{
    const QString value = string.trimmed().toLower();

    if (value == QStringLiteral("text"))
        return MessageType::Text;

    if (value == QStringLiteral("voice"))
        return MessageType::Voice;

    if (value == QStringLiteral("video"))
        return MessageType::Video;

    if (value == QStringLiteral("location"))
        return MessageType::Location;

    if (value == QStringLiteral("image"))
        return MessageType::Image;

    if (value == QStringLiteral("file"))
        return MessageType::File;

    return MessageType::Text; // fallback
}

struct ContentDto
{
    MessageType type;
    QString text;
    QString media_url;
    qint64 fileSize = 0;

    QJsonObject toJson() const
    {
        QJsonObject obj;
        obj["type"] = toString(type);
        obj["text"] = text;
        obj["media_url"] = media_url;
        obj["fileSize"] = fileSize;
        return obj;
    }

    static ContentDto fromJson(const QJsonObject &obj, QString *error_message = nullptr)
    {
        ContentDto c;
        c.type      = fromString (obj.value("type").toString());
        c.text      = obj.value("text").toString();
        c.media_url = obj.value("media_url").toString();
        c.fileSize  = obj.value("fileSize").toVariant().toLongLong();
        return c;
    }

    static std::optional<ContentDto> fromJsonString(const QString &string) {
        QJsonParseError error;
        const QJsonDocument doc = QJsonDocument::fromJson (string.toUtf8 (), &error);
        if (error.error != QJsonParseError::NoError
            || !doc.isObject ()) {
            return std::nullopt;
        }
        return fromJson (doc.object ());

    }

    [[nodiscard]] domain::entity::Content toDomain() const
    {
        return {
            .type = type,
            .text = text,
            .media_url = media_url,
            .file_size = fileSize
        };
    }

};
struct PayloadDto
{
    QString message_id;
    QString sender_id;
    QString receiver_id;
    QString reply_message_id;
    QString timestamp;
    ContentDto content;


    QJsonObject toJson() const
    {
        QJsonObject obj;
        obj["message_id"] = message_id;
        obj["sender_id"] = sender_id;
        obj["receiver_id"] = receiver_id;
        obj["reply_message_id"] = reply_message_id;
        obj["timestamp"] = timestamp;
        obj["content"] = content.toJson();
        return obj;
    }

    static  PayloadDto fromJson(const QJsonObject &obj, QString *error_message = nullptr)
    {
        PayloadDto p;

        p.message_id       = obj.value("message_id").toString();
        p.sender_id        = obj.value("sender_id").toString();
        p.receiver_id      = obj.value("receiver_id").toString();
        p.reply_message_id = obj.value("reply_message_id").toString();
        p.timestamp         = obj.value("timestamp").toString();
        p.content           = ContentDto::fromJson(
            obj.value(QStringLiteral("content")).toObject(),
            error_message);
        return p;
    }

    // struct -> string
    QString toJsonString() const
    {
        return QString::fromUtf8(
            QJsonDocument(toJson()).toJson(QJsonDocument::Compact));
    }

    // string -> struct
    static std::optional <PayloadDto> fromJsonString(const QString &jsonStr, QString *error_message = nullptr)
    {
        QJsonParseError err;
        const QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8(), &err);
        if (err.error != QJsonParseError::NoError
            || !doc.isObject()) {
            if (error_message != nullptr) {
                *error_message = "JSON Parese from Json string error: ";
            }
            return std::nullopt; // consider std::optional<PayloadDto> instead
        }
        return fromJson(doc.object());
    }

    [[nodiscard]] domain::entity::Payload toDomain() const
    {
        return {
            .message_id = message_id,
            .sender_id = sender_id,
            .receiver_id = receiver_id,
            .reply_message_id = reply_message_id,
            .timestamp = timestamp,
            .content = content.toDomain()
        };
    }
};

struct MessageItemDto {
    QString id;
    QString sender_id ;
    QString recipient_id;
    PayloadDto body;
    QString sent_at;

    [[nodiscard]]
    static std::optional<MessageItemDto> fromJson(
        const QJsonObject &object,
        QString *error_message = nullptr
        ) {
        MessageItemDto item;

        if (!object.contains ("body")
            || !object.value ("body").isString ()) {
            if (error_message != nullptr) {
                *error_message = "Missing or invalide: [body]";
            }
            return std::nullopt;
        }
        QString error;
        std::optional<PayloadDto> body = PayloadDto::fromJsonString (object.value ("body").toString (), &error);
        if (!body.has_value ()) {
            if (error_message != nullptr) {
                *error_message = QStringLiteral ("Invalid [body]: %1").arg (error);
            }
            return std::nullopt;
        }
        item.id = object.value ("id").toString ();
        item.sender_id = object.value ("sender_id").toString ();
        item.recipient_id = object.value ("recipient_id").toString ();
        item.body = std::move (body.value ());
        item.sent_at = object.value ("sent_at").toString ();
        return item;
    }

    [[nodiscard]]
    domain::entity::MessageItem toDomain() {
        return {
            .id = id,
            .sender_id = sender_id,
            .recipient_id = recipient_id,
            .body = body.toDomain (),
            .sent_at = sent_at
        };
    }
};

struct MessageDto
{
    QList<MessageItemDto> messages;
    int page;
    int limit;
    int total;
    int total_pages;

    static std::optional<MessageDto> fromJson(
        const QJsonObject &object,
        QString *error_message = nullptr
        ) {
        if (!object.contains (QStringLiteral ("messages"))
            || !object.value (QStringLiteral ("messages")).isArray ()) {
            if (error_message != nullptr) {
                *error_message = QStringLiteral ("Missing or invalide: [messages]");
            }
            return std::nullopt;
        }

        QString error;
        auto messages = core::network::JsonSerializer::fromArray<MessageItemDto> (
            object.value (QStringLiteral ("messages")).toArray (),
            &error
            );

        if (!messages.has_value ()) {
            if (error_message != nullptr) {
                *error_message = QStringLiteral ("Invalide [messages]: %1").arg (error);
            }
            return std::nullopt;
        }

        MessageDto dto;
        dto.messages = std::move (messages.value ());
        dto.page = object.value ("page").toInt ();
        dto.limit = object.value ("limit").toInt ();
        dto.total = object.value ("total").toInt ();
        dto.total_pages = object.value ("total_pages").toInt ();
        return dto;
    }

    domain::entity::MessageResponse toDomain() {
        QList<domain::entity::MessageItem> msg;
        for (auto m : std::as_const (messages)) {
            msg.append (std::move (m.toDomain ()));
        }


        return {
            .messages = std::move (msg),
            .page = page,
            .limit = limit,
            .total = total,
            .total_pages = total_pages
        };
    }
};






























} // namespace dto
} // namespace data

#endif // FEATURES_CHAT_DATA_DTO_MESSAGE_PAYLOAD_DTO_HPP
