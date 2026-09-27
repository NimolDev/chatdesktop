#ifndef CORE_WEBRTC_PEER_CONNECTION_OBSERVER_HPP
#define CORE_WEBRTC_PEER_CONNECTION_OBSERVER_HPP

#include "api/peer_connection_interface.h"

#include <QObject>
#include <QImage>
#include <QMutex>
#include <api/video/video_frame.h>
namespace core {
namespace rtc {

class PeerConnectionObserver final : public QObject, public webrtc::PeerConnectionObserver,
                                     public webrtc::VideoSinkInterface<webrtc::VideoFrame>
{
    Q_OBJECT
public:
    PeerConnectionObserver(QObject *parent = nullptr);

    ~PeerConnectionObserver() override;
    void OnTrack(webrtc::scoped_refptr<webrtc::RtpTransceiverInterface> transceiver) override;
    void OnRemoveTrack(webrtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver) override;
    void OnFrame(const webrtc::VideoFrame &frame) override;
    bool takeRemoteVideoFrame(QImage &image);
    void detachRemoteVideo();

    // PeerConnectionObserver interface
public:
    void OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state) override;
    void OnDataChannel(webrtc::scoped_refptr<webrtc::DataChannelInterface> data_channel) override;
    void OnRenegotiationNeeded() override;
    void OnConnectionChange(webrtc::PeerConnectionInterface::PeerConnectionState) override;
    void OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state) override;
    void OnIceCandidate(const webrtc::IceCandidate *candidate) override;
    void OnIceCandidateError(const std::string &address, int port,
                             const std::string &url, int errorCode,
                             const std::string &errorText) override;
    void OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState) override;

private:
    webrtc::scoped_refptr<webrtc::VideoTrackInterface> m_remoteVideoTrack;
    QMutex m_frameMutex;
    QImage m_latestFrame;
    bool m_framePending = false;

signals:
    void errorOccurred(const QString &message);
    void iceCandidateChanged(const QString &candidate, const QString &sdpMid, int sdpMLineIndex);
    void connectionStateChanged(webrtc::PeerConnectionInterface::PeerConnectionState state);
};

} // namespace rtc
} // namespace core


#endif // CORE_WEBRTC_PEER_CONNECTION_OBSERVER_HPP

