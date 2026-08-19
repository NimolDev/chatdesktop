#ifndef FEATURES_AUTHENTICATION_DATA_REPOSITORY_SESSION_REPOSITORY_IMPL_HPP
#define FEATURES_AUTHENTICATION_DATA_REPOSITORY_SESSION_REPOSITORY_IMPL_HPP

#include "xmpp/xmpp_manager.hpp"
#include "domain/repository/session_repository.hpp"
#include "dto/local_user.hpp"

#include <memory>
#include <optional>

namespace data {
namespace repository {

class SessionRepositoryImpl final : public domain::repository::SessionRepository
{
    Q_OBJECT
public:
    explicit SessionRepositoryImpl(
        std::shared_ptr<core::xmpp::XmppManager> xmpp,
        QObject *parent = nullptr
        );

    bool sessionChecked() const override;
    bool isSession() const override;
    bool logout() override;

private:
    std::shared_ptr<core::xmpp::XmppManager> m_xmpp;
    std::optional<dto::LocalUser> m_user;
    bool m_sessionChecked = false;
};

} // namespace repository
} // namespace data


#endif // FEATURES_AUTHENTICATION_DATA_REPOSITORY_SESSION_REPOSITORY_IMPL_HPP
