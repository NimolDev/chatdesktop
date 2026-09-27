#include "macos_tray_icon.hpp"

#include <QApplication>
#include <QFile>
#include <QWindow>

#import <AppKit/AppKit.h>
#import <objc/runtime.h>

namespace {
const void *chatAppTrayKey = &chatAppTrayKey;
} // namespace

class MacosTrayIcon::Private
{
public:
    QApplication *app = nullptr;
    QWindow *window = nullptr;
    NSStatusItem *statusItem = nil;

    NSMenu *menu = nil;
    NSMenuItem *openApp = nil;
    NSMenuItem *notificationItem = nil;
    NSImage *trayIcon = nil;

    id applicationDelegate = nil;
    Class originalDelegateClass = Nil;
    Class reopenDelegateClass = Nil;

    NSMenu *dockMenu = nil;
    NSMenuItem *dockNotification = nil;

    void restoreWindow()
    {
        window->show();
        window->raise();
        window->requestActivate();
        [NSApp activateIgnoringOtherApps:YES];
    }
    void minimizeToTray() {
        window->hide ();
    }
};

namespace {

BOOL handleApplicationReopen(id delegate,
                             SEL,
                             NSApplication *,
                             BOOL)
{
    NSValue *trayValue = objc_getAssociatedObject(delegate, chatAppTrayKey);
    auto *tray = static_cast<MacosTrayIcon::Private *>(trayValue.pointerValue);
    if (tray) {
        tray->restoreWindow();
    }

    // The hidden ChatApp window has already been restored, so AppKit does not
    // need to create or look for another window.
    return NO;
}

/**
 * @brief handleApplicationDockMenu
 * @param delegate
 * @note This function is use for create dock menu
 * @warning Should be call this function in appdeledate
 * @return
 */
NSMenu *handleApplicationDockMenu (
    id delegate,
    SEL,
    NSApplication *
    ) {

    NSValue *trayValue = objc_getAssociatedObject (delegate, chatAppTrayKey);
    auto *tray = static_cast<MacosTrayIcon::Private *>(trayValue.pointerValue);
    return tray ? tray->dockMenu : nil;
}

} // namespace


@interface ChatAppTrayTarget : NSObject
@property(nonatomic, assign) MacosTrayIcon::Private *tray;
- (void)trayClicked:(id)sender;
- (void)openApp:(id)sender;
- (void)toggleNotifications:(id)sender;
- (void)quitApp:(id)sender;
@end

@implementation ChatAppTrayTarget
- (void)trayClicked:(id)sender
{
    NSEvent *event = NSApp.currentEvent;
    if (event.type == NSEventTypeRightMouseUp) {
        self.tray->statusItem.menu = self.tray->menu;
        [self.tray->statusItem.button performClick:nil];
        self.tray->statusItem.menu = nil;
    } else {

        if (self.tray->window->isActive ()) {
            NSLog (@"Window is already active");
            [self.tray->openApp setTitle: NSLocalizedString(@"Minimize to Tray ", @"")];
            // [self.tray->statusItem.button sendActionOn: (Ns)];

            self.tray->statusItem.menu = self.tray->menu;
            [self.tray->statusItem.button performClick:nil];
            self.tray->statusItem.menu = nil;

        } else {
            NSLog (@"Window is in tray");
            [self.tray->openApp setTitle: NSLocalizedString (@"Open ChatApp", @"")];
            self.tray->restoreWindow();
        }
    }
}

- (void)openApp:(id)sender
{
    if (self.tray->window->isActive ()) {
        self.tray->window->hide ();
        [self.tray->openApp setTitle: NSLocalizedString (@"Open ChatApp", @"")];
    } else {
        self.tray->restoreWindow();
    }
}

- (void)toggleNotifications:(id)sender
{
    bool disabled = [self.tray->notificationItem.title isEqualToString:@"Disable Notification"];
    NSString *notification_text ;
    if (disabled) {
        notification_text = NSLocalizedString (@"Enable Notification", @"Alert Notification");
    } else {
        notification_text = NSLocalizedString (@"Disable Notification", @"Alert Notification");
    }

    [self.tray->dockNotification setTitle: notification_text];
    [self.tray->notificationItem setTitle: notification_text];
}

- (void)quitApp:(id)sender
{
    self.tray->app->quit();
}
@end

