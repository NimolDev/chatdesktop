#ifndef CORE_NETWORK_LOGGING_INTERCEPTOR_HPP
#define CORE_NETWORK_LOGGING_INTERCEPTOR_HPP

#include "network_interceptor.hpp"
#include <QSet>

namespace core {
namespace network {
namespace logging {

class LoggingInterceptor final : public INetworkInterceptor
{
public:
    explicit LoggingInterceptor( bool log_response_body = false);

    // INetworkInterceptor interface
public:
    void interceptRequest(NetworkRequest &request) override;
    void interceptResponse(const NetworkRequest &request, NetworkResponse &response) override;

private:
    [[nodiscard]]
    QByteArray redactedHeaderValue(
        const QByteArray &name,
        const QByteArray &value
        ) const;

    bool m_logResponseBody = false;

    QSet<QByteArray> m_sensitiveHeaders {
        QByteArrayLiteral ("authorization"),
        QByteArrayLiteral ("cookie"),
        QByteArrayLiteral ("set-cookie"),
        QByteArrayLiteral ("x-api-key")
    };
};

} // namespace logging
} // namespace network
} // namespace core


#endif // CORE_NETWORK_LOGGING_INTERCEPTOR_HPP
