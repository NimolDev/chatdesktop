#ifndef CORE_STORAGE_DATABASE_HELPER_HPP
#define CORE_STORAGE_DATABASE_HELPER_HPP

#include <QString>
#include <optional>

namespace core {
namespace storage {

struct CurrentUser
{
    QString username;
    QString user_id;
};

class UserData
{
public:
    // Reads the shared encrypted auth store; no SQL connection is required.
    static std::optional<CurrentUser> getCurrentUser();
};

} // namespace storage
} // namespace core

#endif // CORE_STORAGE_DATABASE_HELPER_HPP
