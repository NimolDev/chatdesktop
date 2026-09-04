#ifndef CORE_WEBRTC_SDP_OBSERVER_HPP
#define CORE_WEBRTC_SDP_OBSERVER_HPP

#include <functional>
#include <memory>

#include "api/jsep.h"
namespace core {
namespace rtc {
class SdpObserver  : public webrtc::CreateSessionDescriptionObserver
{
public:
    using SuccessHandler = std::function<void(
        std::unique_ptr<webrtc::SessionDescriptionInterface>)>;
    using FailureHandler = std::function<void(const webrtc::RTCError &)>;

    explicit SdpObserver(
        SuccessHandler onSuccess = {},
        FailureHandler onFailure = {});



    // SetSessionDescriptionObserver interface
public:
    void OnSuccess(webrtc::SessionDescriptionInterface *desc) override;
    void OnFailure(webrtc::RTCError error) override;

private:
    SuccessHandler m_onSuccess;
    FailureHandler m_onFailure;
};

} // namespace rtc
} // namespace core


#endif // CORE_WEBRTC_SDP_OBSERVER_HPP
