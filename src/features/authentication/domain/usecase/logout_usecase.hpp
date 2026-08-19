#ifndef FEATURES_AUTHENTICATION_DOMAIN_USECASE_LOGOUT_USECASE_HPP
#define FEATURES_AUTHENTICATION_DOMAIN_USECASE_LOGOUT_USECASE_HPP

#include <QObject>
#include "domain/repository/session_repository.hpp"

namespace domain {
namespace usecase {

class LogoutUsecase : public QObject
{
    Q_OBJECT
public:
    explicit LogoutUsecase(
        std::shared_ptr<domain::repository::SessionRepository> repository,
        QObject *parent = nullptr
        );
    void execute();

signals:
    void sessionChanged(bool session);
private:
    std::shared_ptr<domain::repository::SessionRepository> m_repository;
};

} // namespace usecase
} // namespace domain


#endif // FEATURES_AUTHENTICATION_DOMAIN_USECASE_LOGOUT_USECASE_HPP
