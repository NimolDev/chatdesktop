#ifndef FEATURES_CHAT_DOMAIN_ENTITY_MESSAG_PAYLOAD_HPP
#define FEATURES_CHAT_DOMAIN_ENTITY_MESSAG_PAYLOAD_HPP

#include <QString>
#include <QtTypes>

namespace domain {
namespace entity {

enum class MessageType
{
    Text,
    Voice,
    Video,
    Location,
    Image,
    File
};

struct Content
{
    MessageType type;
    QString text;
    QString media_url;
    qint64 file_size = 0;
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
};

} // namespace entity
} // namespace domain

#endif // FEATURES_CHAT_DOMAIN_ENTITY_MESSAG_PAYLOAD_HPP
