#ifndef CORE_NETWORK_AUTH_INTERCEPTOR_HPP
#define CORE_NETWORK_AUTH_INTERCEPTOR_HPP

#include "network_interceptor.hpp"
#include <functional>
#include <QString>

namespace core {
namespace network {
namespace logging {


class AuthInterceptor final: public INetworkInterceptor
{
public:
    using TokenProvider = std::function<QString()>;

    explicit AuthInterceptor(TokenProvider token_provider);

    // INetworkInterceptor interface
public:
    void interceptRequest(NetworkRequest &request) override;

private:
    TokenProvider m_tokenProvider;
};

} // namespace logging
} // namespace network
} // namespace core


#endif // CORE_NETWORK_AUTH_INTERCEPTOR_HPP
