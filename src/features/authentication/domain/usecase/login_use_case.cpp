#include "login_use_case.hpp"

namespace domain {
namespace usecase {

LoginUseCase::LoginUseCase(
    std::shared_ptr<LoginRepository> repository,
    QObject *parent
    )
    : m_repository(std::move (repository)),
    QObject(parent)
{
    connect(
        m_repository.get (),
        &domain::LoginRepository::loginSucceeded,
        this,
        &LoginUseCase::loginSuccessed
        );
    connect(
        m_repository.get (),
        &domain::LoginRepository::connecting,
        this,
        &LoginUseCase::connecting
        );
}

void LoginUseCase::execute(LoginRequest request)
{
    if (request.user_name.isEmpty ()) {
        emit loginFailed ();
    }
    m_repository->login (request);
}

} // namespace usecase
} // namespace domain
