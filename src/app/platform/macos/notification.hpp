#ifndef APP_PLATFORM_MACOS_NOTIFICATION_HPP
#define APP_PLATFORM_MACOS_NOTIFICATION_HPP

#include "../notification.hpp"
namespace platform {
namespace macos {

class Notification : public platform::Notification
{
public:
    Notification();
    // Notification interface
public:
    void show(const QString &title, const QString &message) override;
    void requestPermission() override;
};

} // namespace macos
} // namespace platform

#endif // APP_PLATFORM_MACOS_NOTIFICATION_HPP
