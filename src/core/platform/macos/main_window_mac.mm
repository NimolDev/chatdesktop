#include "main_window_mac.hpp"

#include <QWindow>
#include <AppKit/AppKit.h>

core::platform::macos::MainWindowMac::MainWindowMac()
{

}

core::platform::macos::MainWindowMac::~MainWindowMac()
{
    // MARK: - clear memory after unsed
}

void core::platform::macos::MainWindowMac::setup(QWindow *window)
{
    m_window = window;
    if (!window) {
        return;
    }

    // Qt reapplies these flags when entering and leaving full screen.
    // Keep title-bar transparency in Qt's state as well as the native window.
    window->setFlag(Qt::NoTitleBarBackgroundHint, true);

    NSWindow *nativeWindow = getCurrentWindow (window);
    if (!nativeWindow) {
        return;
    }


    nativeWindow.titleVisibility = NSWindowTitleVisible;
    nativeWindow.titlebarAppearsTransparent = YES;
}

void core::platform::macos::MainWindowMac::setTitleBarColor(QWindow *window, const QColor &color)
{
    if (!window) {
        return;
    }
    NSWindow *nativeWidnow = getCurrentWindow (window);
    if (!nativeWidnow) {
        return;
    }
    nativeWidnow.backgroundColor = [NSColor colorWithRed: color.redF ()
                                                   green: color.greenF ()
                                                    blue: color.blueF ()
                                                   alpha: color.alphaF ()];
}


void core::platform::macos::MainWindowMac::setWindowFillContent()
{
    if (!m_window) {
        return;
    }

    m_window->setFlags(m_window->flags() | Qt::ExpandedClientAreaHint
                       | Qt::NoTitleBarBackgroundHint);
    NSWindow *nativeWindow = getCurrentWindow(m_window);
    if (!nativeWindow) {
        return;
    }
    nativeWindow.titleVisibility = NSWindowTitleHidden;
    nativeWindow.titlebarAppearsTransparent = YES;
    nativeWindow.movableByWindowBackground = YES;
}

void core::platform::macos::MainWindowMac::pineWindow(bool pinned)
{
    if (!m_window) {
        return;
    }
    NSWindow *nativeWindow = getCurrentWindow(m_window);
    if (!nativeWindow) {
        return;
    }
    // Changing only the level preserves the title-bar and content styling.
    nativeWindow.level = pinned ? NSFloatingWindowLevel : NSNormalWindowLevel;

}
//MARK: -  PRIVATE API:
NSWindow *core::platform::macos::MainWindowMac::getCurrentWindow(QWindow *window)
{
    if (!window) {
        return NULL;
    }
    NSView *nativeView = reinterpret_cast<NSView *>(window->winId ());
    if (!nativeView) {
        return NULL;
    }
    return nativeView.window;
}

