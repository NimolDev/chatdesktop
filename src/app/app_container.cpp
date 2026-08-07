#include "app_container.hpp"

#include "login_repository_impl.hpp"
#include "xmpp/xmpp_manager.hpp"
#include "network/network_client.hpp"
#include "network/logging_interceptor.hpp"

// #include "authentication/data/login_repository_impl.hpp"
// #include "authentication/domain/login_repository.hpp"
// #include "authentication/domain/usecase/login_use_case.hpp"
#include "authentication/presentation/viewmodel/login_vm.hpp"

AppContainer::AppContainer(QQmlApplicationEngine *engine)
    : m_engine(engine)
{
    registerCoreService ();
    registerAuthentication ();
    // registerQmlSignleton ();
}

void AppContainer::registerCoreService()
{
    m_container.registerFactory<core::network::NetworkClient> ([](ServiceContainer &c) {
        auto client = std::make_shared<core::network::NetworkClient> (QStringLiteral (""));
        client->addInterceptor (c.resolve<core::network::logging::LoggingInterceptor> ());
        return client;
    });
    m_container.registerSingleton<core::xmpp::XmppManager> ([](ServiceContainer &c) {
        return std::make_shared<core::xmpp::XmppManager> ();
    });
}

void AppContainer::registerQmlSignleton()
{
    LoginVM::setInstance(m_container.resolve<LoginVM>().get());
}

void AppContainer::registerAuthentication()
{
    m_container.registerFactory<domain::LoginRepository>([](ServiceContainer &c) {
        return std::make_shared<data::LoginRepositoryImpl> (c.resolve<core::xmpp::XmppManager> ());
    });
    m_container.registerFactory<domain::usecase::LoginUseCase> ([] (ServiceContainer &c) {
        return std::make_shared<domain::usecase::LoginUseCase> (c.resolve<domain::LoginRepository> ());
    });
    m_container.registerSingleton<LoginVM> ([](ServiceContainer &c) {
        return std::make_shared<LoginVM> (c.resolve<domain::usecase::LoginUseCase> ());
    });

    LoginVM::setInstance(m_container.resolve<LoginVM>().get());
}
