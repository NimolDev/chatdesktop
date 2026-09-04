#include "peer_connection_observer.hpp"

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

void PeerConnectionObserver::OnConnectionChange(webrtc::PeerConnectionInterface::PeerConnectionState)
{

}

void PeerConnectionObserver::OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state)
{

}

void PeerConnectionObserver::OnIceCandidate(const webrtc::IceCandidate *candidate)
{

}


} // namespace rtc
} // namespace core
