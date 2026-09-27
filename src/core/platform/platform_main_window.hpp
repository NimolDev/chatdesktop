#pragma once

#include <QtGlobal>

#ifdef Q_OS_WIN
#include "win/main_window_win.hpp"
#elif defined(Q_OS_MACOS)
#include "macos/main_window_mac.hpp"
#endif

namespace core {
namespace platform {

#ifdef Q_OS_WIN
using PlatformMainWindow = win::MainWindowWin;
#elif defined(Q_OS_MACOS)
using PlatformMainWindow = macos::MainWindowMac;
#endif

} // namespace platform
} // namespace core
