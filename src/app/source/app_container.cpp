#include "app_container.hpp"
#include "app_controller.hpp"

#include "constants/app_constants.hpp"
#include "repository/conversations_repository_impl.hpp"

#include "network/auth_interceptor.hpp"
#include "network/network_client.hpp"
#include "network/logging_interceptor.hpp"

#include "authentication/presentation/viewmodel/login_vm.hpp"
#include "authentication/data/local/login_response_store.hpp"
#include "authentication/data/repository/login_repository_impl.hpp"
#include "authentication/data/repository/session_repository_impl.hpp"

#include "chat/domain/repository/conversation_list_repository.hpp"
#include "chat/domain/usecase/conversations_usecase.hpp"
#include "chat/presentation/viewmodel/conversations_vm.hpp"
#include "chat/domain/repository/message_repository.hpp"
#include "chat/data/repository/message_repository_impl.hpp"
#include "chat/domain/usecase/message_usecase.hpp"
#include "chat/presentation/viewmodel/messaging_vm.hpp"
#include "chat/presentation/viewmodel/home_chat_vm.hpp"

AppContainer::AppContainer()
{
    setupXmpp ();
    registerCoreService ();
    registerAppController ();
    registerAuthentication ();
    registerChat ();
}

AppContainer::~AppContainer()
{
    shutdownXmpp ();
}

void AppContainer::setupXmpp()
{
    m_xmppThread = new QThread();
    m_xmppThread->setObjectName(QStringLiteral("XmppThread"));

    auto *xmpp = new core::xmpp::XmppManager();
    xmpp->moveToThread(m_xmppThread);
    m_xmpp = std::shared_ptr<core::xmpp::XmppManager>(
        xmpp,
        [](core::xmpp::XmppManager *manager) {
            QThread *ownerThread = manager->thread();
            if (ownerThread == QThread::currentThread() || !ownerThread->isRunning()) {
                delete manager;
                return;
            }
            QMetaObject::invokeMethod(
                manager,
                [manager]() { delete manager; },
                Qt::BlockingQueuedConnection);
        });

    QObject::connect(
        m_xmppThread,
        &QThread::started,
        xmpp,
        &core::xmpp::XmppManager::initialize);
    m_xmppThread->start();
}

void AppContainer::shutdownXmpp()
{
    if (!m_xmppThread) {
        return;
    }

    // Release every service that shares the manager before stopping its event
    // loop, then destroy QXmppClient and its socket on XmppThread.
    m_container.clear();
    if (m_xmpp) {
        QMetaObject::invokeMethod(
            m_xmpp.get(),
            &core::xmpp::XmppManager::closeConnection,
            Qt::BlockingQueuedConnection);
        m_xmpp.reset();
    }

    m_xmppThread->quit();
    m_xmppThread->wait();

    delete m_xmppThread;
    m_xmppThread = nullptr;
}

void AppContainer::registerCoreService()
{
    m_container.registerSingleton<core::network::logging::AuthInterceptor> (
        [](ServiceContainer &) {
            return std::make_shared<core::network::logging::AuthInterceptor> ([] {
                const auto user = data::local::LoginResponseStore::getLocalUser();
                return user.has_value() ? user->access_token : QString {};
            });
        });

    m_container.registerSingleton<core::network::logging::LoggingInterceptor> (
        [](ServiceContainer &) {
            return std::make_shared<core::network::logging::LoggingInterceptor> ();
        });

    m_container.registerFactory<core::network::NetworkClient> ([](ServiceContainer &c) {
        auto client = std::make_shared<core::network::NetworkClient> (core::constants::AppConstants::baseUrl ());
        client->addInterceptor (c.resolve<core::network::logging::AuthInterceptor> ());
        client->addInterceptor (c.resolve<core::network::logging::LoggingInterceptor> ());
        return client;
    });
    m_container.registerSingleton<core::xmpp::XmppManager> ([this](ServiceContainer &) {
        return m_xmpp;
    });
}

