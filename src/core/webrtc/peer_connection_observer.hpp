#ifndef CORE_WEBRTC_PEER_CONNECTION_OBSERVER_HPP
#define CORE_WEBRTC_PEER_CONNECTION_OBSERVER_HPP

#include "api/peer_connection_interface.h"


namespace core {
namespace rtc {

class PeerConnectionObserver final : public webrtc::PeerConnectionObserver
{
public:
    PeerConnectionObserver();



    // PeerConnectionObserver interface
public:
    void OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state) override;
    void OnDataChannel(webrtc::scoped_refptr<webrtc::DataChannelInterface> data_channel) override;
    void OnRenegotiationNeeded() override;
    void OnConnectionChange(webrtc::PeerConnectionInterface::PeerConnectionState) override;
    void OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state) override;
    void OnIceCandidate(const webrtc::IceCandidate *candidate) override;


};

} // namespace rtc
} // namespace core


#endif // CORE_WEBRTC_PEER_CONNECTION_OBSERVER_HPP
