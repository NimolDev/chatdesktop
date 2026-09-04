#include "peer_connection_observer.hpp"

#include "logger.hpp"

namespace core {
namespace rtc {

PeerConnectionObserver::PeerConnectionObserver() = default;

void PeerConnectionObserver::OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state)
{

}

void PeerConnectionObserver::OnDataChannel(webrtc::scoped_refptr<webrtc::DataChannelInterface> data_channel)
{

}

void PeerConnectionObserver::OnRenegotiationNeeded()
{

}

void PeerConnectionObserver::OnConnectionChange(webrtc::PeerConnectionInterface::PeerConnectionState state)
{
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
        break;
    case webrtc::PeerConnectionInterface::PeerConnectionState::kClosed:
        LOG_INFO (QStringLiteral ("OnConnectionChange: Closed"));
        break;
    }


}

void PeerConnectionObserver::OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state)
{

}

void PeerConnectionObserver::OnIceCandidate(const webrtc::IceCandidate *candidate)
{

}

void PeerConnectionObserver::OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState state)
{
    switch (state) {
    case webrtc::PeerConnectionInterface::kIceConnectionNew:
         LOG_INFO (QStringLiteral ("OnConnectionChange: New"));
        break;
    case webrtc::PeerConnectionInterface::kIceConnectionChecking:
         LOG_INFO (QStringLiteral ("OnConnectionChange: Checking"));
        break;
    case webrtc::PeerConnectionInterface::kIceConnectionConnected:
         LOG_INFO (QStringLiteral ("OnConnectionChange: Connected"));
        break;
    case webrtc::PeerConnectionInterface::kIceConnectionCompleted:
         LOG_INFO (QStringLiteral ("OnConnectionChange: Completed"));
        break;
    case webrtc::PeerConnectionInterface::kIceConnectionFailed:
         LOG_INFO (QStringLiteral ("OnConnectionChange: Failed"));
        break;
    case webrtc::PeerConnectionInterface::kIceConnectionDisconnected:
         LOG_INFO (QStringLiteral ("OnConnectionChange: Disconnected"));
        break;
    case webrtc::PeerConnectionInterface::kIceConnectionClosed:
         LOG_INFO (QStringLiteral ("OnConnectionChange: Closed"));
        break;
    case webrtc::PeerConnectionInterface::kIceConnectionMax:
         LOG_INFO (QStringLiteral ("OnConnectionChange: Max"));
        break;
    }
}


} // namespace rtc
} // namespace core
