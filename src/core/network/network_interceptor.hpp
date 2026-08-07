#ifndef CORE_NETWORK_NETWORK_INTERCEPTOR_HPP
#define CORE_NETWORK_NETWORK_INTERCEPTOR_HPP

#include "network_request.hpp"
#include "network_response.hpp"

namespace core {
namespace network {

class INetworkInterceptor
{
public:
    virtual ~INetworkInterceptor() = default;

    virtual void interceptRequest(NetworkRequest &request) { Q_UNUSED (request); }

    virtual void interceptResponse(
        const NetworkRequest &request,
        NetworkResponse &response
        )
    {
        Q_UNUSED (request);
        Q_UNUSED (response);
    }

    virtual void interceptorResponse(
        const NetworkRequest &request,
        NetworkResponse &response
        )
    {
        interceptResponse(request, response);
    }
};

} // namespace network
} // namespace core



#endif // CORE_NETWORK_NETWORK_INTERCEPTOR_HPP
