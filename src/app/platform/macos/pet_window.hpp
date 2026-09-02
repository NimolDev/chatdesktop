#ifndef APP_PLATFORM_MACOS_PET_WINDOW_HPP
#define APP_PLATFORM_MACOS_PET_WINDOW_HPP

class QWindow;

namespace platform::macos {

// Makes a Qt window a desktop companion: above normal application windows and
// visible on every macOS Space, including full-screen Spaces.
void configurePetWindow(QWindow *window);

} // namespace platform::macos

#endif // APP_PLATFORM_MACOS_PET_WINDOW_HPP
