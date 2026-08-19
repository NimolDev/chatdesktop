#include "logout_usecase.hpp"
#include <QtCore/qdebug.h>

namespace domain {
namespace usecase {

LogoutUsecase::LogoutUsecase(
    std::shared_ptr<repository::SessionRepository> repository,
    QObject *parent)
    : m_repository(std::move (repository)),
    QObject(parent)
{
    // connect (
    //     m_repository.get (),
    //     &domain::repository::SessionRepository::sessionChanged,
    //     this,
    //     &domain::usecase::LogoutUsecase::sessionChanged
    // );

}

void LogoutUsecase::execute()
{
    bool logout = m_repository->logout ();
    qDebug() << "Logout usecase" << logout;
    emit sessionChanged (!logout);

}


} // namespace usecase
} // namespace domain
