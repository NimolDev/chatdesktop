#ifndef FEATURES_AUTHENTICATION_DATA_LOCAL_LOGIN_RESPONSE_STORE_HPP
#define FEATURES_AUTHENTICATION_DATA_LOCAL_LOGIN_RESPONSE_STORE_HPP

#include "dto/local_user.hpp"
#include "dto/login_response_dto.hpp"

#include <optional>

namespace data {
namespace local {


class LoginResponseStore
{
public:
    [[nodiscard]] static bool save(
        const dto::LoginResponseDTO &response,
        const QString password);
    [[nodiscard]] static std::optional<dto::LocalUser> getLocalUser();
    static bool logout();
};

} // namespace local
} // namespace data

#endif // FEATURES_AUTHENTICATION_DATA_LOCAL_LOGIN_RESPONSE_STORE_HPP
