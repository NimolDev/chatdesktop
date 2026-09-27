#include "peer_connection_observer.hpp"

#include "logger.hpp"
#include <QMutexLocker>
#include <QTransform>
#include <common_video/libyuv/include/webrtc_libyuv.h>

namespace core {
namespace rtc {

// PeerConnectionObserver::PeerConnectionObserver() = default;

PeerConnectionObserver::PeerConnectionObserver(QObject *parent)
    :QObject(parent)
{

}


PeerConnectionObserver::~PeerConnectionObserver()
{
    detachRemoteVideo();
}

void PeerConnectionObserver::detachRemoteVideo()
{
    if (m_remoteVideoTrack) {
        m_remoteVideoTrack->RemoveSink(this);
        m_remoteVideoTrack = nullptr;
    }
    QMutexLocker lock(&m_frameMutex);
    m_latestFrame = QImage();
    m_framePending = true;
}

void PeerConnectionObserver::OnTrack(
    webrtc::scoped_refptr<webrtc::RtpTransceiverInterface> transceiver)
{
    auto track = transceiver->receiver()->track();
    if (!track || track->kind() != webrtc::MediaStreamTrackInterface::kVideoKind) {
        return;
    }
    detachRemoteVideo();
    m_remoteVideoTrack = static_cast<webrtc::VideoTrackInterface *>(track.get());
    m_remoteVideoTrack->AddOrUpdateSink(this, webrtc::VideoSinkWants{});
}

void PeerConnectionObserver::OnRemoveTrack(
    webrtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver)
{
    if (receiver->track().get() == m_remoteVideoTrack.get()) {
        detachRemoteVideo();
    }
}

void PeerConnectionObserver::OnFrame(const webrtc::VideoFrame &frame)
{
    auto buffer = frame.video_frame_buffer()->ToI420();
    if (!buffer) {
        return;
    }
    QImage image(buffer->width(), buffer->height(), QImage::Format_RGBA8888);
    if (image.isNull()) {
        return;
    }
    const auto i420Frame = webrtc::VideoFrame::Builder()
                               .set_video_frame_buffer(buffer)
                               .build();
    if (webrtc::ConvertFromI420(i420Frame, webrtc::VideoType::kABGR,
                               image.bytesPerLine(), image.bits()) != 0) {
        return;
    }
    if (frame.rotation() != webrtc::kVideoRotation_0) {
        image = image.transformed(QTransform().rotate(static_cast<int>(frame.rotation())));
    }
    // Keep only the newest decoded frame until Qt is ready to present it.
    QMutexLocker lock(&m_frameMutex);
    m_latestFrame = std::move(image);
    m_framePending = true;
}

bool PeerConnectionObserver::takeRemoteVideoFrame(QImage &image)
{
    QMutexLocker lock(&m_frameMutex);
    if (!m_framePending) {
        return false;
    }
    image = std::move(m_latestFrame);
    m_framePending = false;
    return true;
}

void PeerConnectionObserver::OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state)
{
    switch (new_state) {
    case webrtc::PeerConnectionInterface::kStable:
        LOG_INFO (QStringLiteral ("OnConnectionChange: Stable"));
        break;
    case webrtc::PeerConnectionInterface::kHaveLocalOffer:
        LOG_INFO (QStringLiteral ("OnConnectionChange:Local Offer"));
        break;
    case webrtc::PeerConnectionInterface::kHaveLocalPrAnswer:
        LOG_INFO (QStringLiteral ("OnConnectionChange: Local Answer"));
        break;
    case webrtc::PeerConnectionInterface::kHaveRemoteOffer:
        LOG_INFO (QStringLiteral ("OnConnectionChange:Remote Offer"));
        break;
    case webrtc::PeerConnectionInterface::kHaveRemotePrAnswer:
        LOG_INFO (QStringLiteral ("OnConnectionChange:Remote Answer"));
        break;
    case webrtc::PeerConnectionInterface::kClosed:
        LOG_INFO (QStringLiteral ("OnConnectionChange: closed"));
        break;
    }
}

void PeerConnectionObserver::OnDataChannel(webrtc::scoped_refptr<webrtc::DataChannelInterface> data_channel)
{

}

void PeerConnectionObserver::OnRenegotiationNeeded()
{
    LOG_INFO (QStringLiteral ("OnRenegotiationNeeded"));
}

void PeerConnectionObserver::OnConnectionChange(webrtc::PeerConnectionInterface::PeerConnectionState state)
{
    emit connectionStateChanged(state);
    switch (state) {
    case webrtc::PeerConnectionInterface::PeerConnectionState::kNew:
        LOG_INFO (QStringLiteral ("OnConnectionChange: New"));
        break;
    case webrtc::PeerConnectionInterface::PeerConnectionState::kConnecting:
        LOG_INFO (QStringLiteral ("OnConnectionChange: Connecting"));
        break;
    case webrtc::PeerConnectionInterface::PeerConnectionState::kConnected:
        LOG_INFO (QStringLiteral ("OnConnectionChange: Connected"));
        break;
    case webrtc::PeerConnectionInterface::PeerConnectionState::kDisconnected:
        LOG_INFO (QStringLiteral ("OnConnectionChange: Disconnected"));
        break;
    case webrtc::PeerConnectionInterface::PeerConnectionState::kFailed:
        LOG_INFO (QStringLiteral ("OnConnectionChange: Failed"));
        emit errorOccurred(QStringLiteral("WebRTC peer connection failed"));
        break;
    case webrtc::PeerConnectionInterface::PeerConnectionState::kClosed:
        LOG_INFO (QStringLiteral ("OnConnectionChange: Closed"));
        break;
    }
}

void PeerConnectionObserver::OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state)
{
    switch(new_state) {
    case webrtc::PeerConnectionInterface::kIceGatheringNew:
        LOG_INFO (QStringLiteral ("OnIceGatheringChange: New"));
        break;
    case webrtc::PeerConnectionInterface::kIceGatheringGathering:
        LOG_INFO (QStringLiteral ("OnIceGatheringChange: Gathering"));
        break;
    case webrtc::PeerConnectionInterface::kIceGatheringComplete:
        LOG_INFO (QStringLiteral ("OnIceGatheringChange: Complete"));
        break;
    }
}

