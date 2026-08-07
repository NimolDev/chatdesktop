#include "font_manager.hpp"
#include <QFontDatabase>
#include <QDebug>


namespace shared::theme {
namespace {
    QString m_family;
}

bool FontManager::initialize()
{
    const int light_id = QFontDatabase::addApplicationFont (":/fonts/Inter-Light.ttf");
    const int regular_id = QFontDatabase::addApplicationFont (":/fonts/Inter-Regular.ttf");
    const int medium_id = QFontDatabase::addApplicationFont (":/fonts/Inter-Medium.ttf");
    const int semibold_id = QFontDatabase::addApplicationFont (":/fonts/Inter-SemiBold.ttf");
    const int bold_id = QFontDatabase::addApplicationFont (":/fonts/Inter-Bold.ttf");

    const int km_thin = QFontDatabase::addApplicationFont (":/fonts/Battambang-Thin.ttf");
    const int km_light = QFontDatabase::addApplicationFont (":/fonts/Battambang-Light.ttf");
    const int km_regualar = QFontDatabase::addApplicationFont (":/fonts/Battambang-Regular.ttf");
    const int km_bold = QFontDatabase::addApplicationFont (":/fonts/Battambang-Bold.ttf");

    if (
        light_id == -1 ||
        regular_id == -1 ||
        medium_id == -1 ||
        semibold_id == -1 ||
        bold_id == -1 ||
        km_light == -1 ||
        km_thin == -1 ||
        km_regualar == -1 ||
        km_bold == -1
        ) {
        qWarning() << "Failed to load fonts";
        return false;
    }
    const QStringList families = QFontDatabase::applicationFontFamilies(light_id);
    if (families.isEmpty ()) {
        qDebug() << "No font family found";
        return false;
    }
    m_family = families.first ();
    qDebug() << "Loaded fonts: " << m_family;
    return true;
}

QString FontManager::family()
{
    return m_family;
}

}