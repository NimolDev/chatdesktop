#pragma once

#include <QObject>
#include <QPointer>
#include <QString>
#include "chat/presentation/viewmodel/messaging_vm.hpp"

#include "platform/platform_main_window.hpp"

class QWindow;
class QQmlEngine;
class CallVM;
class AppContainer;

class CallCoordinator : public QObject
{
    Q_OBJECT

public:
    explicit CallCoordinator(AppContainer &appContainer, QQmlEngine &engine,
                             QObject *parent = nullptr);
    ~CallCoordinator() override;

    MessagingViewModel *chatViewModel() const { return m_chatViewModel; }
    void setChatViewModel(MessagingViewModel *viewModel);
    void reset();

private slots:
    void updateCallWindowPin();

private:
    void setCallViewModel(CallVM *viewModel);
    void showCallWindow(const QString &receiverId, const QString &userName,
                        bool incoming = false, bool video = false);
    QPointer<QQmlEngine> m_engine;
    QPointer<MessagingViewModel> m_chatViewModel;
    QMetaObject::Connection m_callConnection;
    QPointer<CallVM> m_callViewModel;
    QPointer<QWindow> m_callWindow;

    core::platform::PlatformMainWindow m_window;
};
