#ifndef FEATURES_CHAT_DATA_DTO_MESSAGE_PAYLOAD_DTO_HPP
#define FEATURES_CHAT_DATA_DTO_MESSAGE_PAYLOAD_DTO_HPP

#include <QString>
#include <QtTypes>
#include <QJsonObject>

#include "domain/entity/messag_payload.hpp"

namespace data {
namespace dto {

// enum class MessageType
// {
//     Text,
//     Voice,
//     Video,
//     Location,
//     Image,
//     File
// };

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

    static ContentDto fromJson(const QJsonObject &obj)
    {
        ContentDto c;
        c.type      = fromString (obj.value("type").toString());
        c.text      = obj.value("text").toString();
        c.media_url = obj.value("media_url").toString();
        c.fileSize  = obj.value("fileSize").toVariant().toLongLong();
        return c;
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

    static PayloadDto fromJson(const QJsonObject &obj)
    {
        PayloadDto p;
        p.message_id       = obj.value("message_id").toString();
        p.sender_id        = obj.value("sender_id").toString();
        p.receiver_id      = obj.value("receiver_id").toString();
        p.reply_message_id = obj.value("reply_message_id").toString();
        p.timestamp         = obj.value("timestamp").toString();
        p.content           = ContentDto::fromJson(obj.value("content").toObject());
        return p;
    }

    // struct -> string
    QString toJsonString() const
    {
        return QString::fromUtf8(
            QJsonDocument(toJson()).toJson(QJsonDocument::Compact));
    }

    // string -> struct
    static PayloadDto fromJsonString(const QString &jsonStr)
    {
        QJsonParseError err;
        const QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8(), &err);
        if (err.error != QJsonParseError::NoError
            || !doc.isObject()) {
            return PayloadDto{}; // consider std::optional<PayloadDto> instead
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

} // namespace dto
} // namespace data

#endif // FEATURES_CHAT_DATA_DTO_MESSAGE_PAYLOAD_DTO_HPP
