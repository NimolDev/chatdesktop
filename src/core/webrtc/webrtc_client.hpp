#ifndef CORE_WEBRTC_WEBRTC_CLIENT_HPP
#define CORE_WEBRTC_WEBRTC_CLIENT_HPP

#include <QObject>


#include "peer_connection_observer.hpp"
#include "sdp_observer.hpp"

#include "api/peer_connection_interface.h"
#include "api/video/video_frame.h"




namespace core {
namespace rtc {

class WebRtcClientPrivate;

class WebrtcClient : public QObject
{
    Q_OBJECT
public:
    explicit WebrtcClient(QObject *parent = nullptr);
    ~WebrtcClient();

    bool initialize();

    // void startCall(const QString &user_id);
    // void handleOffer(const QString &sdp);

    // void addIceCandidate(
    //     const QString *mid,
    //     int mlineIndex,
    //     const QString &candidate);
    bool createPeerConnection();

    void createOffer();

    // Delivers a captured frame to the local WebRTC video track. This may be
    // called from a capture thread after createPeerConnection() succeeds.
    bool pushVideoFrame(const webrtc::VideoFrame &frame);

signals:
    void localSdpCreated(
        const QString &type,
        const QString &sdp);

    void iceCandidateCreated(
        const QString &mid,
        int mlineIndex,
        const QString &candidate);

    void remoteVideoFrameReady();
    void connected();
    void disconnected();

private:
    std::unique_ptr<WebRtcClientPrivate> d;
    webrtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> m_factory;
    webrtc::scoped_refptr<webrtc::PeerConnectionInterface> m_peerConnection;

    std::unique_ptr<core::rtc::PeerConnectionObserver> m_peerObserver;
    webrtc::scoped_refptr<core::rtc::SdpObserver> m_sdpObserver;

};

} // namespace rtc
} // namespace core


#endif // CORE_WEBRTC_WEBRTC_CLIENT_HPP
