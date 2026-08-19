#ifndef FEATURES_AUTHENTICATION_DOMAIN_USECASE_LOGIN_USE_CASE_HPP
#define FEATURES_AUTHENTICATION_DOMAIN_USECASE_LOGIN_USE_CASE_HPP

// #include <data/login_repository_impl.hpp>
#include "domain/entity/login_request.hpp"
#include "domain/repository/login_repository.hpp"
#include <QObject>

namespace domain {
namespace usecase {

class LoginUseCase : public QObject
{
    Q_OBJECT
public:
    explicit LoginUseCase(
         std::shared_ptr<domain::repository::LoginRepository> repository,
        QObject *parent = nullptr
        );
    void execute(domain::entity::LoginRequest request);

signals:
    void loginSucceded();
    void loginFailed();
    void connecting();
    void connected();


private:
    std::shared_ptr<domain::repository::LoginRepository> m_repository;
};

} // namespace usecase
} // namespace domain


#endif // FEATURES_AUTHENTICATION_DOMAIN_USECASE_LOGIN_USE_CASE_HPP
