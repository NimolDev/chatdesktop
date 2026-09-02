#include "session_usecase.hpp"

namespace domain {
namespace usecase {

SessionUsecase::SessionUsecase(
    std::shared_ptr<repository::SessionRepository> repository,
    QObject *parent)
    : m_repository(std::move (repository)),
    QObject(parent)
{

    // connect (
    //     m_repository.get (),
    //     &domain::repository::SessionRepository::sessionChanged,
    //     this,
    //     &domain::usecase::SessionUsecase::sessionChanged
    //     );
}

void SessionUsecase::execute()
{
    m_repository->sessionChecked().then(this, [this](bool isSession) {
        m_repository->connectToXmpp ();
        emit sessionChanged(isSession);
    });
}


} // namespace usecase
} // namespace domain
