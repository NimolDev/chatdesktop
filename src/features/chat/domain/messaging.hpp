#ifndef FEATURES_CHAT_DOMAIN_MESSAGING_HPP
#define FEATURES_CHAT_DOMAIN_MESSAGING_HPP

#include <QString>

namespace domain {

struct Messaging
{
    QString id;
    QString name;
    QString sender_id;
    QString msg;
    QString date;
    QString msg_type;
};

} // namespace domain

#endif // FEATURES_CHAT_DOMAIN_MESSAGING_HPP
