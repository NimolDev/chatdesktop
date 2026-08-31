#include "app_controller.hpp"

#include <QDebug>

#include "storage/database_helper.hpp"

AppController *AppController::s_instance = nullptr;

AppController::AppController(
    std::shared_ptr<domain::usecase::SessionUsecase> usecase,
    std::shared_ptr<domain::usecase::LogoutUsecase> logout_usecase,
    std::shared_ptr<core::xmpp::XmppManager> xmpp,
    QObject *parent
    )
    : m_session(std::move (usecase)),
    m_logout(std::move (logout_usecase)),
    m_xmpp(std::move (xmpp)),
    QObject(parent)
{

    // connect (
    //     m_authReposity.get (),
    //     &IAuthenticationRepository::logoutSucceded,
    //     this,
    //     [this]() {
    //         qDebug() << "Logout Success";
    //         setState (AppState::Unauthenticated);
    //         emit stateChanged();
    //     }
    //     );

    connect (
        m_session.get (),
        &domain::usecase::SessionUsecase::sessionChanged,
        this,
        [this](bool session) {
            qDebug() << "Session" << session;
            if (session) {
                setState (AppState::Authenticated);
                auto result = core::storage::DatabaseHelper::getCurrentUser ();
                QString user_name = result.value (0).toString ();
                QString id = result.value (1).toString ();
                qDebug() << "User Name:"<<user_name;
                setUserName(user_name);
            } else {
                setState (AppState::Unauthenticated);
            }
        }
        );
    connect(
        m_xmpp.get (),
        &core::xmpp::XmppManager::connectedChanged,
        this,
        [this]() {
            switch(m_xmpp->connectionState ()) {
            case core::xmpp::XmppManager::ConnectionState::Disconnected:
                qDebug() << "Appcontroller disconnected ";
                // setState (AppState::Authenticated);
                break;
            case core::xmpp::XmppManager::ConnectionState::Connecting:
                qDebug() << "Appcontroller Connecting ";
                // setState (AppState::Authenticated);
                break;
            case core::xmpp::XmppManager::ConnectionState::Connected:
                qDebug() << "Appcontroller Connected ";
                // setState (AppState::Authenticated);
                break;
            }
        }
        );

    connect (
        m_logout.get (),
        &domain::usecase::LogoutUsecase::sessionChanged,
        this,
        [this](bool logout) {
            if (!logout) {
                setState (AppState::Logout);
                qDebug() << "Logout" << logout;
            }
        }
        );

}

AppController *AppController::create(QQmlEngine *engine, QJSEngine *scriptEngine)
{
    Q_UNUSED(scriptEngine)
    Q_ASSERT(s_instance);
    Q_ASSERT(s_instance->thread() == engine->thread());

    QQmlEngine::setObjectOwnership(s_instance, QQmlEngine::CppOwnership);
    return s_instance;
}

void AppController::setInstance(AppController *instance)
{
    Q_ASSERT (instance);
    s_instance = instance;
}

void AppController::getRememberUsers()
{
    // QSqlQuery query = core::storage::DatabaseHelper::getUserQuery ();

    // query.prepare (R"(SELECT * FROM user)");

    // if (!query.exec ()) {
    //     qWarning() << query.lastError ().text ();
    //     return;
    // }

    // QList<auth::domain::User> users;
    // while(query.next ()) {
    //     auth::domain::User user;
    //     user.id = query.value ("id").toString ();
    //     user.name = query.value ("name").toString ();
    //     user.password = query.value ("password").toString ();
    //     users.append (user);
    // }

    // qDebug() << "User count"<< users.count ();

}

AppController::AppState AppController::state() const
{
    return m_state;
}

void AppController::checkAuthentication()
{
    m_session->execute ();
}

void AppController::logout()
{
    // m_authReposity ->logout ();
    m_logout->execute ();
}

QString AppController::userName()
{
    return m_userName;
    // return "";
}

void AppController::setState(AppState state)
{
    if (m_state == state) {
        return;
    }
    m_state = state;
    emit stateChanged();
    // qDebug() << "State" << state;
}

void AppController::setUserName(QString &user_name)
{
    if (m_userName == user_name) {
        return;
    }
    m_userName = user_name;
    emit userNameChanged ();
}
