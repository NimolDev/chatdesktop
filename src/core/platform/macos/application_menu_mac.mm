#include "application_menu_mac.hpp"

#import <AppKit/AppKit.h>
#include <QtCore/qtpreprocessorsupport.h>

@interface  ApplicationMenuHandler : NSObject
@property (nonatomic, copy) void (^aboutHandler)(void);
@property (nonatomic, copy) void (^settingHandler)(void);
@property (nonatomic, copy) void (^quitHandler)(void);
@property (nonatomic, retain) NSPanel *quitPanel;
@property (nonatomic) BOOL quitKeysHeld;
@property (nonatomic, retain) NSTimer *quitTimer;
@property (nonatomic, retain) id quitEventMonitor;
@property (nonatomic) NSTimeInterval quitHoldStarted;
@property (nonatomic) unsigned short quitKeyCode;

- (void)didTapAbout:(id)sender;
- (void)didTapPreferences:(id)sender;
- (void)didTapQuit:(id)sender;
- (void)didTapLogout:(id)sender;
- (void)cancelQuitHold;
- (void)updateQuitHold:(NSTimer *)timer;
- (void)observeQuitEvent:(NSEvent *)event;
- (void)applicationResigned:(NSNotification *)notification;

@end

@implementation ApplicationMenuHandler

- (void)didTapAbout:(id)sender
{
    if (self.aboutHandler) {
        self.aboutHandler();
    }
}

- (void)didTapPreferences:(id)sender
{
    if (self.settingHandler) {
        self.settingHandler();
    }
}

- (void)didTapQuit:(id)sender
{
    NSEvent *event = NSApp.currentEvent;
    if (event.type == NSEventTypeKeyDown
        && (event.modifierFlags & NSEventModifierFlagCommand)
        && [event.charactersIgnoringModifiers.lowercaseString isEqualToString:@"q"]) {
        if (event.isARepeat) {
            return;
        }
        if (self.quitTimer) {
            const NSTimeInterval elapsed = NSProcessInfo.processInfo.systemUptime - self.quitHoldStarted;
            if (!self.quitKeysHeld && elapsed <= 1.0) {
                [self cancelQuitHold];
                if (self.quitHandler) {
                    self.quitHandler();
                }
                return;
            }
            if (self.quitKeysHeld) {
                return;
            }
            [self cancelQuitHold];
        }
        if (!self.quitPanel) {
            NSPanel *panel = [[NSPanel alloc] initWithContentRect:NSMakeRect(0, 0, 340, 108)
                styleMask:NSWindowStyleMaskBorderless | NSWindowStyleMaskNonactivatingPanel
                backing:NSBackingStoreBuffered defer:NO];
            panel.releasedWhenClosed = NO;
            panel.level = NSFloatingWindowLevel;
            panel.hasShadow = YES;
            panel.opaque = NO;
            panel.backgroundColor = NSColor.clearColor;
            panel.ignoresMouseEvents = YES;
            NSVisualEffectView *background = [[NSVisualEffectView alloc] initWithFrame:panel.contentView.bounds];
            background.material = NSVisualEffectMaterialHUDWindow;
            background.state = NSVisualEffectStateActive;
            background.wantsLayer = YES;
            background.layer.cornerRadius = 16;
            background.layer.masksToBounds = YES;
            panel.contentView = background;
            [background release];

            NSTextField *title = [NSTextField labelWithString:NSLocalizedString(@"Hold ⌘Q to quit", @"")];
            title.font = [NSFont boldSystemFontOfSize:20];
            title.alignment = NSTextAlignmentCenter;
            title.frame = NSMakeRect(20, 57, 300, 28);
            [panel.contentView addSubview:title];
            NSTextField *hint = [NSTextField labelWithString:NSLocalizedString(@"Or press ⌘Q again to quit", @"")];
            hint.textColor = NSColor.secondaryLabelColor;
            hint.alignment = NSTextAlignmentCenter;
            hint.frame = NSMakeRect(20, 29, 300, 20);
            [panel.contentView addSubview:hint];
            self.quitPanel = panel;
            [panel release];
        }
        NSWindow *window = NSApp.keyWindow;
        NSRect frame = window ? window.frame : NSScreen.mainScreen.visibleFrame;
        [self.quitPanel setFrameOrigin:NSMakePoint(NSMidX(frame) - 170, NSMidY(frame) - 54)];
        self.quitKeysHeld = YES;
        [self.quitPanel orderFront:nil];
        self.quitHoldStarted = NSProcessInfo.processInfo.systemUptime;
        self.quitKeyCode = event.keyCode;
        self.quitEventMonitor = [NSEvent addLocalMonitorForEventsMatchingMask:
            NSEventMaskKeyUp | NSEventMaskFlagsChanged | NSEventMaskKeyDown
            handler:^NSEvent *(NSEvent *nextEvent) {
                [self observeQuitEvent:nextEvent];
                return nextEvent;
            }];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(applicationResigned:)
            name:NSApplicationDidResignActiveNotification object:NSApp];
        self.quitTimer = [NSTimer timerWithTimeInterval:0.02 target:self
            selector:@selector(updateQuitHold:) userInfo:nil repeats:YES];
        [[NSRunLoop mainRunLoop] addTimer:self.quitTimer forMode:NSRunLoopCommonModes];
        return;
    }
    [self cancelQuitHold];
    if (self.quitHandler) {
        self.quitHandler();
    }
}

