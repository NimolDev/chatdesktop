#include "language_manager.hpp"

#include <QCoreApplication>
#include <QDebug>
#include <QSettings>

namespace shared {
namespace localization {
namespace
{
constexpr auto kLanguageSettingsKey = "application/language";
constexpr auto kDefaultLanguage = "en";
} // namespace

LanguageManager::LanguageManager(QQmlEngine *engine, QObject *parent)
    : m_engine(engine),
    QObject(parent)
{
    QSettings settings;

    const QString savedLanguage = settings.value(
        kLanguageSettingsKey,
        kDefaultLanguage
    ).toString();

    if (!setSetLanguage (savedLanguage)) {
        setSetLanguage (kDefaultLanguage);
    }
}

LanguageManager *LanguageManager::create(QQmlEngine *engine, QJSEngine *scriptEngine)
{
    Q_UNUSED (scriptEngine);
    return new LanguageManager(engine);
}

// LanguageManager *LanguageManager::create(QQmlEngine *engine, QJSEngine *)
// {
//     return LanguageManager(engine);
// }

QString LanguageManager::currentLanguage() const
{
    return m_currentLanguage;
}

QStringList LanguageManager::availableLanguage() const
{
    return {
        QStringLiteral ("en"),
        QStringLiteral ("km")
    };
}

bool LanguageManager::setSetLanguage(const QString &language_code)
{
    const QString normalized_code = language_code.trimmed ();

    if (normalized_code.isEmpty ()) {
        emit languageLoadFailed (language_code);
        return false;
    }
    if (normalized_code == m_currentLanguage) {
        return true;
    }

    /*
     * English is the source language, so it nomally has no .qm file.
     * Removing the active translator restores the source strings.
    */

    if (normalized_code == kDefaultLanguage) {
        QCoreApplication::removeTranslator (&m_translator);

        m_currentLanguage = normalized_code;
        saveLanguageCode (normalized_code);

        if (m_engine != nullptr) {
            m_engine->retranslate ();
        }
        emit currentLanguageChanged ();
        return true;
    }

    if (!loadTranslator (normalized_code)) {
        emit languageLoadFailed (normalized_code);
        return false;
    }

    m_currentLanguage = normalized_code;
    saveLanguageCode (normalized_code);

    if (m_engine != nullptr) {
        m_engine->retranslate ();
    }

    emit currentLanguageChanged ();
    return true;
}

bool LanguageManager::loadTranslator(const QString &language_code)
{
    QCoreApplication::removeTranslator (&m_translator);

    const QString translation_path =
        QStringLiteral(":/i18n/shared/localization/app_%1.qm").arg(language_code);

    if (!m_translator.load (translation_path)) {
        qWarning() << "Cannot load translation: " << translation_path;
        return false;;
    }

    if (!QCoreApplication::installTranslator (&m_translator)) {
        qWarning() << "Cannot install translation: " << translation_path;
        return false;
    }
    return true;
}

void LanguageManager::saveLanguageCode(const QString &language_code)
{
    QSettings settings;
    settings.setValue (kLanguageSettingsKey, language_code);
}

} // namespace localization
} // namespace shared