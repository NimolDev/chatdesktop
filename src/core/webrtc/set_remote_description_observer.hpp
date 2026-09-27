#ifndef CORE_WEBRTC_SET_REMOTE_DESCRIPTION_OBSERVER_HPP
#define CORE_WEBRTC_SET_REMOTE_DESCRIPTION_OBSERVER_HPP

#include "api/jsep.h"

#include <functional>
#include <utility>

namespace core {
namespace rtc {

class SetRemoteDescriptionObserver: public webrtc::SetSessionDescriptionObserver
{
public:
    explicit SetRemoteDescriptionObserver(std::function<void()> onSuccess = {})
        : m_onSuccess(std::move(onSuccess))
    {
    }

public:
    void OnSuccess() override;
    void OnFailure(webrtc::RTCError error) override;

private:
    std::function<void()> m_onSuccess;

};



} // namespace rtc

} // namespace core

#endif // CORE_WEBRTC_SET_REMOTE_DESCRIPTION_OBSERVER_HPP
