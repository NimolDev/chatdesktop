#include "login_response_store.hpp"
#include "storage/auth_storage.hpp"

namespace data {
namespace local {
namespace {

std::optional<dto::LocalUser> decodeSession(const QJsonObject &object)
{
    // Validate before the DTO mapper, which otherwise defaults missing fields.
    const auto user = object.value(QStringLiteral("user")).toObject();
    const auto xmpp = object.value(QStringLiteral("xmpp")).toObject();
    for (const auto *key : {"access_token", "refresh_token", "password"}) {
        if (!object.value(QLatin1String(key)).isString()) {
            return std::nullopt;
        }
    }
    for (const auto *key : {"id", "username", "display_name"}) {
        if (!user.value(QLatin1String(key)).isString()) {
            return std::nullopt;
        }
    }
    const auto port = xmpp.value(QStringLiteral("port"));
    const int portNumber = port.toInt(-1);
    if (object.value(QStringLiteral("version")).toInt() != 1 ||
        !xmpp.value(QStringLiteral("jid")).isString() ||
        !xmpp.value(QStringLiteral("host")).isString() ||
        !port.isDouble() || portNumber < 0 || portNumber > 65535 ||
        port.toDouble() != portNumber) {
        return std::nullopt;
    }
    const auto response = dto::LoginResponseDTO::fromJson(object);
    if (!response) {
        return std::nullopt;
    }
    return dto::LocalUser {
        .access_token = response->access_token,
        .refresh_token = response->refresh_token,
        .user_id = response->user.id,
        .username = response->user.username,
        .password = object.value(QStringLiteral("password")).toString(),
        .display_name = response->user.display_name,
        .xmpp_jid = response->xmpp.jid,
        .xmpp_host = response->xmpp.host,
        .xmpp_port = response->xmpp.port
    };
}

} // namespace

bool LoginResponseStore::save(const dto::LoginResponseDTO &response, const QString password)
{
    auto object = response.toJson();
    object.insert(QStringLiteral("version"), 1);
    object.insert(QStringLiteral("password"), password);
    return core::storage::AuthStorage::save(object);
}

std::optional<dto::LocalUser> LoginResponseStore::getLocalUser()
{
    const auto object = core::storage::AuthStorage::read();
    return object ? decodeSession(*object) : std::nullopt;
}

bool LoginResponseStore::logout()
{
    return core::storage::AuthStorage::clear();
}

bool LoginResponseStore::updateToken(dto::TokenResponseDto &token)
{
    return core::storage::AuthStorage::update([&token](QJsonObject &object) {
        if (!decodeSession(object)) {
            return false;
        }
        object.insert(QStringLiteral("access_token"), token.access_token);
        object.insert(QStringLiteral("refresh_token"), token.refresh_token);
        return true;
    });
}

} // namespace local
} // namespace data
