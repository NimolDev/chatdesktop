#pragma once

#include <QtGlobal>

#ifdef Q_OS_WIN
#include "win/notification_win.hpp"
#elif defined(Q_OS_MACOS)
#include "macos/notification_mac.hpp"
#endif

namespace core {
namespace platform {

#ifdef Q_OS_WIN
using PlaformNotification = win::NotificationWin;
#elif defined(Q_OS_MACOS)
using PlaformNotification = macos::NotificationMac;
#endif

} // namespace platform
} // namespace core
