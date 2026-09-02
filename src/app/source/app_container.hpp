#ifndef APP_APP_CONTAINER_HPP
#define APP_APP_CONTAINER_HPP

#include <memory>
#include <QThread>

#include "service_container.hpp"
#include "xmpp/xmpp_manager.hpp"

class AppContainer
{

public:
    AppContainer();
    ~AppContainer();
    ServiceContainer &container() { return m_container;};

private:

    void setupXmpp();
    void shutdownXmpp();
    void registerCoreService();
    void registerAppController();
    void registerAuthentication();
    void registerChat();

    ServiceContainer m_container;

    QThread *m_xmppThread = nullptr;
    std::shared_ptr<core::xmpp::XmppManager> m_xmpp;
};

#endif // APP_APP_CONTAINER_HPP
