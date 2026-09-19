#pragma once

#include <QObject>
#include <QImage>
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
    void initialize(QXmppClient *client);
    void startCall(const QString &receiverId, bool video = false);
    void acceptCall();
    void endCall();
    void rejectCall();
    void ringing();


public slots:
    void onLocalIceCandidateReceived(const QString &candidate, const QString &sdpMid, int sdpMLineIndex);
    void onProposeReceived(const QString &sender,
                           const QList<voip::signaling::jingle::StreamType> &streams);
    void onExchangeIceCandidateReceived(const std::string &sdp, const std::string &sdpMid, const int sdpMLineIndex);

signals:
    void localVideoFrameReady(const QImage &image);
    void remoteVideoFrameReady(const QImage &image);
    void messageReceived(const QString &action, const QString &sender,
                         const QString &sessionId,
                         const QList<voip::signaling::jingle::StreamType> &streams,
                         const QString &reason);
    void proposalSent(const QString &recipient, const QString &proposalId);
    void proposalReceived(const QString &sender,
                          const QList<voip::signaling::jingle::StreamType> &streams);
    void signalingFailed(const QString &reason);
    void localIceCandidateReceived(const QString &candidate, const QString &sdpMid, int sdpMLineIndex);

    void connectionStateChange(const core::rtc::ConnectionState &state);

private:
    QPointer<voip::signaling::JingleExtension> m_jingle;
    QPointer<QXmppClient> m_client;

    std::unique_ptr<core::rtc::WebrtcClient> m_webrtc;
     std::unique_ptr<core::xmpp::XmppServiceDiscovery> m_discovery;

    void createWebrtcClient();
    bool isConnected();
    bool m_videoCall = false;

    QString m_recipientId;
};

} // namespace signaling
} // namespace voip
