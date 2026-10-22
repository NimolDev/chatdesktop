#ifndef APP_LOGIN_VM_FACTORY_HPP
#define APP_LOGIN_VM_FACTORY_HPP

#include "authentication/presentation/viewmodel/login_vm.hpp"
#include "service_container.hpp"
#include <QQmlEngine>
#include <QSet>

class LoginVMFactory : public QObject
{
    Q_OBJECT

public:
    explicit LoginVMFactory(ServiceContainer &container, QObject *parent = nullptr)
        : QObject(parent), m_container(container)
    {
    }

    Q_INVOKABLE QObject *createLoginVM(QObject *page)
    {
        if (!page) {
            return nullptr;
        }
        auto *viewModel = new LoginVM(
            m_container.resolve<domain::usecase::LoginUseCase>(), page);
        QQmlEngine::setObjectOwnership(viewModel, QQmlEngine::CppOwnership);

        return viewModel;
    }



private:
    ServiceContainer &m_container;
};

#endif // APP_LOGIN_VM_FACTORY_HPP
