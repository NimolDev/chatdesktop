#ifndef FEATURES_AUTHENTICATION_DOMAIN_USECASE_LOGIN_USE_CASE_HPP
#define FEATURES_AUTHENTICATION_DOMAIN_USECASE_LOGIN_USE_CASE_HPP

// #include <data/login_repository_impl.hpp>
#include <login_repository.hpp>
#include <QObject>

namespace domain {
namespace usecase {

class LoginUseCase : public QObject
{
    Q_OBJECT
public:
    explicit LoginUseCase(
         std::shared_ptr<LoginRepository> repository,
        QObject *parent = nullptr
        );
    void execute(LoginRequest request);

signals:
    void loginSuccessed(domain::LoginResponse response);
    void loginFailed();
    void connecting();
    // void loginResponsed(domain::LoginResponse response);

private:
    std::shared_ptr<LoginRepository> m_repository;
};

} // namespace usecase
} // namespace domain


#endif // FEATURES_AUTHENTICATION_DOMAIN_USECASE_LOGIN_USE_CASE_HPP
