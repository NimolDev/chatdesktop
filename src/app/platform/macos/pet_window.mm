#include "pet_window.hpp"

#import <AppKit/AppKit.h>

#include <QWindow>

namespace platform::macos {

void configurePetWindow(QWindow *window)
{
    if (!window) {
        return;
    }

    // On macOS Qt's WId is the NSView hosted by the native NSWindow. Calling
    // winId() also ensures that the native resources have been created.
    NSView *nativeView = reinterpret_cast<NSView *>(window->winId());
    NSWindow *nativeWindow = nativeView.window;
    if (!nativeWindow) {
        return;
    }

    nativeWindow.level = NSFloatingWindowLevel;
    nativeWindow.hidesOnDeactivate = NO;
    // Qt tool windows may carry MoveToActiveSpace by default. AppKit treats
    // that flag and CanJoinAllSpaces as mutually exclusive and raises an
    // NSInternalInconsistencyException if both are assigned.
    NSWindowCollectionBehavior behavior = nativeWindow.collectionBehavior;
    behavior &= ~NSWindowCollectionBehaviorMoveToActiveSpace;
    behavior |= NSWindowCollectionBehaviorCanJoinAllSpaces
              | NSWindowCollectionBehaviorFullScreenAuxiliary;
    nativeWindow.collectionBehavior = behavior;
}

} // namespace platform::macos
