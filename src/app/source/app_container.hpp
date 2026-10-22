#ifndef APP_APP_CONTAINER_HPP
#define APP_APP_CONTAINER_HPP

#include <memory>
#include <QThread>
#include <QObject>
#include <QSet>

#include "service_container.hpp"
#include "xmpp/xmpp_manager.hpp"

class QQmlEngine;

namespace voip {
namespace signaling { class JingleService; }
}

class AppContainer : public QObject
{
    Q_OBJECT

public:
    AppContainer();
    ~AppContainer() override;

    Q_INVOKABLE QObject *createLoginVM(QObject *page);
    Q_INVOKABLE QObject *createMessagingVM(QObject *page);
    Q_INVOKABLE QObject *createConversationVM(QObject *page);
    Q_INVOKABLE void destroyViewModel(QObject *viewModel);

    ServiceContainer &container() { return m_container;};


private:

    void setupXmpp();
    void shutdownXmpp();
    void registerCoreService();
    void registerAppController();
    void registerAuthentication();
    void registerChat();
    void registerVoip();

    QObject *ownViewModel(QObject *viewModel);

    ServiceContainer m_container;
    QSet<QObject *> m_pageViewModels;

    QThread *m_xmppThread = nullptr;
    std::shared_ptr<core::xmpp::XmppManager> m_xmpp;
    voip::signaling::JingleService *m_jingleService = nullptr;
};

#endif // APP_APP_CONTAINER_HPP
