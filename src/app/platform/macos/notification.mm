#include "notification.hpp"

#import <AppKit/AppKit.h>
#import <UserNotifications/UserNotifications.h>


@interface NotificationDelegate: NSObject<UNUserNotificationCenterDelegate>
- (void)applicationDidBecomeActive:(NSNotification *)notification;
@end

@implementation NotificationDelegate



- (void)userNotificationCenter:(UNUserNotificationCenter *)center
       willPresentNotification:(UNNotification *)notification
         withCompletionHandler:(void (^)(UNNotificationPresentationOptions))completionHandler
{
    Q_UNUSED(center);
    Q_UNUSED(notification);

    // Do not present a system banner over ChatApp itself. Besides being
    // redundant, clicking a foreground banner causes macOS to send the app a
    // reopen request. The app should update its current conversation in-app.
    if (NSApp.isActive) {
        completionHandler(UNNotificationPresentationOptionNone);
        return;
    }

    NSLog (@"Notification will receive");
    completionHandler(UNNotificationPresentationOptionBanner |
                      UNNotificationPresentationOptionSound);
}

- (void)userNotificationCenter:(UNUserNotificationCenter *)center
    didReceiveNotificationResponse:(UNNotificationResponse *)response
             withCompletionHandler:(void (^)(void))completionHandler
{
    Q_UNUSED(center);

    if ([response.actionIdentifier
            isEqualToString:UNNotificationDefaultActionIdentifier]) {
        dispatch_async(dispatch_get_main_queue(), ^{
            // Notification Center activates the application before invoking
            // this callback. If it is already showing a key window, leave the
            // current application state alone instead of reopening it.
            if (NSApp.isActive && NSApp.keyWindow != nil) {
                return;
            }

            // Reuse the existing Qt window when the app was hidden or
            // minimized. Never create another application/window here.
            for (NSWindow *window in NSApp.windows) {
                if (window.canBecomeKeyWindow) {
                    [window makeKeyAndOrderFront:nil];
                    [NSApp activateIgnoringOtherApps:YES];
                    break;
                }
            }
        });

        NSLog(@"Notification clicked: %@",
              response.notification.request.identifier);
        NSDictionary *userInfo = response.notification.request.content.userInfo;
        NSLog (@"notification: %@", userInfo);
    }

    // UserNotifications requires this to be called exactly once. Leaving the
    // response unfinished can cause macOS to retry or retain the activation.
    completionHandler();
}

- (void)applicationDidBecomeActive:(NSNotification *)notification {
}

@end

static NotificationDelegate *notificationDelegate = nil;

static void installNotificationDelegate()
{
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        notificationDelegate = [[NotificationDelegate alloc] init];
        [UNUserNotificationCenter currentNotificationCenter].delegate =
            notificationDelegate;
        [[NSNotificationCenter defaultCenter]
            addObserver:notificationDelegate
               selector:@selector(applicationDidBecomeActive:)
                   name:NSApplicationDidBecomeActiveNotification
                 object:NSApp];
    });
}

namespace platform {
namespace macos {

static NSString *toNSString(const QString &value)
{
    return [NSString stringWithUTF8String: value.toUtf8 ().constData ()];
}

Notification::Notification()
{
    installNotificationDelegate();
}

Notification &Notification::instance()
{
    static Notification notification;
    return notification;
}



void Notification::show(const QString &title, const QString &message)
{

    UNMutableNotificationContent *content = [[UNMutableNotificationContent alloc] init];

    NSString *contentTitle = toNSString (title);
    NSString *contentBody = toNSString (message);
    if (contentTitle && contentBody) {
        content.title = contentTitle;
        content.body = contentBody;
    }

    content.sound = [UNNotificationSound defaultSound];
    content.userInfo = @{
        @"senderId": toNSString("senderId"),
        @"senderName": toNSString("senderName")
    };

    NSString *identifier = [[NSUUID UUID] UUIDString];
    UNNotificationRequest *request = [UNNotificationRequest requestWithIdentifier: identifier
                                                                          content: content
                                                                          trigger:nil];

    [[UNUserNotificationCenter currentNotificationCenter]
        addNotificationRequest:request
         withCompletionHandler:^(NSError *error) {
             if (error) {
                 NSLog(@"Failed to show notification: %@", error.localizedDescription);
             }

         }];
#if !__has_feature(objc_arc)
    [content release];
#endif

}

void Notification::requestPermission()
{
    dispatch_async(dispatch_get_main_queue(), ^{
        UNUserNotificationCenter *center =
            [UNUserNotificationCenter currentNotificationCenter];

        installNotificationDelegate();

        [center getNotificationSettingsWithCompletionHandler:
            ^(UNNotificationSettings *settings) {
                if (settings.authorizationStatus !=
                    UNAuthorizationStatusNotDetermined) {
                    NSLog(@"Notification permission already resolved: %ld",
                          (long)settings.authorizationStatus);
                    return;
                }

                const UNAuthorizationOptions options =
                    UNAuthorizationOptionAlert |
                    UNAuthorizationOptionSound |
                    UNAuthorizationOptionBadge;

                [center requestAuthorizationWithOptions:options
                    completionHandler:^(BOOL granted, NSError *error) {
                        if (error != nil) {
                            NSLog(@"Notification permission error (%@/%ld): %@",
                                  error.domain,
                                  (long)error.code,
                                  error.localizedDescription);
                            return;
                        }

                        NSLog(@"Notification permission: %@",
                              granted ? @"granted" : @"denied");
                    }];
            }];
    });
}




} // namespace macos
} // namespace platform
