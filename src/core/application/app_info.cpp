#include "application/app_info.hpp"

#include "app_version.hpp"

#include <string_view>

namespace {

[[nodiscard]] QString fromUtf8(std::string_view value)
{
    return QString::fromUtf8(value.data(), static_cast<qsizetype>(value.size()));
}

} // namespace

namespace core::application {

QString AppInfo::name()
{
    return fromUtf8(build::Name);
}

QString AppInfo::version()
{
    return fromUtf8(build::Version);
}

QString AppInfo::fullVersion()
{
    return QStringLiteral("%1 (%2)").arg(version()).arg(buildNumber());
}

int AppInfo::versionMajor() noexcept
{
    return build::VersionMajor;
}

int AppInfo::versionMinor() noexcept
{
    return build::VersionMinor;
}

int AppInfo::versionPatch() noexcept
{
    return build::VersionPatch;
}

int AppInfo::buildNumber() noexcept
{
    return build::BuildNumber;
}

QString AppInfo::bundleIdentifier()
{
    return fromUtf8(build::BundleIdentifier);
}

QString AppInfo::organizationName()
{
    return fromUtf8(build::OrganizationName);
}

QString AppInfo::organizationDomain()
{
    return fromUtf8(build::OrganizationDomain);
}

} // namespace core::application