- (void)observeQuitEvent:(NSEvent *)event
{
    if ((event.type == NSEventTypeKeyUp && event.keyCode == self.quitKeyCode)
        || (event.type == NSEventTypeFlagsChanged
            && !(event.modifierFlags & NSEventModifierFlagCommand))) {
        // Keep the prompt briefly so a second press can confirm quitting.
        self.quitKeysHeld = NO;
    } else if (event.type == NSEventTypeKeyDown && event.keyCode != self.quitKeyCode) {
        [self cancelQuitHold];
    }
}

- (void)applicationResigned:(NSNotification *)notification
{
    [self cancelQuitHold];
}

- (void)updateQuitHold:(NSTimer *)timer
{
    if (!self.quitTimer) {
        return;
    }
    const NSTimeInterval elapsed = NSProcessInfo.processInfo.systemUptime - self.quitHoldStarted;
    if (elapsed >= 1.0) {
        const BOOL shouldQuit = self.quitKeysHeld;
        [self cancelQuitHold];
        if (shouldQuit && self.quitHandler) {
            self.quitHandler();
        }
    }
}

- (void)cancelQuitHold
{
    self.quitKeysHeld = NO;
    [self.quitTimer invalidate];
    self.quitTimer = nil;
    if (self.quitEventMonitor) {
        [NSEvent removeMonitor:self.quitEventMonitor];
        self.quitEventMonitor = nil;
    }
    [[NSNotificationCenter defaultCenter] removeObserver:self
        name:NSApplicationDidResignActiveNotification object:NSApp];
    [self.quitPanel orderOut:nil];
}

- (void)dealloc
{
    [self cancelQuitHold];
    self.quitPanel = nil;
    self.aboutHandler = nil;
    self.settingHandler = nil;
    self.quitHandler = nil;
    [super dealloc ];
}

- (void)didTapLogout:(id)sender {
}

@end


class core::platform::macos::ApplicationMenuMac::Impl
{
public:
    NSMenu *menuBar = nil;
    NSMenu *appMenu = nil;
    NSMenu *fileMenu = nil;
    NSMenu *editMenu = nil;
    NSMenu *windowMenu = nil;

    ApplicationMenuHandler *handler = nil;

    std::function<void()> aboutCallback;
    std::function<void()> settingCallback;
    std::function<void()> quitCallback;
};

core::platform::macos::ApplicationMenuMac::ApplicationMenuMac()
    : d(std::make_unique<Impl> ())
{}

core::platform::macos::ApplicationMenuMac::~ApplicationMenuMac()
{
    [d->handler cancelQuitHold];
    d->handler.quitHandler = nil;
    [d->handler release];
}

void core::platform::macos::ApplicationMenuMac::setup()
{
    if (d->menuBar) {
        return;
    }

    d->handler = [[ApplicationMenuHandler alloc] init];
    d->handler.quitHandler = ^{
        if (d->quitCallback) {
            const auto callback = d->quitCallback;
            callback();
        }
    };
    // Top-level macOS menu bar
    d->menuBar = [[NSMenu alloc] init];

    createAppMenu ();
    createFileMenu ();
    createEditMenu ();
    createWindowMenu ();

    NSApp.mainMenu = d->menuBar;


}



