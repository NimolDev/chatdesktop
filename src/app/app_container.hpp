#ifndef APP_APP_CONTAINER_HPP
#define APP_APP_CONTAINER_HPP

#include <memory.h>
#include <service_container.hpp>



class QQmlApplicationEngine;

class AppContainer
{

public:
    explicit AppContainer(QQmlApplicationEngine *engine);
    ServiceContainer &container() { return m_container;};

private:
    void registerCoreService();
    void registerAppController();
    void registerQmlSignleton();
    void registerAuthentication();
    void registerChat();


    ServiceContainer m_container;
    QQmlApplicationEngine *m_engine;
};

#endif // APP_APP_CONTAINER_HPP
