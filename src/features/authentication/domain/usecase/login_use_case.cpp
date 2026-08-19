#include "login_use_case.hpp"

namespace domain {
namespace usecase {

LoginUseCase::LoginUseCase(
    std::shared_ptr<domain::repository::LoginRepository> repository,
    QObject *parent
    )
    : m_repository(std::move (repository)),
    QObject(parent)
{
    // connect(
    //     m_repository.get (),
    //     &domain::repository::LoginRepository::loginSucceeded,
    //     this,
    //     &LoginUseCase::loginSuccessed
    //     );
    connect(
        m_repository.get (),
        &domain::repository::LoginRepository::connecting,
        this,
        &LoginUseCase::connecting
        );
    connect (
        m_repository.get (),
        &domain::repository::LoginRepository::connected,
        this,
        &LoginUseCase::connected
        );
    connect (
        m_repository.get (),
        &domain::repository::LoginRepository::loginSucceded,
        this,
        &LoginUseCase::loginSucceded
        );
    connect(
        m_repository.get (),
        &domain::repository::LoginRepository::loginFailed,
        this,
        &LoginUseCase::loginFailed
        );
//     connect(
//         m_repository.get (),
//         &domain::repository::LoginRepository::disConnented,
//         this,
//         &LoginUseCase::disconnect
//         );
}

void LoginUseCase::execute(domain::entity::LoginRequest request)
{
    if (request.username.isEmpty ()) {
        emit loginFailed ();
    }
    m_repository->login (request);
}

} // namespace usecase
} // namespace domain