void core::platform::macos::ApplicationMenuMac::setAboutCallback(std::__1::function<void ()> callback)
{
    d->aboutCallback = std::move (callback);
}

void core::platform::macos::ApplicationMenuMac::setSettingCallback(std::__1::function<void ()> callback)
{
    d->settingCallback = std::move (callback);
}

void core::platform::macos::ApplicationMenuMac::setQuiteCallback(std::__1::function<void ()> callback)
{
    d->quitCallback = std::move (callback);
}


// PRIVATE: API
void core::platform::macos::ApplicationMenuMac::createAppMenu()
{
    NSMenuItem *appMenuItem = [[NSMenuItem alloc] init];
    [d->menuBar addItem: appMenuItem];

    // App submenu
    d->appMenu = [[NSMenu alloc] initWithTitle: NSLocalizedString (@"ChatApp", @"")];
    appMenuItem.submenu = d->appMenu;

    // About menu
    NSString *appName = [[NSProcessInfo processInfo] processName];

    NSString *aboutTitle = [NSString stringWithFormat:@"About %@", appName];
    NSMenuItem *about = [[NSMenuItem alloc] initWithTitle: aboutTitle
                                                   action: @selector(didTapAbout:)
                                            keyEquivalent: @""];
    about.target = d->handler;
    [d->appMenu addItem: about];
    [d->appMenu addItem: NSMenuItem.separatorItem];

    // preferences
    NSString *preferencesTitle = NSLocalizedString (@"Preferences…", @"");
    NSMenuItem *preferences = [[NSMenuItem alloc] initWithTitle: preferencesTitle
                                                     action: @selector(didTapPreferences:)
                                              keyEquivalent: @","];
    preferences.target = d->handler;
    preferences.keyEquivalentModifierMask = NSEventModifierFlagCommand;
    [d->appMenu addItem: preferences];
    [d->appMenu addItem: NSMenuItem.separatorItem];

    // servvice
    NSMenuItem *serviceItem = [[NSMenuItem alloc] initWithTitle: NSLocalizedString (@"Services", @"")
                                                         action: nil
                                                  keyEquivalent: @""];
    NSMenu *services = [[NSMenu alloc] initWithTitle: NSLocalizedString (@"Services", @"")];
    serviceItem.submenu = services;
    [d->appMenu addItem: serviceItem];
    NSApp.servicesMenu = services;

    // Hide
    NSMenuItem *hide = [[NSMenuItem alloc] initWithTitle: NSLocalizedString (@"Hide", @"")
                                                  action: @selector(hide:)
                                           keyEquivalent: @"h"];
    hide.target = NSApp;
    [d->appMenu addItem: hide];

    // Hide Others
    NSMenuItem *hideOther = [[NSMenuItem alloc] initWithTitle: NSLocalizedString (@"Hide Others", @"")
                                                       action: @selector(hideOtherApplications:)
                                                keyEquivalent: @"h"];
    hideOther.keyEquivalentModifierMask = NSEventModifierFlagOption | NSEventModifierFlagCommand;
    hideOther.target = NSApp;
    [d->appMenu addItem: hideOther];

    // Show All
    NSMenuItem *showAll = [[NSMenuItem alloc] initWithTitle: NSLocalizedString (@"Show All", @"")
                                                     action: @selector(unhideAllApplications:)
                                              keyEquivalent: @""];
    showAll.target = NSApp;
    [d->appMenu addItem: showAll];

    // Quit
    NSMenuItem *quit = [[NSMenuItem alloc] initWithTitle: NSLocalizedString (@"Quit", @"")
                                                  action: @selector(didTapQuit:)
                                           keyEquivalent: @"q"];
    quit.target = d->handler;
    quit.keyEquivalentModifierMask = NSEventModifierFlagCommand;
    [d->appMenu addItem: quit];

#if !__has_feature(objc_arc)
    [appMenuItem release];
    [about release ];
    [preferences release];
    [hide release];
    [hideOther release];
    [showAll release];
    [quit release];
    [serviceItem release];
    [services release];
#endif
}

