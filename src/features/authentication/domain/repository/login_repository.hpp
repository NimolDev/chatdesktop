#ifndef FEATURES_AUTHENTICATION_DOMAIN_LOGIN_REPOSITORY_HPP
#define FEATURES_AUTHENTICATION_DOMAIN_LOGIN_REPOSITORY_HPP

#include <QObject>
#include"entity/login_request.hpp"


namespace domain {
namespace repository {

class LoginRepository : public QObject
{
    Q_OBJECT
public:
    explicit LoginRepository(QObject *parent = nullptr) : QObject(parent)
    {};

    ~LoginRepository() = default;
    virtual void login(entity::LoginRequest request) = 0;
    virtual void connectToXmpp() = 0;

signals:

    void loginSucceded();
    void loginFailed();
    void connecting();
    void connected();
    void disConnented();

};

} // namespace domain
} // namespace repository



#endif // FEATURES_AUTHENTICATION_DOMAIN_LOGIN_REPOSITORY_HPP