MacosTrayIcon::MacosTrayIcon(QApplication *app, QWindow *window)
    : d(new Private)
{
    d->app = app;
    d->window = window;

    // Qt owns NSApp.delegate, so do not replace it. Give that one delegate a
    // runtime subclass which adds the standard macOS Dock-reopen callback and
    // inherits every other delegate method from Qt unchanged.
    d->applicationDelegate = NSApp.delegate;
    if (d->applicationDelegate) {
        d->originalDelegateClass = object_getClass(d->applicationDelegate);
        d->reopenDelegateClass = objc_allocateClassPair(
            d->originalDelegateClass,
            "ChatAppReopenApplicationDelegate",
            0);

        if (d->reopenDelegateClass) {
            const SEL reopenSelector =
                @selector(applicationShouldHandleReopen:hasVisibleWindows:);
            class_addMethod(d->reopenDelegateClass,
                            reopenSelector,
                            reinterpret_cast<IMP>(handleApplicationReopen),
                            "c@:@c");
            objc_registerClassPair(d->reopenDelegateClass);

            objc_setAssociatedObject(
                d->applicationDelegate,
                chatAppTrayKey,
                [NSValue valueWithPointer:d],
                OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            object_setClass(d->applicationDelegate, d->reopenDelegateClass);
             const SEL dockMenuSelector = @selector(applicationDockMenu:);
            class_addMethod (
                 d->reopenDelegateClass,
                 dockMenuSelector,
                 reinterpret_cast<IMP>(handleApplicationDockMenu),
                 "@@:@"
                 );
        }
    }



    ChatAppTrayTarget *target = [[ChatAppTrayTarget alloc] init];
    target.tray = d;

    // d->statusItem = [NSStatusBar.systemStatusBar
    //     statusItemWithLength:NSSquareStatusItemLength];
    d->statusItem = [NSStatusBar.systemStatusBar statusItemWithLength: NSVariableStatusItemLength];

    // set tray icon on status bar ---
    QFile iconFile(QStringLiteral(":/images/1024.png"));
    if (iconFile.open(QIODevice::ReadOnly)) {
        const QByteArray bytes = iconFile.readAll();
        NSData *data = [NSData dataWithBytes:bytes.constData()
                                      length:bytes.size()];
        d->trayIcon = [[NSImage alloc] initWithData:data];
        d->trayIcon.size = NSMakeSize(18.0, 18.0);
        d->statusItem.button.image =  d->trayIcon;
        [ d->trayIcon release];
    }


    d->statusItem.button.toolTip = NSLocalizedString(@"ChatApp", @"Tray icon tooltip");

    d->statusItem.button.target = target;
    d->statusItem.button.action = @selector(trayClicked:);
    [d->statusItem.button sendActionOn:
        NSEventMaskLeftMouseUp | NSEventMaskRightMouseUp];

    d->menu = [[NSMenu alloc] initWithTitle: NSLocalizedString (@"ChatApp", @"App Context Menu")];
    d->openApp = [[NSMenuItem alloc]
        initWithTitle: NSLocalizedString (@"Open ChatApp", NULL)
        action:@selector(openApp:)
        keyEquivalent:@""];
    d->openApp.target = target;
    [d->menu addItem:d->openApp];

    d->notificationItem = [[NSMenuItem alloc]
        initWithTitle: NSLocalizedString (@"Disable Notification", NULL)
        action:@selector(toggleNotifications:)
        keyEquivalent:@""];
    d->notificationItem.target = target;
    [d->menu addItem:d->notificationItem];
    [d->menu addItem:NSMenuItem.separatorItem];

    NSMenuItem *quitItem = [[NSMenuItem alloc]
        initWithTitle: NSLocalizedString (@"Quit ChatApp", NULL)
               action:@selector(quitApp:)
        keyEquivalent:@""];
    quitItem.target = target;
    [d->menu addItem:quitItem];


    // -----> Dock Menu <-------
    d->dockMenu = [[NSMenu alloc] initWithTitle:NSLocalizedString (@"ChatApp Dock Menu", @"")];
    d->dockNotification = [[NSMenuItem alloc]
        initWithTitle:NSLocalizedString(@"Disable Notification", @"")
               action:@selector(toggleNotifications:)
        keyEquivalent:@""];

    d->dockNotification.target = target;
    [d->dockMenu addItem:d->dockNotification];
    [d->dockNotification release];

    objc_setAssociatedObject(d->statusItem.button,
                             "ChatAppTrayTarget",
                             target,
                             OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    [target release];
}

MacosTrayIcon::~MacosTrayIcon() {
    if (d->applicationDelegate && d->reopenDelegateClass
        && object_getClass(d->applicationDelegate) == d->reopenDelegateClass) {
        object_setClass(d->applicationDelegate, d->originalDelegateClass);
        objc_setAssociatedObject(d->applicationDelegate,
                                 chatAppTrayKey,
                                 nil,
                                 OBJC_ASSOCIATION_ASSIGN);
    }

    [NSStatusBar.systemStatusBar removeStatusItem:d->statusItem];
    delete d;
}

void MacosTrayIcon::setBadgeNumber(int count) {

    if (count <= 0) {
        d->statusItem.button.image = d->trayIcon;
        return;
    }
    NSString *label = count > 99 ? NSLocalizedString (@"99+", @"") : [NSString stringWithFormat: @"%d", count];
    const NSSize canvaSize = NSMakeSize (25.0, 18.0);
    NSImage *result = [[[NSImage alloc] initWithSize: canvaSize] autorelease];
    [result lockFocus];

    // Draw the normal status-bar icon.
    [d->trayIcon drawInRect:NSMakeRect (0, 0,18.0 ,18.0)
                    fromRect: NSZeroRect
                    operation: NSCompositingOperationSourceOver
                    fraction: 1.0];

    NSDictionary *attributes = @{
        NSFontAttributeName: [NSFont boldSystemFontOfSize: 8.0],
        NSForegroundColorAttributeName: NSColor.whiteColor
    };

    NSSize textSize = [label sizeWithAttributes: attributes];

    const CGFloat badgeHeight = 11.0;
    const CGFloat badgeWidth = MAX (13.0, textSize.width + 5.0);

    // Appklit coordinates start at the bottom-left.
    NSRect badgeRect = NSMakeRect (canvaSize.width - badgeWidth ,
                                  0.0,
                                  badgeWidth,
                                  badgeHeight);
    [NSColor.systemRedColor setFill];

    NSBezierPath *badge = [NSBezierPath bezierPathWithRoundedRect: badgeRect
                                                         xRadius: badgeHeight / 2.0
                                                         yRadius: badgeHeight / 2.0];
    [badge fill];
    NSPoint textPoint = NSMakePoint (
        NSMidX (badgeRect) - textSize.width / 2.0,
        NSMidY (badgeRect) - textSize.height / 2.0);

    [label drawAtPoint: textPoint withAttributes: attributes];
    [result unlockFocus];
    d->statusItem.button.image = result;


    NSApp.dockTile.badgeLabel = count > 0
                                    ? (count > 99 ? [NSString stringWithFormat: @"99+"] : [NSString stringWithFormat: @"%d",count])
                                    : nil;
}
