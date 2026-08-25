#ifndef FEATURES_CHAT_DOMAIN_ENTITY_MESSAGS_HPP
#define FEATURES_CHAT_DOMAIN_ENTITY_MESSAGS_HPP

#include <QString>
#include <QList>

namespace domain {
namespace entity {

struct Item
{
    bool is_mine = false;
    QString id;
    QString sender_id;
    QString recipient_id;
    QString body;
    QString sent_at;

};

struct Message
{
    QList<Item> messages;
};

} // namespace entity
} // namespace domain

#endif // FEATURES_CHAT_DOMAIN_ENTITY_MESSAGS_HPP