void core::platform::macos::ApplicationMenuMac::createFileMenu()
{
    NSMenuItem *fileMenuItem = [[NSMenuItem alloc] init];
    [d->menuBar addItem: fileMenuItem];

    d->fileMenu = [[NSMenu alloc] initWithTitle: NSLocalizedString (@"File" , @"")];

    NSMenuItem *logout = [[NSMenuItem alloc] initWithTitle: NSLocalizedString (@"Log out", @"")
                                                    action: @selector(didTapLogout:)
                                             keyEquivalent: @""];
    logout.target = d->handler;
    [d->fileMenu addItem: logout];

    fileMenuItem.submenu = d->fileMenu;
#if !__has_feature(objc_arc)
   [fileMenuItem release];
   [logout release];
#endif

}

void core::platform::macos::ApplicationMenuMac::createEditMenu()
{
    NSMenuItem *editMenuItem = [[NSMenuItem alloc] init];
    [d->menuBar addItem: editMenuItem];

    d->editMenu = [[NSMenu alloc] initWithTitle: NSLocalizedString (@"Edit", @"")];
    // A nil target sends editing actions through the native responder chain.
    auto addEditItem = [this](NSString *title, SEL action, NSString *keyEquivalent) {
        NSMenuItem *item = [d->editMenu addItemWithTitle: title
                                               action: action
                                        keyEquivalent: keyEquivalent];
        item.target = nil;
        item.keyEquivalentModifierMask = NSEventModifierFlagCommand;
        return item;
    };

    addEditItem(NSLocalizedString (@"Undo", @""), @selector(undo:), @"z");
    NSMenuItem *redo = addEditItem(NSLocalizedString (@"Redo", @""), @selector(redo:), @"z");
    redo.keyEquivalentModifierMask = NSEventModifierFlagCommand | NSEventModifierFlagShift;
    [d->editMenu addItem: NSMenuItem.separatorItem];

    addEditItem(NSLocalizedString (@"Cut", @""), @selector(cut:), @"x");
    addEditItem(NSLocalizedString (@"Copy", @""), @selector(copy:), @"c");
    addEditItem(NSLocalizedString (@"Paste", @""), @selector(paste:), @"v");
    NSMenuItem *pasteAndMatchStyle = addEditItem(NSLocalizedString (@"Paste and Match Style", @""),
                                               @selector(pasteAsPlainText:), @"v");
    pasteAndMatchStyle.keyEquivalentModifierMask = NSEventModifierFlagCommand
        | NSEventModifierFlagOption | NSEventModifierFlagShift;
    addEditItem(NSLocalizedString (@"Delete", @""), @selector(delete:), @"");
    [d->editMenu addItem: NSMenuItem.separatorItem];
    addEditItem(NSLocalizedString (@"Select All", @""), @selector(selectAll:), @"a");

    editMenuItem.submenu = d->editMenu;

#if !__has_feature(objc_arc)
    [editMenuItem release];
#endif

}

void core::platform::macos::ApplicationMenuMac::createWindowMenu()
{
    NSMenuItem *windowMenuItem = [[NSMenuItem alloc] init];
    [d->menuBar addItem: windowMenuItem];

    d->windowMenu = [[NSMenu alloc] initWithTitle: NSLocalizedString (@"Window", @"")];

    // Window actions follow the responder chain to the active window.
    NSMenuItem *minimize = [d->windowMenu addItemWithTitle: NSLocalizedString (@"Minimize", @"")
                                                 action: @selector(performMiniaturize:)
                                          keyEquivalent: @"m"];
    minimize.target = nil;
    minimize.keyEquivalentModifierMask = NSEventModifierFlagCommand;

    NSMenuItem *zoom = [d->windowMenu addItemWithTitle: NSLocalizedString (@"Zoom", @"")
                                             action: @selector(performZoom:)
                                      keyEquivalent: @""];
    zoom.target = nil;
    [d->windowMenu addItem: NSMenuItem.separatorItem];

    NSMenuItem *bringAllToFront = [d->windowMenu addItemWithTitle: NSLocalizedString (@"Bring All to Front", @"")
                                                        action: @selector(arrangeInFront:)
                                                 keyEquivalent: @""];
    bringAllToFront.target = NSApp;

    windowMenuItem.submenu = d->windowMenu;
    NSApp.windowsMenu = d->windowMenu;

#if !__has_feature(objc_arc)
    [windowMenuItem release];
#endif
}










































