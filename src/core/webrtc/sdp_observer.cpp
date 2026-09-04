#include "sdp_observer.hpp"
#include <QDebug>

namespace core {
namespace rtc {

SdpObserver::SdpObserver(SuccessHandler onSuccess, FailureHandler onFailure)
    : m_onSuccess(std::move(onSuccess)),
      m_onFailure(std::move(onFailure))
{
}

void SdpObserver::OnSuccess(webrtc::SessionDescriptionInterface *desc)
{
    std::unique_ptr<webrtc::SessionDescriptionInterface> description(desc);
    if (m_onSuccess) {
        m_onSuccess(std::move(description));
    }
    qWarning() << "SDP :" << desc->ToString ();
}

void SdpObserver::OnFailure(webrtc::RTCError error)
{
    if (m_onFailure) {
        m_onFailure(error);
        return;
    }

    qWarning() << "SDP error:" << QString::fromUtf8(error.message());
}


} // namespace rtc
} // namespace core
