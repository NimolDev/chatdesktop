#ifndef FEATURES_CHAT_DOMAIN_ENTITY_MESSAG_PAYLOAD_HPP
#define FEATURES_CHAT_DOMAIN_ENTITY_MESSAG_PAYLOAD_HPP

#include <QString>
#include <QtTypes>
#include <QList>

namespace domain {
namespace entity {

enum class MessageType
{
    Text,
    Voice,
    Video,
    Location,
    Image,
    File,
    Sticker
};

struct Content
{
    MessageType type = MessageType::Text;
    QString text;
    QString media_url; // media file url
    QString file_name;
    qint64 file_size = 0;
    qint64 duration = 0; // milliseconds
    QString localpath; // Device-only path; never included in the wire payload.
};

struct Payload
{
    bool is_mine = false;
    QString message_id;
    QString sender_id;
    QString receiver_id;
    QString reply_message_id;
    QString timestamp;
    Content content;
    QString attachment_batch_id;
    int attachment_index = -1;
};

struct MessageItem
{
    QString id;
    QString sender_id ;
    QString recipient_id;
    Payload body;
    QString sent_at;
    bool is_mine;
    bool sending;
    bool send_success;
};

struct MessageResponse
{
    QList<MessageItem> messages ;
    int page;
    int limit;
    int total;
    int total_pages;
};


} // namespace entity
} // namespace domain

#endif // FEATURES_CHAT_DOMAIN_ENTITY_MESSAG_PAYLOAD_HPP
