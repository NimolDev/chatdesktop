#ifndef FEATURES_AUTHENTICATION_DOMAIN_LOGIN_REQUEST_HPP
#define FEATURES_AUTHENTICATION_DOMAIN_LOGIN_REQUEST_HPP

#include <QString>

namespace domain {

struct LoginRequest {
    QString user_name;
    QString password;
};

} // namespace domain

#endif // FEATURES_AUTHENTICATION_DOMAIN_LOGIN_REQUEST_HPP
