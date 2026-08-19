#ifndef FEATURES_AUTHENTICATION_DATA_DTO_LOGIN_REQUEST_DTO_HPP
#define FEATURES_AUTHENTICATION_DATA_DTO_LOGIN_REQUEST_DTO_HPP

#include <QString>
// #include "network/json_serializer.hpp"
#include <QJsonObject>

namespace data {
namespace dto {

struct LoginRequest
{
    QString username;
    QString password;

    QJsonObject toJson() const {
        QJsonObject object;
        object.insert (QStringLiteral ("username"), username);
        object.insert (QStringLiteral ("password"), password);
        return object;
    }

};

} // namespace dto
} // namespace data
#endif // FEATURES_AUTHENTICATION_DATA_DTO_LOGIN_REQUEST_DTO_HPP
