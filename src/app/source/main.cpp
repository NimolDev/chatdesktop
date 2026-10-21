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
#include <QQuickStyle>
#include <QQmlEngine>
#include <qqmlregistration.h>


#include "app/app_state.hpp"
#include "network/network_monitor.hpp"

#include "application/app_info.hpp"
#include "localization/language_manager.hpp"
#include "theme/font_manager.hpp"


#include "app_container.hpp"
#include "call_coordinator.hpp"

#include "logging/logger.hpp"


#include "platform/platform_main_window.hpp"
#include "platform/platform_tray.hpp"
#include "platform/platform_menu.hpp"
#include "platform/platform_notification.hpp"

namespace  {
#ifdef QT_QML_DEBUG
bool isQmlPreviewSession(int argc, char *argv[])
{
    for (int index = 1; index < argc; ++index) {
        const QString argument = QString::fromLocal8Bit(argv[index]);
        if (!argument.startsWith(QStringLiteral("-qmljsdebugger="))) {
            continue;
        }
        const auto options = argument.mid(QStringLiteral("-qmljsdebugger=").size())
                                 .split(QLatin1Char(','));
        bool services = false;
        for (const QString &option : options) {
            if (option.startsWith(QStringLiteral("services:"))) {
                services = true;
                if (option.mid(9) == QStringLiteral("QmlPreview")) {
                    return true;
                }
            } else if (services && option == QStringLiteral("QmlPreview")) {
                return true;
            }
        }
    }
    return false;
}
#endif

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
#ifdef QT_QML_DEBUG
    // QApplication consumes Qt's debugger argument during construction.
    const bool qmlPreviewSession = isQmlPreviewSession(argc, argv);
#endif

    QQuickStyle::setStyle("Basic");
    QApplication app(argc, argv);

    core::logging::Logger::initialize ();


    QCoreApplication::setApplicationName(core::application::AppInfo::name());
    QCoreApplication::setApplicationVersion(core::application::AppInfo::version());
    // QCoreApplication::setOrganizationName(core::application::AppInfo::organizationName());
    // QCoreApplication::setOrganizationDomain(core::application::AppInfo::organizationDomain());

    // Notification Center can ask Launch Services to start the bundle again
    // when an alert is clicked. Stop that second process before it constructs
    // another QML engine/window. Keep the lock alive for the entire main().

    auto &network_monitor = core::network::NetworkMonitor::instance ();
    QObject::connect(&network_monitor,
                     &core::network::NetworkMonitor::networkConnected,
                     []() {
                         qDebug() << "Network connected";

                     });
    QObject::connect (&network_monitor,
                     &core::network::NetworkMonitor::networkDisconnected,
                     []() {
                         qDebug() << "Network disconnected";
                     });
    network_monitor.setMaximumRetry (10);
    QObject::connect (&network_monitor,
                     &core::network::NetworkMonitor::retryCountDownChanged,
                     [](int value) {
                         qDebug() << "Retry count down: " << value;
                     }
                     );

#ifdef QT_NO_DEBUG
    const QString instanceLockPath = QDir(
        QStandardPaths::writableLocation(QStandardPaths::TempLocation)
        ).filePath(core::application::AppInfo::bundleIdentifier()
                   + QStringLiteral(".lock"));
    QLockFile instanceLock(instanceLockPath);
    if (!instanceLock.tryLock()) {
        qInfo() << "ChatApp is already running; refusing duplicate launch";
        return EXIT_SUCCESS;
    }
#endif
    app.setQuitOnLastWindowClosed (false);

    // Container-owned QML singletons must outlive the QML engine. Local
    // variables are destroyed in reverse construction order.

    AppContainer app_container;
    QQmlApplicationEngine engine;
    appEngineRegister (app, engine);

#ifdef QT_QML_DEBUG
    if (qmlPreviewSession) {
        app.setQuitOnLastWindowClosed(true);
        // Let Qt's preview service own the window: it creates a host for an
        // Item or uses the selected component's Window. A separate host here
        // would survive preview reloads and introduce an extra window.
        const int result = app.exec();
        core::logging::Logger::shutdown();
        return result;
    }
#endif

    // Destroy the coordinator and its windows before the QML engine.
    CallCoordinator callCoordinator(app_container, engine);

    // set app to dark mode only
    QStyleHints *styleHints = QGuiApplication::styleHints();
    styleHints->setColorScheme(Qt::ColorScheme::Dark);

    qmlRegisterSingletonInstance (
        "App.Core",
        1, 0,
        "NetworkMonitor",
        &core::network::NetworkMonitor::instance());
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection
    );
    engine.loadFromModule("ChatApp", "Main");




    permissionRequest ();

    /// ----- QTray icon ----
    auto *window = qobject_cast<QWindow *>(engine.rootObjects ().constFirst());
    auto &app_core = core::app::AppState::instance();

    // app_core.setNotificationCount (10);
    if (!window) {
        qWarning() << "Root object it not a QWindow";
        return EXIT_FAILURE;
    }
    core::platform::PlatformMainWindow platform;
    platform.setup(window);
    platform.setTitleBarColor (window, QColor(0x111315));

    core::platform::PlatformTray tray;
    tray.setup (window, &app);
    tray.showTrayIcon ();
    tray.setTooltip (QStringLiteral ("ChatApp"));
    // tray.updateIcon ();



    tray.setQuitCallback ([&app] {
        app.quit ();
    });
    core::platform::PlatformMenu appMenu;
    appMenu.setup();
    appMenu.setQuiteCallback ([&app] {
        app.quit ();
        qDebug() << "App request to quit";
    });

    core::platform::PlaformNotification notifcation;
    notifcation.requestPermission ();
    // notifcation.show ("Hello", "test");

    QObject::connect (
        app_container.home_chat,
        &HomeChatVM::messageReceived,
        [&notifcation, &app_core, &tray](const domain::entity::MessageItem &payload) {
            notifcation.show (payload.sender_id, payload.body.content.text);
            app_core.appendNotificationCount (1);
            tray.updateIcon ();
        }
        );

    // auto vm =  new CallVM();
    const int result = app.exec ();
    core::logging::Logger::shutdown ();
    return result;
}
