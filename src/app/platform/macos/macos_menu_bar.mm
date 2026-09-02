#include "macos_menu_bar.hpp"

#include <QApplication>
#include <QWindow>

#import <AppKit/AppKit.h>

@interface ChatAppMenuTarget : NSObject
@property(nonatomic, assign) QApplication *app;
@property(nonatomic, assign) QWindow *window;
- (void)showAbout:(id)sender;
- (void)showPreferences:(id)sender;
- (void)quitApp:(id)sender;
@end

@implementation ChatAppMenuTarget

- (void)showAbout:(id)sender
{
    Q_UNUSED(sender);
    if (QWindow *aboutWindow = self.window->findChild<QWindow *>(
            QStringLiteral("aboutWindow"))) {
        aboutWindow->show();
        aboutWindow->raise();
        aboutWindow->requestActivate();
    }
}

- (void)showPreferences:(id)sender
{
    Q_UNUSED(sender);
}

- (void)quitApp:(id)sender
{
    Q_UNUSED(sender);
    self.app->quit();
}

@end

namespace platform::macos {

class MacosMenuBar::Private
{
public:
    NSMenu *mainMenu = nil;
    ChatAppMenuTarget *target = nil;
};

MacosMenuBar::MacosMenuBar(QApplication *app, QWindow *window)
    : d(new Private)
{
    d->target = [[ChatAppMenuTarget alloc] init];
    d->target.app = app;
    d->target.window = window;

    const auto addItem = [](NSMenu *menu, NSString *title, SEL action,
                            NSString *shortcut, id target = nil) {
        NSMenuItem *item = [[NSMenuItem alloc] initWithTitle:NSLocalizedString (title, @"")
                                                     action:action
                                              keyEquivalent:shortcut];
        item.target = target;
        [menu addItem:item];
        [item release];
    };

    const auto addTopLevelMenu = [](NSMenu *menuBar, NSString *title) {
        NSMenuItem *rootItem = [[NSMenuItem alloc] initWithTitle:NSLocalizedString (title, @"")
                                                          action:nil
                                                   keyEquivalent:@""];
        NSMenu *menu = [[NSMenu alloc] initWithTitle:NSLocalizedString (title, @"")];
        rootItem.submenu = menu;
        [menuBar addItem:rootItem];
        [rootItem release];
        return menu;
    };

    NSString *appName = NSProcessInfo.processInfo.processName;
    d->mainMenu = [[NSMenu alloc] initWithTitle:@""];

    // Application menu
    NSMenu *appMenu = addTopLevelMenu(d->mainMenu, appName);
    addItem(appMenu, [NSString stringWithFormat:@"About %@", appName],
            @selector(showAbout:), @"", d->target);
    addItem(appMenu, @"Settings…", @selector(showPreferences:), @",", d->target);
    [appMenu addItem:NSMenuItem.separatorItem];
    NSMenuItem *servicesItem = [[NSMenuItem alloc] initWithTitle: NSLocalizedString (@"Services", @"")
                                                          action:nil
                                                   keyEquivalent:@""];
    NSMenu *servicesMenu = [[NSMenu alloc] initWithTitle: NSLocalizedString (@"Services", @"")];
    servicesItem.submenu = servicesMenu;
    [appMenu addItem:servicesItem];
    NSApp.servicesMenu = servicesMenu;
    [servicesMenu release];
    [servicesItem release];
    [appMenu addItem:NSMenuItem.separatorItem];
    addItem(appMenu, [NSString stringWithFormat:@"Hide %@", appName],
            @selector(hide:), @"h", NSApp);
    addItem(appMenu, @"Hide Others", @selector(hideOtherApplications:), @"h", NSApp);
    appMenu.itemArray.lastObject.keyEquivalentModifierMask =
        NSEventModifierFlagCommand | NSEventModifierFlagOption;
    addItem(appMenu, @"Show All", @selector(unhideAllApplications:), @"", NSApp);
    [appMenu addItem:NSMenuItem.separatorItem];
    addItem(appMenu, [NSString stringWithFormat:@"Quit %@", appName],
            @selector(quitApp:), @"q", d->target);
    [appMenu release];

    // File menu
    NSMenu *fileMenu = addTopLevelMenu(d->mainMenu, @"File");
    addItem(fileMenu, @"Close", @selector(performClose:), @"w");
    [fileMenu release];

    // Edit menu actions use the native responder chain, so they work with
    // whichever Qt text control currently has focus.
    NSMenu *editMenu = addTopLevelMenu(d->mainMenu, @"Edit");
    addItem(editMenu, @"Undo", @selector(undo:), @"z");
    addItem(editMenu, @"Redo", @selector(redo:), @"Z");
    [editMenu addItem:NSMenuItem.separatorItem];
    addItem(editMenu, @"Cut", @selector(cut:), @"x");
    addItem(editMenu, @"Copy", @selector(copy:), @"c");
    addItem(editMenu, @"Paste", @selector(paste:), @"v");
    addItem(editMenu, @"Paste and Match Style", @selector(pasteAsPlainText:),
            @"v");
    editMenu.itemArray.lastObject.keyEquivalentModifierMask =
        NSEventModifierFlagCommand | NSEventModifierFlagOption | NSEventModifierFlagShift;
    addItem(editMenu, @"Delete", @selector(delete:), @"");
    addItem(editMenu, @"Select All", @selector(selectAll:), @"a");
    [editMenu release];

    // View menu
    NSMenu *viewMenu = addTopLevelMenu(d->mainMenu, @"View");
    addItem(viewMenu, @"Enter Full Screen", @selector(toggleFullScreen:), @"f");
    viewMenu.itemArray.lastObject.keyEquivalentModifierMask =
        NSEventModifierFlagCommand | NSEventModifierFlagControl;
    [viewMenu release];

    // Window menu
    NSMenu *windowMenu = addTopLevelMenu(d->mainMenu, @"Window");
    addItem(windowMenu, @"Minimize", @selector(performMiniaturize:), @"m");
    addItem(windowMenu, @"Zoom", @selector(performZoom:), @"");
    [windowMenu addItem:NSMenuItem.separatorItem];
    addItem(windowMenu, @"Bring All to Front", @selector(arrangeInFront:), @"");
    NSApp.windowsMenu = windowMenu;
    [windowMenu release];

    // Help menu
    NSMenu *helpMenu = addTopLevelMenu(d->mainMenu, @"Help");
    addItem(helpMenu, [NSString stringWithFormat:@"%@ Help", appName],
            @selector(showHelp:), @"?", NSApp);
    NSApp.helpMenu = helpMenu;
    [helpMenu release];

    NSApp.mainMenu = d->mainMenu;
}

MacosMenuBar::~MacosMenuBar()
{
    if (NSApp.mainMenu == d->mainMenu) {
        NSApp.mainMenu = nil;
    }
    [d->target release];
    [d->mainMenu release];
    delete d;
}

} // namespace platform::macos
