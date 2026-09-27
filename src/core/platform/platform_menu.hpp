#pragma once

// Plaform dependent implementations.
#ifdef Q_OS_WIN
#elif defined(Q_OS_MAC)
#include "macos/application_menu_mac.hpp"
namespace core::platform {
using PlatformMenu = core::platform::macos::ApplicationMenuMac;
}
#endif