#ifndef CORE_APPLICATION_APP_INFO_HPP
#define CORE_APPLICATION_APP_INFO_HPP

#include <QString>

namespace core::application {

class AppInfo final
{
public:
    AppInfo() = delete;

    [[nodiscard]] static QString name();
    [[nodiscard]] static QString version();
    [[nodiscard]] static QString fullVersion();
    [[nodiscard]] static int versionMajor() noexcept;
    [[nodiscard]] static int versionMinor() noexcept;
    [[nodiscard]] static int versionPatch() noexcept;
    [[nodiscard]] static int buildNumber() noexcept;
    [[nodiscard]] static QString bundleIdentifier();
    [[nodiscard]] static QString organizationName();
    [[nodiscard]] static QString organizationDomain();
};

} // namespace core::application

#endif // CORE_APPLICATION_APP_INFO_HPP
