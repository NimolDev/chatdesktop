#ifndef SHARED_LOCALIZATION_LANGUAGE_MANAGER_HPP
#define SHARED_LOCALIZATION_LANGUAGE_MANAGER_HPP

#include <QObject>
#include <QTranslator>
#include <QQmlEngine>
#include <QtQml/qqmlregistration.h>

namespace shared {
namespace localization {

class LanguageManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QString currentLanguage READ currentLanguage  NOTIFY currentLanguageChanged FINAL);
public:
    explicit LanguageManager(
        QQmlEngine *engine,
        QObject *parent = nullptr);

   static LanguageManager *create(QQmlEngine *engine, QJSEngine *scriptEngine);
    [[nodiscard]] QString currentLanguage() const;

    Q_INVOKABLE bool setSetLanguage(const QString &language_code);
    Q_INVOKABLE QStringList availableLanguage() const;

signals:
    void currentLanguageChanged();
    void languageLoadFailed(const QString language_code);

private:
    bool loadTranslator(const QString &language_code);
    void saveLanguageCode(const QString &language_code);

private:
    QQmlEngine * m_engine = nullptr;
    QTranslator m_translator;
    QString m_currentLanguage = QStringLiteral ("en");
};

} // namespace localization
} // namespace shared



#endif // SHARED_LOCALIZATION_LANGUAGE_MANAGER_HPP
