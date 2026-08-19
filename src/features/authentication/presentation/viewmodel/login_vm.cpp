#include "login_vm.hpp"



LoginVM *LoginVM::s_instance = nullptr;

LoginVM::LoginVM(std::shared_ptr<domain::usecase::LoginUseCase> usecase, QObject *parent)
    : m_usecase(std::move (usecase)),
    QObject(parent)
{

    // connect(
    //     m_usecase.get (),
    //     &domain::usecase::LoginUseCase::loginSuccessed,
    //     this,
    //     [this](domain::entity::LoginResponse response) {
    //         // qDebug() << "Login Success" << response.user_name;
    //         // m_userName = response.;
    //         // m_userJid = response.user_jid;
    //         emit loginSucceeded ();
    //     }
    // );

    connect(
        m_usecase.get (),
        &domain::usecase::LoginUseCase::loginFailed,
        this,
        &LoginVM::loginFailed
        );

    connect(
        m_usecase.get (),
        &domain::usecase::LoginUseCase::connecting,
        this,
        &LoginVM::connecting
        );
    connect(
        m_usecase.get (),
        &domain::usecase::LoginUseCase::connected,
        this,
        &LoginVM::connected
        );
    connect (
        m_usecase.get (),
        &domain::usecase::LoginUseCase::loginSucceded,
        this,
        [this]() {
            emit loginSucceeded ();
            setIsLoggedIn (true);
        }
        );


}

LoginVM *LoginVM::create(QQmlEngine *engine, QJSEngine *scriptEngine)
{
    Q_UNUSED(scriptEngine)
    Q_ASSERT(s_instance);
    Q_ASSERT(s_instance->thread() == engine->thread());

    QQmlEngine::setObjectOwnership(s_instance, QQmlEngine::CppOwnership);
    return s_instance;
}

void LoginVM::setInstance(LoginVM *instance)
{
    Q_ASSERT(instance);
    s_instance = instance;
}

void LoginVM::login(const QString user_name, const QString password)
{
    domain::entity::LoginRequest request = {
        user_name,
        password
    };
    m_usecase ->execute (request);
}

bool LoginVM::isLoading() const
{
    return m_loading;
}

bool LoginVM::isLoggedIn() const
{
    return m_loggedIn;
}

QString LoginVM::userName() const
{
    return m_userName;
}

QString LoginVM::userJid() const
{
    return m_userJid;
}

void LoginVM::setIsLoading(bool loading)
{
    if (m_loading == loading) {
        return;
    }
    m_loading = loading;
    emit isLoadingChanged();
}

void LoginVM::setIsLoggedIn(bool loggedIn)
{
    if (m_loggedIn == loggedIn) {
        return;
    }
    m_loggedIn = loggedIn;
    emit isLoggedInChanged ();
}
