#include "call_coordinator.hpp"
#include "app_container.hpp"
#include "app_controller.hpp"
#include "logger.hpp"
#include "voip/presentation/viewmodel/call_vm.hpp"

#include <QQmlComponent>
#include <QQmlEngine>
#include <QVariantMap>
#include <QWindow>
#include <QtQml/qqmlinfo.h>


CallCoordinator::CallCoordinator(AppContainer &appContainer, QQmlEngine &engine, QObject *parent)
    : QObject(parent)
    , m_engine(&engine)
{
    auto &container = appContainer.container();
    setCallViewModel(container.resolve<CallVM>().get());
    setChatViewModel(container.resolve<MessagingViewModel>().get());
    auto *controller = container.resolve<AppController>().get();
    connect(controller, &AppController::stateChanged, this, [this, controller] {
        if (controller->state() == AppController::AppState::Unauthenticated
            || controller->state() == AppController::AppState::Logout) {
            reset();
        }
    });

}

void CallCoordinator::setCallViewModel(CallVM *viewModel)
{
    if (m_callViewModel == viewModel) {
        return;
    }
    disconnect(m_callConnection);
    reset();
    if (m_callViewModel) {
        disconnect(m_callViewModel, nullptr, this, nullptr);
    }
    m_callViewModel = viewModel;
    if (!viewModel) {
        return;
    }
    if (m_chatViewModel) {
        m_callConnection = connect(m_chatViewModel, &MessagingViewModel::callRequested,
                                   viewModel, &CallVM::requestCall);
    }
    connect(m_callViewModel, &CallVM::callWindowRequested,
            this, [this](const QString &receiverId, const QString &userName) {
                showCallWindow(receiverId, userName);
            });
    connect(m_callViewModel, &CallVM::callFailed, this, [this](const QString &reason) {
        if (m_callWindow) {
            // Errors during negotiation must not navigate away from the call.
            m_callWindow->setProperty("signalingError", reason);
        }
    });
    connect(m_callViewModel,
            &CallVM::proposeReceived,
            this,
            [this](const QString &sender,
                   bool video) {
                LOG_INFO("Incoming call");
                showCallWindow(sender, sender.section(QLatin1Char('/'), 0, 0), true, video);
            });
}


CallCoordinator::~CallCoordinator()
{
    disconnect(m_callConnection);
    reset();
}

void CallCoordinator::setChatViewModel(MessagingViewModel *viewModel)
{
    if (m_chatViewModel == viewModel) {
        return;
    }
    disconnect(m_callConnection);
    reset();
    m_chatViewModel = viewModel;
    if (viewModel && m_callViewModel) {
        m_callConnection = connect(viewModel, &MessagingViewModel::callRequested,
                                   m_callViewModel, &CallVM::requestCall);
    }
}

void CallCoordinator::showCallWindow(const QString &receiverId, const QString &userName,
                                     bool incoming, bool video)
{
    if (receiverId.trimmed().isEmpty()) {
        return;
    }

    if (!m_callWindow) {
        auto *engine = m_engine.data();
        if (!engine) {
            qmlWarning(this) << "Cannot open call window without a QML engine";
            return;
        }

        QQmlComponent component(engine);
        component.loadFromModule("Features.Voip", "CallPage", QQmlComponent::PreferSynchronous);
        if (!component.isReady()) {
            qmlWarning(this) << "Cannot load call window:" << component.errorString();
            return;
        }

        QObject *object = component.createWithInitialProperties({
            {QStringLiteral("visible"), false},
            {QStringLiteral("receiverId"), receiverId},
            {QStringLiteral("userName"), userName}
        });
        auto *window = qobject_cast<QWindow *>(object);
        if (!window) {
            qmlWarning(this) << "CallPage must create a Window:" << component.errorString();
            delete object;
            return;
        }

        QQmlEngine::setObjectOwnership(window, QQmlEngine::CppOwnership);
        window->QObject::setParent(this);
        m_callWindow = window;
        m_window.setup(window);
        connect(window, SIGNAL(startCallRequested(bool)),
                m_callViewModel, SLOT(startCall(bool)));
#ifdef Q_OS_MACOS
        // _isPinned is declared by the dynamically loaded QML window.
        connect(window, SIGNAL(_isPinnedChanged()),
                this, SLOT(updateCallWindowPin()));
#endif
    }

    m_callWindow->setProperty("receiverId", receiverId);
    m_callWindow->setProperty("userName", userName);
    m_callWindow->setProperty("signalingError", QString());
    m_callWindow->setProperty("_isStartCall", false);
    m_callWindow->setProperty("incomingCall", incoming);
    m_callWindow->setProperty("incomingVideo", video);

    m_window.setup(m_callWindow);
    m_window.setWindowFillContent ();
    m_callWindow->showNormal();
    updateCallWindowPin();
    m_callWindow->raise();
    m_callWindow->requestActivate();
}

void CallCoordinator::updateCallWindowPin()
{
#ifdef Q_OS_MACOS
    if (m_callWindow) {
        m_window.pineWindow (m_callWindow->property("_isPinned").toBool());
    }
#endif
}

void CallCoordinator::reset()
{
    if (m_callViewModel) {
        m_callViewModel->reset();
    }
    // Clearing the pointer first also makes repeated resets harmless.
    auto *window = m_callWindow.data();
    m_callWindow.clear();
    delete window;
}
