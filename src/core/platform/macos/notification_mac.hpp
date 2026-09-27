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

    // NotificationMac(const NotificationMac &) = delete;
    // NotificationMac &operator=(const NotificationMac &) = delete;
    // NotificationMac(NotificationMac &&) = delete;
    // NotificationMac &operator=(NotificationMac &&) = delete;

    void show(const QString &title, const QString &message) override;
    void requestPermission() override;

// private:
//     NotificationMac();
//     ~NotificationMac() override = default;
};

} // namespace macos
} // namespace platform
} // namespace core

#endif // APP_PLATFORM_MACOS_NOTIFICATION_HPP
