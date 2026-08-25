#ifndef FEATURES_AUTHENTICATION_DATA_DTO_REFRESH_TOKEN_RESPONSE_DTO_HPP
#define FEATURES_AUTHENTICATION_DATA_DTO_REFRESH_TOKEN_RESPONSE_DTO_HPP

#include <QString>
#include <QtCore/qjsonobject.h>

namespace data {
namespace dto {

struct TokenResponseDto
{
    QString access_token;
    QString refresh_token;

    static TokenResponseDto fromJson(
        const QJsonObject &object,
        QString *error_message = nullptr
        ) {
        TokenResponseDto response;
        response.access_token = object.value (QStringLiteral ("access_token")).toString ();
        response.refresh_token = object.value (QStringLiteral ("refresh_token")).toString ();
        return response;
    }
};

} // namespace dto
} // namespace data

#endif // FEATURES_AUTHENTICATION_DATA_DTO_REFRESH_TOKEN_RESPONSE_DTO_HPP
