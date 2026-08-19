#ifndef FEATURES_AUTHENTICATION_DOMAIN_LOGIN_REQUEST_HPP
#define FEATURES_AUTHENTICATION_DOMAIN_LOGIN_REQUEST_HPP

#include <QString>

namespace domain {
namespace entity {

struct LoginRequest {
    QString username;
    QString password;
};

} // namespace domain
} // namespace entity

#endif // FEATURES_AUTHENTICATION_DOMAIN_LOGIN_REQUEST_HPP
