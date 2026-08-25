#ifndef CORE_CONSTANTS_APP_STRINGS_HPP
#define CORE_CONSTANTS_APP_STRINGS_HPP


#include <QString>

namespace core {
namespace constants {

class AppConstants final
{
public:
    AppConstants() = delete;

    [[nodiscard]]
    static QString baseUrl();

    [[nodiscard]]
    static QString login();
    [[nodiscard]] static QString refreshToken();
    [[nodiscard]] static QString conversations();
    [[nodiscard]] static QString messages(const QString &user_id);
    [[nodiscard]]static QString group();
    [[nodiscard]]static QString groupMessage(const QString group_id);
    [[nodiscard]] static QString saveMessages();


};

} // namespace constants
} // namespace core

#endif // CORE_CONSTANTS_APP_STRINGS_HPP
