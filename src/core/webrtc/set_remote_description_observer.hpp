#ifndef CORE_WEBRTC_SET_REMOTE_DESCRIPTION_OBSERVER_HPP
#define CORE_WEBRTC_SET_REMOTE_DESCRIPTION_OBSERVER_HPP

#include "api/jsep.h"

namespace core {
namespace rtc {

class SetRemoteDescriptionObserver: public webrtc::SetSessionDescriptionObserver
{
public:
SetRemoteDescriptionObserver() = default;

public:
    void OnSuccess() override;
    void OnFailure(webrtc::RTCError error) override;

private:

};



} // namespace rtc

} // namespace core

#endif // CORE_WEBRTC_SET_REMOTE_DESCRIPTION_OBSERVER_HPP
