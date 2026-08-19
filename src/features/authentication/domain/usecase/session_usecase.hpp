#ifndef FEATURES_AUTHENTICATION_DOMAIN_USECASE_SESSION_USECASE_HPP
#define FEATURES_AUTHENTICATION_DOMAIN_USECASE_SESSION_USECASE_HPP

#include <QObject>

#include "repository/session_repository.hpp"

namespace domain {
namespace usecase {

class SessionUsecase : public QObject
{
    Q_OBJECT
public:
    explicit SessionUsecase(
        std::shared_ptr<domain::repository::SessionRepository> repository,
        QObject *parent = nullptr
        );

    void execute();
signals:
    void sessionChanged(bool session);
private:
    std::shared_ptr<domain::repository::SessionRepository> m_repository;
};


} // namespace session
} // namespace doamin


#endif // FEATURES_AUTHENTICATION_DOMAIN_USECASE_SESSION_USECASE_HPP
