#include "tray_mac.hpp"



#include <QApplication>
#include <QWindow>

#import <AppKit/NSMenu.h>
#import <AppKit/NSStatusItem.h>
#import <AppKit/NSStatusBar.h>
#import <AppKit/NSStatusBarButton.h>
#import <AppKit/NSImage.h>
#import <AppKit/NSFont.h>
#import <AppKit/NSAttributedString.h>
#import <AppKit/NSColor.h>
#import <AppKit/AppKit.h>
#import <objc/runtime.h>


// MARK: - TrayAction handler
@interface TrayActionHandler: NSObject
@property (nonatomic, copy) void (^openHandler)(void);
@property (nonatomic, copy) void (^closeHandler)(void);
@property (nonatomic, copy) void (^trayIconHandler)(void);
@property(nonatomic, assign)core::platform::macos::TrayMac *tray;

- (void) trayIconClicked:(id) sender;
- (void) openApp:(id) sender;
- (void) quitApp:(id) sender;
- (void) notification:(id) sender;

@end

@implementation TrayActionHandler
- (void)dealloc {
    self.openHandler = nil;
    self.closeHandler = nil;
    self.trayIconHandler = nil;
#if !__has_feature(objc_arc)
    [super dealloc];
#endif
}

- (void)openApp:(id)sender {
    self.tray->hideToTrayRequest ();
    if (self.openHandler) {
        self.openHandler();
    }
}

- (void)quitApp:(id)sender {
    if (self.closeHandler) {
        self.closeHandler();
    }
}

- (void)notification:(id)sender {
    self.tray->setEnableNotification (NO);
}

- (void)trayIconClicked:(id)sender
{
    self.tray->iconClicked ();
}
@end

// MARK: - Application Delegate
@interface ApplicationDelegate: NSObject<NSApplicationDelegate>
@property(nonatomic, copy) void (^reopenHandler)(void);
@property (nonatomic, retain) NSMenu *dockMenu;
@end

@implementation ApplicationDelegate

- (BOOL)applicationShouldHandleReopen:(NSApplication *)sender
                    hasVisibleWindows:(BOOL)flag
{
    NSLog(@"Dock icon clicked");
    if (self.reopenHandler) {
        self.reopenHandler();
    }
    return YES;
}
-  (NSMenu *)applicationDockMenu: (NSApplication *)sender
{
    return self.dockMenu;
}

- (void)dealloc
{
    self.reopenHandler = nil;
    self.dockMenu = nil;
#if !__has_feature(objc_arc)
    [super dealloc];
#endif
}
@end

// MARK: - Application Menu
@interface ApplicationMenu : NSObject
- (void) showAbout:(id)sender;
- (void) preference:(id)sender;
- (void) quite:(id)sender;
@end

@implementation ApplicationMenu

- (void)preference:(id)sender {
}

- (void)showAbout:(id)sender {
}
- (void)quite:(id)sender {
}

@end

// MARK: Private class for implement action
class core::platform::macos::TrayMac::Impl
{
public:
    NSStatusItem *statusItem = nil;
    NSMenu *menu = nil;
    TrayActionHandler *handler = nil;

    QWindow *window = nullptr;
    QApplication *application = nullptr;

    NSImage *icon = nil;
    NSMenuItem *openItem = nil;
    NSMenuItem *quitItem = nil;
    NSMenuItem *notificationItem = nil;

    NSMenu *dockMenu = nil;
    NSMenuItem *docKNotificationItem = nil;
    ApplicationDelegate *appDelegate = nil;

    NSMenu *appMenu = nil;

    std::function<void()> openCallback;
    std::function<void()> closeCallback;
    std::function<void()> trayIconCallback;

    bool isEnableNotification = true;
};

core::platform::macos::TrayMac::TrayMac() : d(std::make_unique<Impl> ())
{
}

core::platform::macos::TrayMac::~TrayMac()
{
    if (d->statusItem) {
        [[NSStatusBar systemStatusBar] removeStatusItem: d->statusItem];
        d->statusItem = nil;
    }
    d->menu = nil;
}

