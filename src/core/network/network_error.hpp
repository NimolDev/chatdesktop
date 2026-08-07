#ifndef CORE_NETWORK_NETWORK_ERROR_HPP
#define CORE_NETWORK_NETWORK_ERROR_HPP

#include <QString>

namespace core {
namespace network {
namespace error {

enum class NetworkError
{
    None,

    InvalideRequest,
    InvalideUrl,
    InvalidRequest = InvalideRequest,
    InvalidUrl = InvalideUrl,

    ConnectionRefused,
    RemoteHostClose,
    HostNotFound,
    OperationCancelled,
    Timeout,
    SslHandshakeFailed,
    NetworkSessionFailed,
    TemporaryNetworkFailure,

    BadRequest,
    Unauthorized,
    Forbidden,
    NotFound,
    Conflict,
    ValidationFailed,
    TooManyRequest,

    InternalServerError,
    BadGateway,
    ServiceUnavailable,
    GatewayTimeout,

    JsonParseError,
    EmptyResponse,

    UnknownNetworkError,
    UnknownHttpError
};

inline QString networkErrorToString(NetworkError error)
{
    switch (error) {

    case NetworkError::None:
        return QStringLiteral ("No error");
    case NetworkError::InvalideRequest:
        return QStringLiteral ("Invalid request");
    case NetworkError::InvalideUrl:
        return QStringLiteral ("Invalid URL");
    case NetworkError::ConnectionRefused:
        return QStringLiteral ("Connection refused");
    case NetworkError::RemoteHostClose:
        return QStringLiteral ("Remote host closed the connection");
    case NetworkError::HostNotFound:
        return QStringLiteral ("Host not found");
    case NetworkError::OperationCancelled:
        return QStringLiteral ("Request cancelled");
    case NetworkError::Timeout:
        return QStringLiteral ("Connection timeout");
    case NetworkError::SslHandshakeFailed:
        return QStringLiteral ("Ssl handshake failed");
    case NetworkError::NetworkSessionFailed:
        return QStringLiteral ("Network session failed");
    case NetworkError::TemporaryNetworkFailure:
        return QStringLiteral ("Temporary network failure");
    case NetworkError::BadRequest:
        return QStringLiteral ("Bad request");
    case NetworkError::Unauthorized:
        return QStringLiteral ("Unauthorized");
    case NetworkError::Forbidden:
        return QStringLiteral ("Forbidden");
    case NetworkError::NotFound:
        return QStringLiteral ("Not found");
    case NetworkError::Conflict:
        return QStringLiteral ("Conflict");
    case NetworkError::ValidationFailed:
        return QStringLiteral ("Validation failed");
    case NetworkError::TooManyRequest:
        return QStringLiteral ("Too many requests");
    case NetworkError::InternalServerError:
        return QStringLiteral ("Internal server error");
    case NetworkError::BadGateway:
        return QStringLiteral ("Bad Gateway");
    case NetworkError::ServiceUnavailable:
        return QStringLiteral ("Service unavailable");
    case NetworkError::GatewayTimeout:
        return QStringLiteral ("Gateway timeout");
    case NetworkError::JsonParseError:
        return QStringLiteral ("Invalid JSON response");
    case NetworkError::EmptyResponse:
        return QStringLiteral ("Empty response");
    case NetworkError::UnknownNetworkError:
        return QStringLiteral ("Unknown network error");
    case NetworkError::UnknownHttpError:
        return QStringLiteral ("Unknown HTTP error");
    }
    return QStringLiteral ("Unknown error");
}

} // namespace error
} // namespace network
} // namespace core
#endif // CORE_NETWORK_NETWORK_ERROR_HPP
