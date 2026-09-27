#ifndef APP_PLATFORM_MACOS_NOTIFICATION_HPP
#define APP_PLATFORM_MACOS_NOTIFICATION_HPP

#include "../notification.hpp"
namespace platform {
namespace macos {

class Notification : public platform::Notification
{
public:
    static Notification &instance();

    Notification(const Notification &) = delete;
    Notification &operator=(const Notification &) = delete;
    Notification(Notification &&) = delete;
    Notification &operator=(Notification &&) = delete;

    void show(const QString &title, const QString &message) override;
    void requestPermission() override;

private:
    Notification();
    ~Notification() override = default;
};

} // namespace macos
} // namespace platform

#endif // APP_PLATFORM_MACOS_NOTIFICATION_HPP
