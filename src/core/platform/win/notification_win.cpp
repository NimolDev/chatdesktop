#include "notification_win.hpp"
#include "win_tray.hpp"

#include <QApplication>
#include <QDebug>
#include <QMetaObject>
#include <QSystemTrayIcon>

namespace core {
namespace platform {
namespace win {

void NotificationWin::requestPermission()
{
    // Tray notifications have no runtime permission prompt. Windows controls
    // delivery through system notification settings and Do Not Disturb.
    if (!qApp) {
        return;
    }
    QMetaObject::invokeMethod(qApp, [] {
        if (!QSystemTrayIcon::isSystemTrayAvailable() || !QSystemTrayIcon::supportsMessages()) {
            qWarning() << "System tray notifications are unavailable";
        }
    }, Qt::AutoConnection);
}

void NotificationWin::show(const QString &title, const QString &message)
{
    if (!qApp || (title.isEmpty() && message.isEmpty())) {
        return;
    }
    // Deliver on the GUI thread, including when called by a network worker.
    // Capture values only so queued work does not depend on this object's life.
    QMetaObject::invokeMethod(qApp, [title, message] {
        if (QApplication::applicationState() == Qt::ApplicationActive) {
            return;
        }
        if (!QSystemTrayIcon::isSystemTrayAvailable() || !QSystemTrayIcon::supportsMessages()) {
            return;
        }
        TrayWin::showNotification(title, message);
    }, Qt::AutoConnection);
}

} // namespace win
} // namespace platform
} // namespace core
