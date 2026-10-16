#include "user_data.hpp"
#include "auth_storage.hpp"

namespace core {
namespace storage {

std::optional<CurrentUser> UserData::getCurrentUser()
{
    const auto session = AuthStorage::read();
    if (!session || session->value(QStringLiteral("version")).toInt() != 1) {
        return std::nullopt;
    }
    const auto user = session->value(QStringLiteral("user")).toObject();
    const auto username = user.value(QStringLiteral("username"));
    const auto id = user.value(QStringLiteral("id"));
    if (!username.isString() || !id.isString()) {
        return std::nullopt;
    }
    return CurrentUser{username.toString(), id.toString()};
}

} // namespace storage
} // namespace core
