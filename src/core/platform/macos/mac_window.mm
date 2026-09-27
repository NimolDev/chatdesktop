#include "mac_window.hpp"

#include <QWindow>
#import <AppKit/AppKit.h>

void core::platform::macos::configureWindow(QWindow *window)
{
    if (!window) {
        return;
    }

    // force to create native window
    WId windId = window->winId ();

    NSView *view = (__bridge NSView *)reinterpret_cast<void *>(windId);

    if (!view) {
        return;
    }

    NSWindow *nsWindow = view.window;
    if (!nsWindow) {
        return;
    }

    nsWindow.titleVisibility = NSWindowTitleHidden;
    nsWindow.titlebarAppearsTransparent = YES;

    nsWindow.styleMask |= NSWindowStyleMaskFullSizeContentView;

    // Optional: gragging window by empty background area.
    nsWindow.movableByWindowBackground = YES;

}

void core::platform::macos::setWindowPinned(QWindow *window, bool pinned)
{
    if (!window)
        return;

    NSView *view = (__bridge NSView *)reinterpret_cast<void *>(window->winId());
    NSWindow *nativeWindow = view.window;
    if (!nativeWindow)
        return;

    // Changing the level preserves the native window and its title-bar style.
    // Do not use QWindow::setFlags here: Qt rewrites the macOS style mask.
    nativeWindow.level = pinned ? NSFloatingWindowLevel : NSNormalWindowLevel;
}

void core::platform::macos::pinWindow(QWindow *window)
{
    if (!window) {
        return;
    }

    NSView *nativeView = reinterpret_cast<NSView *>(window->winId ());
    NSWindow *nativeWindow = nativeView.window;
    if (!nativeWindow) {
        return;
    }

    nativeWindow.level = NSFloatingWindowLevel;
    nativeWindow.hidesOnDeactivate = NO;

    NSWindowCollectionBehavior behavior = nativeWindow.collectionBehavior;
    behavior &= ~NSWindowCollectionBehaviorMoveToActiveSpace;
    behavior |= NSWindowCollectionBehaviorCanJoinAllSpaces
                | NSWindowCollectionBehaviorFullScreenAuxiliary;
    nativeWindow.collectionBehavior = behavior;
}
