#include "set_remote_description_observer.hpp"

#include <QDebug>

void core::rtc::SetRemoteDescriptionObserver::OnSuccess()
{
    qDebug() << "Remote session description set succeessfully";
}

void core::rtc::SetRemoteDescriptionObserver::OnFailure(webrtc::RTCError error)
{
    qCritical() << "Remote sessiond descripotion set failed: "
                << error.message ();
}