void PeerConnectionObserver::OnIceCandidate(const webrtc::IceCandidate *candidate)
{
    LOG_INFO (QStringLiteral ("OnIceCandidate: %1").arg (candidate->sdp_mid ()));
    LOG_INFO (QStringLiteral ("OnIceCandidate: %1").arg (candidate->sdp_mline_index ()));
    LOG_INFO (QStringLiteral ("OnIceCandidate: %1").arg (candidate->server_url ()));
    LOG_INFO (QStringLiteral ("OnIceCandidate: %1").arg (candidate->candidate ().ToString ()));

    const QString candidate_str = QString::fromStdString (candidate->ToString ());
    emit iceCandidateChanged(candidate_str, QString::fromStdString(candidate->sdp_mid()),
                             candidate->sdp_mline_index());
}

void PeerConnectionObserver::OnIceCandidateError(const std::string &address, int port,
                                                const std::string &url, int errorCode,
                                                const std::string &errorText)
{
    const QString message = QStringLiteral("ICE candidate gathering failed (%1): %2; server=%3; local=%4:%5")
        .arg(errorCode)
        .arg(QString::fromStdString(errorText), QString::fromStdString(url),
             QString::fromStdString(address))
        .arg(port);
    LOG_WARNING(message);
    emit errorOccurred(message);
}

void PeerConnectionObserver::OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState state)
{

    switch (state) {
    case webrtc::PeerConnectionInterface::kIceConnectionNew:
         LOG_INFO (QStringLiteral ("OnIceConnectionChange: New"));
        break;
    case webrtc::PeerConnectionInterface::kIceConnectionChecking:
         LOG_INFO (QStringLiteral ("OnIceConnectionChange: Checking"));
        break;
    case webrtc::PeerConnectionInterface::kIceConnectionConnected:
         LOG_INFO (QStringLiteral ("OnIceConnectionChange: Connected"));
        break;
    case webrtc::PeerConnectionInterface::kIceConnectionCompleted:
         LOG_INFO (QStringLiteral ("OnIceConnectionChange: Completed"));
        break;
    case webrtc::PeerConnectionInterface::kIceConnectionFailed:
         LOG_INFO (QStringLiteral ("OnIceConnectionChange: Failed"));
        emit errorOccurred(QStringLiteral("WebRTC ICE connection failed"));
        break;
    case webrtc::PeerConnectionInterface::kIceConnectionDisconnected:
         LOG_INFO (QStringLiteral ("OnIceConnectionChange: Disconnected"));
        break;
    case webrtc::PeerConnectionInterface::kIceConnectionClosed:
         LOG_INFO (QStringLiteral ("OnIceConnectionChange: Closed"));
        break;
    case webrtc::PeerConnectionInterface::kIceConnectionMax:
         LOG_INFO (QStringLiteral ("OnIceConnectionChange: Max"));
        break;
    }
}


} // namespace rtc
} // namespace core
