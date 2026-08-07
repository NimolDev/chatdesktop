#ifndef CORE_NETWORK_HTTP_METHOD_HPP
#define CORE_NETWORK_HTTP_METHOD_HPP

#include <QString>

namespace core {
namespace network {


enum class HttpMethod {
    Get,
    Post,
    Put,
    Patch,
    Delete,
    Head
};


inline QString httpMethodToString(HttpMethod method)
{
    switch (method) {

    case HttpMethod::Get:
        return QStringLiteral ("GET");
    case HttpMethod::Post:
        return QStringLiteral ("POST");
    case HttpMethod::Put:
        return QStringLiteral ("PUT");
    case HttpMethod::Patch:
        return QStringLiteral ("PATCH");
    case HttpMethod::Delete:
        return QStringLiteral ("DELETE");
    case HttpMethod::Head:
        return QStringLiteral ("HEAD");
    }
    return QStringLiteral ("UNKNOWN");
};

inline bool isIdempotentHttpMethod(HttpMethod method)
{
    switch (method) {

    case HttpMethod::Get:
    case HttpMethod::Put:
    case HttpMethod::Delete:
    case HttpMethod::Head:
        return true;
    case HttpMethod::Post:
    case HttpMethod::Patch:
        return false;
    }
    return false;
};

} // namespace network
} // namespace core

#endif // CORE_NETWORK_HTTP_METHOD_HPP
