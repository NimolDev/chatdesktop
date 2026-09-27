#ifndef CORE_PLATFORM_MACOS_MAC_WINDOW_HPP
#define CORE_PLATFORM_MACOS_MAC_WINDOW_HPP


class QWindow;

namespace core {
namespace platform {
namespace macos {


void configureWindow(QWindow *window);
void pinWindow(QWindow *window);
void setWindowPinned(QWindow *window, bool pinned);

} // namespace macos
} // namespace platform
} // namespace core


#endif // CORE_PLATFORM_MACOS_MAC_WINDOW_HPP