void AppContainer::registerAppController()
{
    m_container.registerFactory<domain::repository::SessionRepository> ([](ServiceContainer &c) {
        return std::make_shared<data::repository::SessionRepositoryImpl> (
            c.resolve<core::xmpp::XmppManager> (),
            c.resolve<core::network::NetworkClient> ()
            );
    });
    m_container.registerFactory<domain::usecase::SessionUsecase> ([](ServiceContainer &c) {
        return std::make_shared<domain::usecase::SessionUsecase> (c.resolve<domain::repository::SessionRepository> ());
    });
    m_container.registerFactory<domain::usecase::LogoutUsecase> ([](ServiceContainer &c) {
        return std::make_shared<domain::usecase::LogoutUsecase> (c.resolve<domain::repository::SessionRepository> ());
    });
    m_container.registerSingleton<AppController> ([](ServiceContainer &c) {
        return std::make_shared<AppController> (
            c.resolve<domain::usecase::SessionUsecase> (),
            c.resolve<domain::usecase::LogoutUsecase> (),
            c.resolve<core::xmpp::XmppManager> ()
            );
    });
    AppController::setInstance (m_container.resolve<AppController> ().get ());
}

void AppContainer::registerAuthentication()
{
    m_container.registerFactory<domain::repository::LoginRepository>([](ServiceContainer &c) {
        return std::make_shared<data::repository::LoginRepositoryImpl> (
            c.resolve<core::xmpp::XmppManager> (),
            c.resolve<core::network::NetworkClient> ()
            );
    });
    m_container.registerFactory<domain::usecase::LoginUseCase> ([] (ServiceContainer &c) {
        return std::make_shared<domain::usecase::LoginUseCase> (c.resolve<domain::repository::LoginRepository> ());
    });
    m_container.registerSingleton<LoginVM> ([](ServiceContainer &c) {
        return std::make_shared<LoginVM> (c.resolve<domain::usecase::LoginUseCase> ());
    });

    LoginVM::setInstance(m_container.resolve<LoginVM>().get());
}

void AppContainer::registerChat()
{
    m_container.registerSingleton<domain::repository::MessageRepository> ([](ServiceContainer &c) {
        return std::make_shared<data::repository::MessageRepositoryImpl> (
            c.resolve<core::xmpp::XmppManager> (),
            c.resolve<core::network::NetworkClient> ()
            );
    });
    m_container.registerFactory<domain::usecase::MessageUsecase> ([](ServiceContainer &c) {
        return std::make_shared<domain::usecase::MessageUsecase> (c.resolve<domain::repository::MessageRepository> ());
    });
    m_container.registerFactory<domain::usecase::SendMessageUsecase> ([](ServiceContainer &c) {
        return std::make_shared<domain::usecase::SendMessageUsecase> (c.resolve<domain::repository::MessageRepository> ());
    });
    m_container.registerSingleton<MessagingViewModel> ([](ServiceContainer &c) {
        return std::make_shared<MessagingViewModel> (
            c.resolve<domain::usecase::MessageUsecase> (),
            c.resolve<domain::usecase::SendMessageUsecase> ()
            );
    });
    MessagingViewModel::setInstance (m_container.resolve<MessagingViewModel> ().get ());

    m_container.registerFactory<domain::repository::ConversationListRepository> ([](ServiceContainer &c) {
        return std::make_shared<data::repository::ConversationsRepositoryImpl> (c.resolve<core::network::NetworkClient> ());
    });
    m_container.registerFactory<domain::usecase::ConversationsUsecase> ([](ServiceContainer &c) {
        return std::make_shared<domain::usecase::ConversationsUsecase> (c.resolve<domain::repository::ConversationListRepository> ());
    });
    m_container.registerSingleton<ConversationsVM> ([](ServiceContainer &c) {
        return std::make_shared<ConversationsVM> (c.resolve<domain::usecase::ConversationsUsecase> ());
    });
    ConversationsVM::setInstance (m_container.resolve<ConversationsVM> ().get ());

    m_container.registerSingleton<HomeChatVM> ([](ServiceContainer &c) {
        return std::make_shared<HomeChatVM> (c.resolve<domain::repository::MessageRepository> ());
    });
    HomeChatVM::setInstance (m_container.resolve<HomeChatVM> ().get ());

}
