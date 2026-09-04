#ifndef APP_APP_CONTROLLER_HPP
#define APP_APP_CONTROLLER_HPP

#include <QObject>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlregistration.h>

#include "usecase/logout_usecase.hpp"
#include "usecase/session_usecase.hpp"
#include "xmpp_manager.hpp"

class AppController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(AppState state READ state  NOTIFY stateChanged FINAL)
public:
    enum class AppState {
        Starting,
        CheckingUpdate,
        UpdateRequired,
        Unauthenticated,
        Authenticated,
        FatalError,
        Logout
    };
    Q_ENUM (AppState)

    explicit AppController(
        std::shared_ptr<domain::usecase::SessionUsecase> session_usecase,
        std::shared_ptr<domain::usecase::LogoutUsecase> logout_usecase,
        std::shared_ptr<core::xmpp::XmppManager> xmpp,
        QObject *parent = nullptr
        );

    static AppController *create(QQmlEngine *engine, QJSEngine *scriptEngine);
    static void setInstance(AppController *instance);

    Q_PROPERTY(QString userName READ userName WRITE setUserName NOTIFY userNameChanged FINAL)

    AppState state() const;

    void getRememberUsers();


public:
    Q_INVOKABLE void checkAuthentication();
    Q_INVOKABLE void logout();

    QString userName();


signals:
    void stateChanged();
    // void userNamed(QString &user_name);
    void userChanged(const QString &name);
    void userNameChanged();
    void messageReceived();


private:

    void setState(AppState state);

    AppState m_state = AppState::Starting;

    std::shared_ptr<domain::usecase::SessionUsecase> m_session;
    std::shared_ptr<domain::usecase::LogoutUsecase> m_logout;
    std::shared_ptr<core::xmpp::XmppManager> m_xmpp;
    // std::optional<auth::domain::User> m_user;
    static AppController *s_instance;

    void setUserName(QString &user_name);
    QString m_userName;

};

#endif // APP_APP_CONTROLLER_HPP
