#include <QApplication>
#include <QQmlApplicationEngine>
#include <QtGui>
#include <QLocationPermission>
#include <QPermission>
#include <QSystemTrayIcon>
#include <QWindow>
#include <QIcon>
#include <QMenu>
#include <QAction>
#include <QTimer>
#include <QDir>
#include <QLockFile>
#include <QStandardPaths>

#include "application/app_info.hpp"
#include "localization/language_manager.hpp"
#include "theme/font_manager.hpp"

#include "storage/database_manager.hpp"
#include "app_container.hpp"

#ifdef Q_OS_MACOS
#include "platform/macos/macos_tray_icon.hpp"
#include "platform/macos/notification.hpp"
#endif



namespace  {
void initializedFont() {
    shared::theme::FontManager::initialize ();
}

void permissionRequest() {
    QLocationPermission permission;
    permission.setAccuracy (QLocationPermission::Precise);
    switch(qApp->checkPermission (permission)) {
    case Qt::PermissionStatus::Undetermined:
        qApp->requestPermission (permission, [](const QPermission &permission) {
            if (permission.status () == Qt::PermissionStatus::Granted) {
                qDebug() << "Location permission granted";
            } else {
                qDebug() << "Location permission denied";
            }
        });
        break;
    case Qt::PermissionStatus::Granted:
        qDebug() << "Location permissioin granted";
        break;
    case Qt::PermissionStatus::Denied:
        qDebug() << "Location permission denied";
        qApp->requestPermission (permission, [](const QPermission &permission) {
            if (permission.status () == Qt::PermissionStatus::Granted) {
                qDebug() << "Location permission granted";
            } else {
                qDebug() << "Location permission denied";
            }
        });
        break;
    }

}
void openDBConnection(QGuiApplication &app) {

    if (!core::storage::DatabaseManager::initialize ()) {
        qCritical() << "Local database initialization failed";
        return;
    } else {
        qDebug() << "Database initialization";
    }
    QObject::connect (
        &app,
        &QCoreApplication::aboutToQuit,
        []() {
            core::storage::DatabaseManager::close ();
        }
        );
}
void appEngineRegister(QGuiApplication &app, QQmlApplicationEngine &engine) {
    initializedFont ();


    engine.singletonInstance<shared::localization::LanguageManager *>(
        "Localization",
        "LanguageManager"
        );
}

} // namespace


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QCoreApplication::setApplicationName(core::application::AppInfo::name());
    QCoreApplication::setApplicationVersion(core::application::AppInfo::version());
    // QCoreApplication::setOrganizationName(core::application::AppInfo::organizationName());
    // QCoreApplication::setOrganizationDomain(core::application::AppInfo::organizationDomain());

    // Notification Center can ask Launch Services to start the bundle again
    // when an alert is clicked. Stop that second process before it constructs
    // another QML engine/window. Keep the lock alive for the entire main().

    // const QString instanceLockPath = QDir(
    //     QStandardPaths::writableLocation(QStandardPaths::TempLocation)
    //     ).filePath(core::application::AppInfo::bundleIdentifier()
    //                + QStringLiteral(".lock"));
    // QLockFile instanceLock(instanceLockPath);
    // if (!instanceLock.tryLock()) {
    //     qInfo() << "ChatApp is already running; refusing duplicate launch";
    //     return EXIT_SUCCESS;
    // }

    app.setQuitOnLastWindowClosed (false);

    // Container-owned QML singletons must outlive the QML engine. Local
    // variables are destroyed in reverse construction order.
    openDBConnection (app);
    AppContainer app_container;
    QQmlApplicationEngine engine;
    appEngineRegister (app, engine);

    // set app to dark mode only
    QStyleHints *styleHints = QGuiApplication::styleHints();
    styleHints->setColorScheme(Qt::ColorScheme::Dark);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection
    );
    engine.loadFromModule("ChatApp", "Main");
    if (engine.rootObjects().isEmpty()) {
        core::storage::DatabaseManager::close();
        return EXIT_FAILURE;
    }

    permissionRequest ();
#ifdef Q_OS_MACOS
    // Permission UI must be requested after the app has entered its event loop.
    QTimer::singleShot(0, &app, []() {
        platform::macos::Notification notification;
        notification.requestPermission();
        notification.show ("Test", "Test");
    });
#endif

    /// ----- QTray icon ----
    auto *window = qobject_cast<QWindow *>(engine.rootObjects ().constFirst());

    if (!window) {
        qWarning() << "Root object it not a QWindow";
        return EXIT_FAILURE;
    }

    // app.setBadgeNumber (10);


#ifdef Q_OS_MACOS
    MacosTrayIcon trayIcon(&app, window);
    trayIcon.setBadgeNumber (1000);
#else

    QSystemTrayIcon trayIcon;
    trayIcon.setIcon(QIcon(":/images/1024.png"));
    trayIcon.setToolTip("ChatApp");

    QMenu trayMenu;
    QAction *openAction = trayMenu.addAction("Open ChatApp");
    QAction *notificationAction = trayMenu.addAction("Disable Notification");
    trayMenu.addSeparator();
    QAction *quitAction = trayMenu.addAction("Quit ChatApp");

    trayIcon.show();

    const auto restoreWindow = [window]() {
        window->show();
        window->raise();
        window->requestActivate();
    };

    QObject::connect(
        &trayIcon,
        &QSystemTrayIcon::activated,
        [restoreWindow, &trayMenu, &trayIcon](QSystemTrayIcon::ActivationReason reason) {
            if (reason == QSystemTrayIcon::Trigger) {
                restoreWindow();
            } else if (reason == QSystemTrayIcon::Context) {
                const QRect trayGeometry = trayIcon.geometry();
                const QSize menuSize = trayMenu.sizeHint();
                QScreen *screen = QGuiApplication::screenAt(trayGeometry.center());

                if (!screen) {
                    screen = QGuiApplication::primaryScreen();
                }

                const QRect availableGeometry = screen->availableGeometry();
                int x = trayGeometry.center().x() - menuSize.width() / 2;
                x = qBound(
                    availableGeometry.left(),
                    x,
                    availableGeometry.right() - menuSize.width() + 1
                    );

                const int spaceBelow =
                    availableGeometry.bottom() - trayGeometry.bottom();
                const int y = spaceBelow >= menuSize.height()
                    ? trayGeometry.bottom() + 1
                    : trayGeometry.top() - menuSize.height();

                trayMenu.popup(QPoint(x, y));
            }
        }
        );

    QObject::connect(openAction, &QAction::triggered, restoreWindow);

    QObject::connect(
        notificationAction,
        &QAction::triggered,
        [notificationAction]() {
            const bool notificationsDisabled =
                notificationAction->text() == "Disable Notification";
            notificationAction->setText(
                notificationsDisabled
                    ? "Enable Notification"
                    : "Disable Notification"
                );
        }
        );

    QObject::connect(quitAction, &QAction::triggered, &app, &QApplication::quit);
#endif

    return app.exec ();
}
