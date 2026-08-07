#include "auth_interceptor.hpp"
#include <utility>

namespace core {
namespace network {

AuthInterceptor::AuthInterceptor(TokenProvider token_provider)
    : m_tokenProvider(std::move (token_provider))
{}

void AuthInterceptor::interceptRequest(NetworkRequest &request)
{
    if (!request.requires_authentication) {
        return;
    }
    if (!m_tokenProvider) {
        return;
    }
    const QString token = m_tokenProvider().trimmed ();

    if (token.isEmpty ()) {
        return;
    }

    request.headers.insert (
        QByteArrayLiteral ("Authorization"),
        QByteArrayLiteral ("Bearer ") + token.toUtf8 ()
        );
}



} // namespace network
} // namespace core

