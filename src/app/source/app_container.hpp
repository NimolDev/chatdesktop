#ifndef APP_APP_CONTAINER_HPP
#define APP_APP_CONTAINER_HPP

#include <memory.h>
#include "service_container.hpp"

class AppContainer
{

public:
    AppContainer();
    ServiceContainer &container() { return m_container;};

private:
    void registerCoreService();
    void registerAppController();
    void registerAuthentication();
    void registerChat();

    ServiceContainer m_container;
};

#endif // APP_APP_CONTAINER_HPP
