#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QtGui>

#include "application/app_info.hpp"
#include "localization/language_manager.hpp"
#include "theme/font_manager.hpp"



#include "storage/database_manager.hpp"
#include "app_container.hpp"


namespace  {
void initializedFont() {
    shared::theme::FontManager::initialize ();
}

} // namespace


int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QCoreApplication::setApplicationName(core::application::AppInfo::name());
    QCoreApplication::setApplicationVersion(core::application::AppInfo::version());
    QCoreApplication::setOrganizationName(core::application::AppInfo::organizationName());
    QCoreApplication::setOrganizationDomain(core::application::AppInfo::organizationDomain());

    qDebug() << "DPI:" << QGuiApplication::primaryScreen()->devicePixelRatio();

    QQmlApplicationEngine engine;

    AppContainer app_container(&engine);

    initializedFont ();
    if (!core::storage::DatabaseManager::initialize ()) {
        qCritical() << "Local database initialization failed";
        return EXIT_FAILURE;
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

    engine.singletonInstance<shared::localization::LanguageManager *>(
        "Localization",
        "LanguageManager"
    );


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

    return QGuiApplication::exec();
}
