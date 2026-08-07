#ifndef FEATURES_AUTHENTICATION_DOMAIN_LOGIN_RESPONSE_HPP
#define FEATURES_AUTHENTICATION_DOMAIN_LOGIN_RESPONSE_HPP

#include <QString>

namespace domain {

struct LoginResponse
{
    QString user_name;
    QString user_jid;
};

} // namespace domain

#endif // FEATURES_AUTHENTICATION_DOMAIN_LOGIN_RESPONSE_HPP
