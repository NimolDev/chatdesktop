#include "logging_interceptor.hpp"
#include "http_method.hpp"
#include <QDebug>

namespace core {
namespace network {
namespace logging {


LoggingInterceptor::LoggingInterceptor(bool log_response_body)
    : m_logResponseBody(log_response_body)
{}

void LoggingInterceptor::interceptRequest(NetworkRequest &request)
{
    qInfo().noquote ()
     << QStringLiteral (
           "[HTTP] --> %1 %2 requestId=%3 operation=%4"
           )
            .arg (
                httpMethodToString (request.method),
                request.resolvedUrl ().toString (),
                request.request_id,
                request.operation_name
            );

    for (auto iterator = request.headers.cbegin ();
         iterator != request.headers.cend ();
         ++iterator) {
        qDebug().noquote ()
         << "[HTTP] Header: "
         << iterator.key()
         << "="
         << redactedHeaderValue (
                iterator.key (),
                iterator.value ()
            );
    }

#ifndef QT_NO_DEBUG
    if (!request.body.isEmpty ()) {
        qDebug().noquote ()
            << "[HTTP] Request body: "
            << request.body;

    }
#endif
}

void LoggingInterceptor::interceptResponse(const NetworkRequest &request, NetworkResponse &response)
{
    qInfo().noquote ()
    << QStringLiteral (
           "[HTTP] <-- %1 %2 status=%3 duration=%4ms "
           "attempts=%5 error=%6 requestId=%7"
           )
            .arg (
                httpMethodToString (request.method),
                request.resolvedUrl ().toString ()

                )
            .arg(response.status_code)
            .arg(response.duration_ms)
            .arg(response.attempt_count)
            .arg(
                networkErrorToString (response.error),
                response.request_id
                );

#ifndef QT_NO_DEBUG
    if (m_logResponseBody && !response.body.isEmpty ()) {
        qDebug().noquote ()
        << "[HTTP] Response body: "
        << response.body;
    }
#endif
}

QByteArray LoggingInterceptor::redactedHeaderValue(const QByteArray &name, const QByteArray &value) const
{
    if (m_sensitiveHeaders.contains (name.toLower ())) {
        return QByteArrayLiteral ("<redacted>");
    }
    return value;
}


} // namespace logging
} // namespace network
} // namespace core