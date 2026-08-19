#ifndef FEATURES_AUTHENTICATION_DATA_DTO_LOCAL_USER_HPP
#define FEATURES_AUTHENTICATION_DATA_DTO_LOCAL_USER_HPP

#include <QString>

namespace data {
namespace dto {

struct LocalUser
{
    QString access_token;
    QString refresh_token;
    QString user_id;
    QString username;
    QString password;
    QString display_name;
    QString xmpp_jid;
    QString xmpp_host;
    quint16 xmpp_port = 0;
};

} // namespace dto
} // namespace data

#endif // FEATURES_AUTHENTICATION_DATA_DTO_LOCAL_USER_HPP
