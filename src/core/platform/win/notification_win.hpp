#pragma once

#include "../notification.hpp"

namespace core {
namespace platform {
namespace win {

// Uses the application's TrayWin icon to deliver native Windows notifications.
class NotificationWin final : public Notification
{
public:
    void requestPermission() override;
    void show(const QString &title, const QString &message) override;
};

} // namespace win
} // namespace platform
} // namespace core
