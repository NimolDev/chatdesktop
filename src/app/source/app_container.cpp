#include "app_container.hpp"
#include "app_controller.hpp"

#include "voip/signaling/jingle_service.hpp"
#include "voip/data/call_repository_impl.hpp"
#include "voip/domain/usecase/call_use_case.hpp"
#include "voip/presentation/viewmodel/call_vm.hpp"

#include "constants/app_constants.hpp"

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
#include "chat/data/repository/media_repository_impl.hpp"
#include "chat/domain/usecase/message_usecase.hpp"
#include "chat/presentation/viewmodel/messaging_vm.hpp"
#include "repository/conversations_repository_impl.hpp"



#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlEngine>
#include <QtQml/qqml.h>



AppContainer::AppContainer()
{

    setupXmpp();

    registerCoreService ();
    registerAppController ();
    registerAuthentication ();
    registerChat();
    registerVoip();

    qmlRegisterSingletonInstance("App.Services", 1, 0, "AppContainer", this);
}

AppContainer::~AppContainer()
{
    shutdownXmpp ();
}

QObject *AppContainer::ownViewModel(QObject *viewModel)
{
    QQmlEngine::setObjectOwnership(viewModel, QQmlEngine::CppOwnership);
    m_pageViewModels.insert(viewModel);
    connect(viewModel, &QObject::destroyed, this, [this, viewModel]() {
        m_pageViewModels.remove(viewModel);
    });
    return viewModel;
}

QObject *AppContainer::createLoginVM(QObject *page)
{
    if (!page) {
        return nullptr;
    }
    return ownViewModel(new LoginVM(
        m_container.resolve<domain::usecase::LoginUseCase>(), page));
}

QObject *AppContainer::createMessagingVM(QObject *page)
{
    if (!page) {
        return nullptr;
    }
    return ownViewModel(new MessagingViewModel(
        m_container.resolve<domain::usecase::MessageUsecase>(),
        m_container.resolve<domain::usecase::SendMessageUsecase>(),
        page,
        m_container.resolve<domain::repository::MediaRepository>()));
}

QObject *AppContainer::createConversationVM(QObject *page)
{
    return ownViewModel (new ConversationsVM(
        m_container.resolve<domain::usecase::ConversationsUsecase>(),
        page));
}

void AppContainer::destroyViewModel(QObject *viewModel)
{
    if (!m_pageViewModels.remove(viewModel)) {
        return;
    }
    // Defer deletion so callers can safely finish handling a VM signal.
    viewModel->deleteLater();
}

void AppContainer::setupXmpp()
{
    m_xmppThread = new QThread();
    m_xmppThread->setObjectName(QStringLiteral("XmppThread"));

    auto *xmpp = new core::xmpp::XmppManager();
    m_jingleService = new voip::signaling::JingleService(xmpp);
    QObject::connect(xmpp, &core::xmpp::XmppManager::clientInitialized,
                     m_jingleService, &voip::signaling::JingleService::initialize);
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
        auto client = std::make_shared<core::network::NetworkClient>
            (core::constants::AppConstants::baseUrl ());
        client->addInterceptor (c.resolve<core::network::logging::AuthInterceptor> ());
        client->addInterceptor (c.resolve<core::network::logging::LoggingInterceptor> ()); 
        return client;
    });
    m_container.registerSingleton<core::xmpp::XmppManager> ([this](ServiceContainer &) {
        return m_xmpp;
    });
    m_container.registerSingleton<voip::signaling::JingleService>([this](ServiceContainer &) {
        // Keep the owning manager alive while a feature uses the service.
        return std::shared_ptr<voip::signaling::JingleService>(m_xmpp, m_jingleService);
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

}

void AppContainer::registerChat()
{
    m_container.registerSingleton<domain::repository::MediaRepository>([](ServiceContainer &) {
        return std::make_shared<data::repository::MediaRepositoryImpl>();
    });
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
    m_container.registerFactory<domain::repository::ConversationListRepository> ([](ServiceContainer &c) {
        return std::make_shared<data::repository::ConversationsRepositoryImpl> (c.resolve<core::network::NetworkClient> ());
    });
    m_container.registerFactory<domain::usecase::ConversationsUsecase> ([](ServiceContainer &c) {
        return std::make_shared<domain::usecase::ConversationsUsecase> (c.resolve<domain::repository::ConversationListRepository> ());
    });
    m_container.registerFactory<ConversationsVM> ([](ServiceContainer &c) {
        return std::make_shared<ConversationsVM> (c.resolve<domain::usecase::ConversationsUsecase> ());
    });



}

void AppContainer::registerVoip()
{
    m_container.registerSingleton<domain::CallRepository>([](ServiceContainer &c) {
        return std::make_shared<data::CallRepositoryImpl>(
            c.resolve<voip::signaling::JingleService>());
    });
    m_container.registerSingleton<domain::CallUseCase>([](ServiceContainer &c) {
        return std::make_shared<domain::CallUseCase>(c.resolve<domain::CallRepository>());
    });
    m_container.registerSingleton<CallVM>([](ServiceContainer &c) {
        return std::make_shared<CallVM> (c.resolve<domain::CallUseCase> ());
    });
    CallVM::setInstance(m_container.resolve<CallVM>().get());
}

