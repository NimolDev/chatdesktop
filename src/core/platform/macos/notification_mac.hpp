#ifndef APP_PLATFORM_MACOS_NOTIFICATION_HPP
#define APP_PLATFORM_MACOS_NOTIFICATION_HPP

#include "../notification.hpp"

namespace core {
namespace platform {
namespace macos {

class NotificationMac : public core::platform::Notification
{
public:
    // static NotificationMac &instance();
    explicit NotificationMac();
    ~NotificationMac();

    void show(const QString &title, const QString &message) override;
    void requestPermission() override;


};
} // namespace macos
} // namespace platform
} // namespace core

#endif // APP_PLATFORM_MACOS_NOTIFICATION_HPP
