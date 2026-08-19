#ifndef FEATURES_CHAT_DATA_DTO_CONVERSATIONS_DTO_HPP
#define FEATURES_CHAT_DATA_DTO_CONVERSATIONS_DTO_HPP

#include <QString>
#include <QList>
#include <QJsonObject>

#include "entity/conversation_list.hpp"
#include "network/json_serializer.hpp"

namespace data {
namespace dto {

struct User
{
    QString uuid;
    QString name;

    static std::optional <User> fromJson(
       const QJsonObject &object,
        QString *error_message = nullptr
        ) {

       User dto;
       dto.uuid = object.value (QStringLiteral ("uuid")).toString ();
       dto.name = object.value (QStringLiteral ("name")).toString ();
       return dto;
    }


};

struct ConversationDto
{
    QList<User> users;

    static std::optional<ConversationDto> fromJson(
        const QJsonObject &object,
        QString *error_message = nullptr
        ) {
        if (!object.contains (QStringLiteral ("users"))
            || !object.value (QStringLiteral ("users")).isArray ()) {
            if (error_message != nullptr) {
            *error_message = QStringLiteral ("Missing or Invalide [Users]");
            }
            return std::nullopt;
        }
        QString error;
        auto users = core::network::JsonSerializer::fromArray<User> (
            object.value (QStringLiteral ("users")).toArray (),
            &error
            );
        if (!users.has_value ()) {
            if (error_message != nullptr) {
            *error_message = QStringLiteral ("Invalid [Group]: %1").arg (error);
            }
            return std::nullopt;
        }

        ConversationDto conversation;
        conversation.users = std::move (users.value ());
        return conversation;
    }

    QList<domain::entity::ConversationList> toDomain() {
        QList<domain::entity::ConversationList> result;
        result.reserve (users.size ());
        for (const auto &user: std::as_const(users)) {
            domain::entity::ConversationList con;
            con.name = user.name;
            con.uuid = user.uuid;
            result.append (con);
        }
        return result;
    }
};

} // namespace dto
} // namespace data
#endif // FEATURES_CHAT_DATA_DTO_CONVERSATIONS_DTO_HPP
