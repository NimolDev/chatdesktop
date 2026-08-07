#ifndef FEATURES_AUTHENTICATION_DOMAIN_LOGIN_REPOSITORY_HPP
#define FEATURES_AUTHENTICATION_DOMAIN_LOGIN_REPOSITORY_HPP

#include <login_response.hpp>
#include <login_request.hpp>
#include <QObject>

namespace domain {

class LoginRepository : public QObject
{
    Q_OBJECT
public:
    explicit LoginRepository(QObject *parent = nullptr) : QObject(parent)
    {};

    ~LoginRepository() = default;
    virtual void login(domain::LoginRequest request) = 0;

signals:
    void loginSucceeded(domain::LoginResponse response);
    void loginFailed();
    void connecting();
    void disConnented();

};

} // namespace domain



#endif // FEATURES_AUTHENTICATION_DOMAIN_LOGIN_REPOSITORY_HPP
