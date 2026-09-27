#pragma once

#include <QObject>
#include <QVideoFrame>
#include <QPointer>
#include "webrtc/webrtc_client.hpp"
#include "jingle_extension.hpp"
#include "xmpp_service_discovery.hpp"

namespace voip {
namespace signaling {

// Lives on the XMPP thread. The client owns the protocol extension.
class JingleService final : public QObject
{
    Q_OBJECT

public:
    explicit JingleService(QObject *parent = nullptr);

public slots:
    void setAudioInputDevice(const QAudioDevice &device);
    void setAudioOutputDevice(const QAudioDevice &device);
    void setCameraDevice(const QCameraDevice &device);
    void initialize(QXmppClient *client);
    void startCall(const QString &receiverId, bool video = false);
    void acceptCall();
    void endCall();
    void rejectCall();
    void ringing();

    void prepare();

public slots:
    void onLocalIceCandidateReceived(const QString &candidate, const QString &sdpMid, int sdpMLineIndex);
    void onProposeReceived(const QString &sender,
                           const QList<voip::signaling::jingle::StreamType> &streams);
    void onExchangeIceCandidateReceived(const std::string &sdp, const std::string &sdpMid, const int sdpMLineIndex);

signals:
    void localVideoFrameReady(const QVideoFrame &frame);
    void remoteVideoFrameReady(const QVideoFrame &frame);

    void proposalSent(const QString &recipient, const QString &proposalId);

    void signalingFailed(const QString &reason);
    void localIceCandidateReceived(const QString &candidate, const QString &sdpMid, int sdpMLineIndex);

    void sessionTerminate();

    void connectionStateChange(const core::rtc::ConnectionState &state);
    void callStateChange(const voip::signaling::CallState &state);


    // Jingle Message
    void proposeReceived(const QString from,
                         QList<voip::signaling::jingle::StreamType> types);
    void retractReceived(const QString &retract, const QString &mid);
    void ringingReceived(const QString &ringing, const QString &mid);
    void proceedReceived(const QString &proceed, const QString &mid);
    void rejectReceived(const QString &reject, const QString &mid);
    void acceptReceived(const QString &accept, const QString &mid);
    void finishReceived(const QString &finish, const QString &mid);


private:
    QAudioDevice m_audioInput;
    QAudioDevice m_audioOutput;
    QCameraDevice m_cameraDevice;
    QPointer<voip::signaling::JingleExtension> m_jingle;
    QPointer<QXmppClient> m_client;

    std::unique_ptr<core::rtc::WebrtcClient> m_webrtc;
     std::unique_ptr<core::xmpp::XmppServiceDiscovery> m_discovery;

    void createWebrtcClient();
    bool prepareMedia(bool video);
    bool m_mediaPrepared = false;
    bool m_callStarted = false;
    bool isConnected();
    bool m_videoCall = false;

    QString m_recipientId;
};

} // namespace signaling
} // namespace voip