void core::platform::macos::TrayMac::setup(QWindow *window, QApplication *application)
{
    if (d->statusItem) {
        return;
    }

    d->window = window;
    d->application = application;
    d->handler = [[TrayActionHandler alloc] init];
    d->handler.tray = this;

    d->statusItem = [[NSStatusBar systemStatusBar] statusItemWithLength: NSSquareStatusItemLength];


    NSStatusBarButton *button = d->statusItem.button;
    if (!button) {
        return;
    }
    createIcon ();
    button.image = d->icon;

    button.target = d->handler;
    button.action = @selector(trayIconClicked:);
    [button sendActionOn:
                NSEventMaskLeftMouseUp |
                NSEventMaskRightMouseUp];
    showTrayIcon ();
}

void core::platform::macos::TrayMac::aboutToShowRequests()
{
    NSEvent *event = [NSApp currentEvent];

    if (event.type == NSEventTypeRightMouseUp) {
        if (!d->statusItem.button || !d->menu) {
            return;
        }
        // Attaching the menu during a click does not open it for that same event.
        d->statusItem.menu = d->menu;
        [d->statusItem.button performClick: nil];
        // Resume custom click handling after native menu tracking finishes.
        d->statusItem.menu = nil;
        return;
    } else {
        if (d->window->isActive ()) {
            if (!d->statusItem.button || !d->menu) {
                return;
            }
            // Attaching the menu during a click does not open it for that same event.
            d->statusItem.menu = d->menu;
            [d->statusItem.button performClick: nil];
            // Resume custom click handling after native menu tracking finishes.
            d->statusItem.menu = nil;
            return;
        } else {
            showFromTrayRequest ();
            return;
        }
    }

}

void core::platform::macos::TrayMac::showFromTrayRequest()
{
    d->openItem.title = NSLocalizedString (@"Minimize to tray" , @"");
    d->window->show ();
    d->window->raise ();
    d->window->requestActivate ();
    [NSApp activateIgnoringOtherApps: YES];
}

void core::platform::macos::TrayMac::hideToTrayRequest()
{
    if(d->window->isActive ()) {
        d->openItem.title = NSLocalizedString (@"Open" , @"");
        d->window->hide ();
    } else {
        showFromTrayRequest ();
    }
}

void core::platform::macos::TrayMac::iconClicked()
{
    aboutToShowRequests ();
}

void core::platform::macos::TrayMac::showTrayIcon()
{
    if (!d->statusItem) {
        setup(d->window, d->application);
    }
    d->statusItem.visible = YES;

    d->menu = [[NSMenu alloc] initWithTitle: NSLocalizedString (@"ChatApp", @"App Context Menu")];
    d->openItem = [[NSMenuItem alloc] initWithTitle: NSLocalizedString (@"Minimize to tray", "")
                                             action: @selector(openApp:)
                                      keyEquivalent: @""];
    d->openItem.target = d->handler;
    [d->menu addItem: d->openItem];

    d->notificationItem = [[NSMenuItem alloc] initWithTitle: NSLocalizedString(@"Disable Notification", @"")
                                                     action: @selector(notification:)
                                              keyEquivalent: @""];
    d->notificationItem.target = d->handler;
    [d->menu addItem: d->notificationItem];

    addSeperate ();

    d->quitItem = [[NSMenuItem alloc] initWithTitle: NSLocalizedString (@"Quite App", @"")
                                             action: @selector(quitApp:)
                                      keyEquivalent: @""];
    d->quitItem.target = d->handler;
    [d->menu addItem: d->quitItem];

    d->dockMenu = [[NSMenu alloc] initWithTitle: NSLocalizedString (@"ChatApp", @"")];
    d->docKNotificationItem = [[NSMenuItem alloc] initWithTitle: NSLocalizedString (@"Disable Notification", @"")
                                                         action: @selector(notification:)
                                                  keyEquivalent: @""];
    d->docKNotificationItem.target = d->handler;
    [d->dockMenu addItem: d->docKNotificationItem];
    [d->docKNotificationItem release];

    d->appDelegate = [[ApplicationDelegate alloc] init];
    d->appDelegate.dockMenu = d->dockMenu;
    NSApp.delegate = d->appDelegate;

    d->appDelegate.reopenHandler = ^ {
        auto *impl = d.get ();
        if (!impl->window) {
            return;
        }
        this->showFromTrayRequest ();
    };

}

