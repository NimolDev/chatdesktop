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
    static QString conversations();
    static QString messages(const QString &user_id);

};

} // namespace constants
} // namespace core

#endif // CORE_CONSTANTS_APP_STRINGS_HPP
