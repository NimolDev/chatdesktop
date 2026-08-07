#ifndef CORE_NETWORK_RETRY_POLICY_HPP
#define CORE_NETWORK_RETRY_POLICY_HPP

#include "http_method.hpp"
#include "network_error.hpp"

#include <QSet>
#include <QtGlobal>

namespace core {
namespace network {

using namespace error;

struct RetryPolicy
{
    // Total retries after the first attempt.
    int maximum_retries = 2;

    // First retyr delay.
    int initial_delay_ms = 500;

    // Maximum allowed retry delay.
    int maximum_delay_ms = 5000;

    /*
     *  Example:
     *  500 ms -> 1000 ms -> 2000 ms
     */
    double back_off_mulitplier = 2.0;

    // Avoid retry PATCH storms when may clients fail simultaneously.
    bool use_jitter = true;

    // POST and PATCH are not automatically retried by default.
    bool retry_non_idempotent_reqeusts = false;

    QSet<NetworkError> retryable_errors {
        NetworkError::ConnectionRefused,
        NetworkError::RemoteHostClose,
        NetworkError::Timeout,
        NetworkError::TemporaryNetworkFailure,
        NetworkError::BadGateway,
        NetworkError::ServiceUnavailable,
        NetworkError::GatewayTimeout,
    };


    bool shouldRetry(
        HttpMethod method,
        NetworkError error,
        int completed_retries
        ) const
    {
        if (completed_retries >= maximum_retries) {
            return false;
        }
        if (!retryable_errors.contains (error)) {
            return false;
        }
        if (!retry_non_idempotent_reqeusts && !isIdempotentHttpMethod (method)) {
            return false;
        }
        return true;
    }

    int delayForRetry(int retry_number) const
    {
        // retry_number start at zero.
        double delay = static_cast<double>(initial_delay_ms);

        for (int i = 0; i < retry_number; i++) {
            delay *= back_off_mulitplier;
        }

        return qMin(
            static_cast<int>(delay),
            maximum_delay_ms
            );
    }

};

} // namespace network
} // namespace core

#endif // CORE_NETWORK_RETRY_POLICY_HPP