void core::platform::macos::TrayMac::setTooltip(const QString &tooltip)
{
    if (!d->statusItem.button) {
        return;
    }
    d->statusItem.button.toolTip = [NSString stringWithUTF8String: tooltip.toUtf8 ().constData ()];
}

void core::platform::macos::TrayMac::setOpenCallback(std::__1::function<void ()> callback)
{
    d->openCallback = std::move (callback);

    auto *impl = d.get ();
    d->handler.openHandler = ^ {
        if (impl->openCallback) {
            impl->openCallback();
        }
    };
}

void core::platform::macos::TrayMac::setQuitCallback(std::__1::function<void ()> callback)
{
    d->closeCallback = std::move (callback);
    auto impl = d.get ();
    d->handler.closeHandler = ^ {
        if (impl->closeCallback) {
            impl->closeCallback();
        }
    };
}

void core::platform::macos::TrayMac::setTrayClickCallback(std::function<void ()> callback)
{
    d->trayIconCallback = std::move (callback);
    auto impl = d.get ();
    d->handler.trayIconHandler  = ^{
        if (impl->trayIconCallback) {
            impl->trayIconCallback();
        }
    };
}

void core::platform::macos::TrayMac::createIcon()
{
    const QByteArray bytes = getTrayIconData ();
    NSData *data = [NSData dataWithBytes: bytes.constData ()
                                  length: bytes.size()];
    NSImage *image = [[NSImage alloc] initWithData: data];
    image.size = NSMakeSize (18.0, 18.0);
    d->icon = image;
}

void core::platform::macos::TrayMac::updateIcon()
{

    //TODO: Read unread notification from local cache notification.
    int count = 100;
    if (count <= 0) {
        d->statusItem.button.image = d->icon;
        return;
    }
    NSString *label = count > 99 ? NSLocalizedString (@"99+", @"") : [NSString stringWithFormat: @"%d", count];
    const NSSize canvaSize = NSMakeSize (18.0, 18.0);
    NSImage *result = [[[NSImage alloc] initWithSize: canvaSize] autorelease];
    [result lockFocus];

    // Draw the normal status-bar icon.

    [d->icon drawInRect:NSMakeRect (0, 0,18.0 ,20.0)
                   fromRect: NSZeroRect
                  operation: NSCompositingOperationSourceOver
                   fraction: 1.0];

    NSDictionary *attributes = @{
        NSFontAttributeName: [NSFont systemFontOfSize: 7.0],
        NSForegroundColorAttributeName: NSColor.whiteColor
    };

    NSSize textSize = [label sizeWithAttributes: attributes];

    const CGFloat badgeHeight = 11.0;
    const CGFloat badgeWidth = MAX (13.0, textSize.width + 5.0);

    // Appklit coordinates start at the bottom-left.
    NSRect badgeRect = NSMakeRect (0.0,
                                  -2.0,
                                  canvaSize.width,
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
                                    ? (count > 99 ? [NSString stringWithFormat: @"99+"]
                                                  : [NSString stringWithFormat: @"%d",count]
                                       )
                                    : nil;
}

bool core::platform::macos::TrayMac::hasIcon()
{
    return d->icon;
}
void core::platform::macos::TrayMac::detroyIcon()
{
    d->statusItem.button.image = nil;
}

void core::platform::macos::TrayMac::createMenu()
{

}

void core::platform::macos::TrayMac::destroyMenu()
{

}

void core::platform::macos::TrayMac::addAction()
{

}

void core::platform::macos::TrayMac::addSeperate()
{
    [d->menu addItem: NSMenuItem.separatorItem];
}


void core::platform::macos::TrayMac::setEnableNotification(bool enable)
{
    NSString *notification_title;
    if (d->isEnableNotification) {
        notification_title  = NSLocalizedString (@"Disable Notification", @"");
        d->isEnableNotification = NO;
    } else {
        notification_title = NSLocalizedString (@"Enable Notificaiton" , @"");
        d->isEnableNotification = YES;
    }
    [d->docKNotificationItem setTitle: notification_title];
    [d->notificationItem setTitle: notification_title];

}




