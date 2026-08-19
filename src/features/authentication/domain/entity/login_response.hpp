#ifndef FEATURES_AUTHENTICATION_DOMAIN_ENTITY_LOGIN_RESPONSE_HPP
#define FEATURES_AUTHENTICATION_DOMAIN_ENTITY_LOGIN_RESPONSE_HPP

#include <QString>

namespace domain {
namespace entity {

struct User
{
    QString id;
    QString username;
    QString display_name;
};

struct Xmpp
{
    QString jid;
    QString host;
    quint16 port;
};
struct LoginResponse
{
    QString access_token;
    QString refresh_token;
    User user;
    Xmpp xmpp;
};

} // namespace entity
} // namespace domain

#endif // FEATURES_AUTHENTICATION_DOMAIN_ENTITY_LOGIN_RESPONSE_HPP
