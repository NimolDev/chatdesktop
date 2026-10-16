#pragma once

#include <QJsonObject>
#include <functional>
#include <optional>

namespace core {
namespace storage {

// Shared auth persistence for authentication, the app header, and offline chat.
// Snapshot name: hexName(username, Base64(UTF-8(userId + username))).
// A fixed lookup file stores only the active hex filename for startup.
class AuthStorage
{
public:
    static bool save(const QJsonObject &object);
    static std::optional<QJsonObject> read();
    static bool clear();
    // Runs under the store lock. The callback must not call AuthStorage again.
    static bool update(const std::function<bool(QJsonObject &)> &modify);
};

} // namespace storage
} // namespace core
