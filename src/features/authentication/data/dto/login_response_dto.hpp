#ifndef FEATURES_AUTHENTICATION_DATA_DTO_LOGIN_RESPONSE_DTO_HPP
#define FEATURES_AUTHENTICATION_DATA_DTO_LOGIN_RESPONSE_DTO_HPP

#include <QString>
#include <optional>

#include "network/json_serializer.hpp"
#include "domain/entity/login_response.hpp"

namespace data {
namespace dto {

struct UserDTO
{
    QString id;
    QString username;
    QString display_name;

    static std::optional<UserDTO> fromJson(
        const QJsonObject &object,
        QString *error_message = nullptr
        ) {
        UserDTO user;
        user.id = object.value (QStringLiteral ("id")).toString ();
        user.username = object.value (QStringLiteral ("username")).toString ();
        user.display_name = object.value (QStringLiteral ("display_name")).toString ();
        return user;
    }

    QJsonObject toJson() const {
        QJsonObject object;
        object.insert (QStringLiteral ("id"), id);
        object.insert (QStringLiteral ("username"), username);
        object.insert (QStringLiteral ("display_name"), display_name);
        return object;
    }
    domain::entity::User toDomain() const {
        return {
            .id = id,
            .username = username,
            .display_name = display_name
        };
    }
};

struct XmppDTO
{
    QString jid;
    QString host;
    quint16 port;

    static std::optional<XmppDTO> fromJson(
        const QJsonObject &object,
        QString *error_message = nullptr
        ) {

        XmppDTO xmpp;
        xmpp.jid = object.value (QStringLiteral ("jid")).toString ();
        xmpp.host = object.value (QStringLiteral ("host")).toString ();
        xmpp.port = object.value (QStringLiteral ("port")).toInt ();
        return xmpp;
    }
    QJsonObject toJson() const {
        QJsonObject object;
        object.insert (QStringLiteral ("jid"), jid);
        object.insert (QStringLiteral ("host"), host);
        object.insert (QStringLiteral ("port"), port);
        return object;
    }

    domain::entity::Xmpp toDomain() const {
        return {
            .jid = jid,
            .host = host,
            .port = port
        };
    }

};
struct LoginResponseDTO
{
    QString access_token;
    QString refresh_token;
    UserDTO user;
    XmppDTO xmpp;


    static std::optional<LoginResponseDTO> fromJson(
        const QJsonObject &object,
        QString *error_message = nullptr
        ) {

        if (!object.contains (QStringLiteral ("user"))
            || !object.value (QStringLiteral ("user")).isObject()
            ) {
            if (error_message  != nullptr) {
                *error_message = QStringLiteral ("Missing or invalide: [User]");
            }
            return std::nullopt;
        }
        QString user_error;
        auto user = dto::UserDTO::fromJson (
            object.value (QStringLiteral ("user")).toObject (),
            &user_error
            );

        if (!user.has_value ()) {
            if (error_message != nullptr) {
                *error_message = QStringLiteral ("Invalide [user]: %1").arg (user_error);
            }
            return std::nullopt;
        }

        if (!object.contains (QStringLiteral ("xmpp"))
            || !object.value (QStringLiteral ("xmpp")).isObject ()
            ) {
            if (error_message != nullptr) {
                *error_message = QStringLiteral ("Missing or Invalide: [xmpp]");
            }
            return std::nullopt;
        }
        QString xmpp_error;
        auto xmpp = core::network::JsonSerializer::fromObject<XmppDTO> (
            object.value (QStringLiteral ("xmpp")).toObject (),
            &xmpp_error
            );
        if (!xmpp.has_value ()) {
            if (error_message != nullptr) {
                *error_message = QStringLiteral ("Invalide [xmpp]: %1").arg (xmpp_error);
            }
            return std::nullopt;
        }

        LoginResponseDTO response;
        response.access_token = object.value (QStringLiteral ("access_token")).toString ();
        response.refresh_token = object.value (QStringLiteral ("refresh_token")).toString ();
        response.user = std::move (user.value ());
        response.xmpp = std::move (xmpp.value ());
        return response;
    }

    domain::entity::LoginResponse toDomain() const {
        return {
            .access_token = access_token,
            .refresh_token = refresh_token,
            .user = user.toDomain (),
            .xmpp = xmpp.toDomain ()
        };
    }
};

} // namespace dto
} // namespace data

#endif // FEATURES_AUTHENTICATION_DATA_DTO_LOGIN_RESPONSE_DTO_HPP
