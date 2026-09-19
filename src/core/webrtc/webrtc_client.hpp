#ifndef CORE_WEBRTC_WEBRTC_CLIENT_HPP
#define CORE_WEBRTC_WEBRTC_CLIENT_HPP


#include "ice_server.hpp"
#include "api/jsep.h"

#include <QObject>
#include <QImage>
#include <QList>
#include <functional>
#include <memory>


namespace webrtc { class VideoFrame; }

namespace core {
namespace rtc {

class WebRtcClientPrivate;

enum class ConnectionState
{
    Checking,
    Connecting,
    Connected,
    Disconnect

};

class WebrtcClient : public QObject
{
    Q_OBJECT
public:
    explicit WebrtcClient(QObject *parent = nullptr);
    ~WebrtcClient();

    bool initialize();

    bool createPeerConnection(const QList<core::rtc::IceServer> &iceServers);

    using SdpCompletionHandler = std::function<void(const QString &sdp)>;

    // Called on this object's Qt thread after the local description is set.
    // Failures are logged without invoking the callback. Destroying the client
    // cancels pending delivery. The localSdpCreated signal is also emitted.
    void createOffer(SdpCompletionHandler onSuccess = {});
    void createAnswer(SdpCompletionHandler onSuccess = {});
    // Returns whether setting the SDP was started; onSuccess runs on this
    // object's Qt thread only after WebRTC has applied the remote description.
    bool setRemoteSdp(const QString &type, const QString &sdp,
                      std::function<void()> onSuccess = {});

    bool setRemoteCandidate(const std::string &sdp,
                            const std::string &sdpMid,
                            const int sdpMLineIndex);

    // Delivers a captured frame to the local WebRTC video track. This may be
    // called from a capture thread after createPeerConnection() succeeds.
    bool pushVideoFrame(const webrtc::VideoFrame &frame);

    // Configures voice processing for subsequently created local audio tracks.
    // Called automatically by initialize(), on this object's thread.
    void configureAudioSession();

    void setEnableSpeaker(bool speaker);
    void setMuteMicrophone(bool mute);

    // Enable video before SDP negotiation; capture starts when connected.
    void createLocalCameraTrack();
    void setCameraEnabled(bool enable);

    // Signales for tracks.

// helper function
public:

    std::unique_ptr<webrtc::IceCandidateInterface> parseIceCandidate(
        const std::string &sdp,
        const std::string &sdpMid,
        int sdpMLineIndex);



signals:
    void cameraEnabledChanged(bool enable);
    void microphoneMuteChanged(bool enable);

signals:
    void errorOccurred(const QString &message);

    void localSdpCreated(
        const QString &type,
        const QString &sdp);

    void iceCandidateCreated(
        const QString &mid,
        int mlineIndex,
        const QString &candidate);

    void localVideoFrameReady(const QImage &image);
    void remoteVideoFrameReady(const QImage &image);
    void connected();
    void disconnected();
    void localIceCandidateGenerated(const QString &candidate, const QString &sdpMid, int sdpMLineIndex);
    void connectionStateChanged(core::rtc::ConnectionState state);

private:
    void createLocalAudioTrack();
    void updateCameraCapture();

private:
    std::unique_ptr<WebRtcClientPrivate> d;

};

} // namespace rtc
} // namespace core


#endif // CORE_WEBRTC_WEBRTC_CLIENT_HPP
