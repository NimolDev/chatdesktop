#pragma once

#include <QtGlobal>

#ifdef Q_OS_WIN
#include "win/win_tray.hpp"
#elif defined(Q_OS_MACOS)
#include "macos/tray_mac.hpp"
#endif

namespace core {
namespace platform {

#ifdef Q_OS_WIN
using PlatformTray = win::TrayWin;
#elif defined(Q_OS_MACOS)
using PlatformTray = macos::TrayMac;
#endif

} // namespace platform
} // namespace core
