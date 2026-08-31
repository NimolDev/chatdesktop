#ifndef FEATURES_AUTHENTICATION_DOMAIN_REPOSITORY_SESSION_REPOSITORY_HPP
#define FEATURES_AUTHENTICATION_DOMAIN_REPOSITORY_SESSION_REPOSITORY_HPP

#include <QObject>
#include <QFuture>

namespace domain {
namespace repository {

class SessionRepository : public QObject
{
    Q_OBJECT
public:
    explicit SessionRepository(QObject *parent = nullptr) : QObject(parent) {};

    ~SessionRepository() override = default;
    virtual QFuture<bool> sessionChecked() = 0;
    virtual bool isSession() const = 0;
    virtual bool logout() = 0;
    virtual void connectXmpp() = 0;

signals:
    void sessionChanged(bool logged);
};

} // namespace repository
} // namespace domain


#endif // FEATURES_AUTHENTICATION_DOMAIN_REPOSITORY_SESSION_REPOSITORY_HPP
