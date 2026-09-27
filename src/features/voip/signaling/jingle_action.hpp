#ifndef VOIP_SIGNALING_JINGLE_ACTION_HPP
#define VOIP_SIGNALING_JINGLE_ACTION_HPP

#include "QXmppClient.h"
#include "jingle_message.hpp"
#include "ice_server.hpp"


#include <QObject>


namespace voip {
namespace signaling {
namespace jingle {


enum class ActionType
{
    SessionInitiate,
    SessionTerminate,
    SessionAccept,
    SessionInfo,
    SessionTransport,
    TransportReplace,
    TransportAccept,

};

inline QString fromType(ActionType type)
{
    switch (type) {
    case ActionType::SessionInitiate: return QStringLiteral("session-initiate");
    case ActionType::SessionTerminate:  return QStringLiteral("session-terminate");
    case ActionType::SessionAccept: return QStringLiteral("session-accept");
    case ActionType::SessionInfo: return QStringLiteral("session-info");
    case ActionType::SessionTransport: return QStringLiteral ("session-transport");
    case ActionType::TransportReplace: return QStringLiteral ("transport-replace");
    case ActionType::TransportAccept: return QStringLiteral ("transport-accept");
    }
    return {};

}

inline std::optional<ActionType> fromString(const QString &type)
{
    if (type == QStringLiteral("session-initiate")) { return ActionType::SessionInitiate;}
    if (type == QStringLiteral("session-terminate")) { return ActionType::SessionTerminate; }
    if (type == QStringLiteral("session-accept")) { return ActionType::SessionAccept; }
    if (type == QStringLiteral("session-info")) { return ActionType::SessionInfo; }
    if (type == QStringLiteral ("session-transport")) { return ActionType::SessionTransport; }
    if (type == QStringLiteral ("transport-replace")) { return ActionType::TransportReplace; }
    if (type == QStringLiteral ("transport-accept")) { return ActionType::TransportAccept; }
    return std::nullopt;
}

enum class SessionInfo
{
    Mute,
    Ringing,
    Unmute,
    Active
};

inline QString fromSessionInfo(SessionInfo info) {
    switch(info) {
    case SessionInfo::Mute: return QStringLiteral ("mute");
    case SessionInfo::Ringing: return QStringLiteral ("ringing");
    case SessionInfo::Unmute: return QStringLiteral ("unmute");
    case SessionInfo::Active: return QStringLiteral ("active");
    }
    return {};
}

inline std::optional<SessionInfo> fromSessionInfoString(const QString &str) {
    if (str == QStringLiteral ("mute")) { return SessionInfo::Mute; }
    if (str == QStringLiteral ("ringing")) { return SessionInfo::Ringing; }
    if (str == QStringLiteral ("unmute")) { return SessionInfo::Unmute; }
    if (str == QStringLiteral ("active")) { return SessionInfo::Active; }
    return std::nullopt;
}

enum class MediaType
{
    Microphone,
    Camera
};
inline QString fromMediaType(MediaType type) {
    switch(type) {
    case MediaType::Microphone: return QStringLiteral ("microphone");
    case MediaType::Camera: return QStringLiteral ("camera");
    }
}
inline std::optional<MediaType> fromMediaString(const QString &str) {
    if (str == QStringLiteral ("microphone")) { return MediaType::Microphone; }
    if (str == QStringLiteral ("camera")) { return MediaType::Camera; }
    return std::nullopt;
}


class JingleAction : public QObject
{
    Q_OBJECT
public:
    explicit JingleAction(QXmppClient *client, QObject *parent = nullptr);

    bool parse(const QDomElement &element);

    QXmppIq ackKnowledge(const QString &recipient, const QString &sid);
    QXmppIq sessionInitaite(
        const QString recipient,
        const QString sdp_offer,
        const QList<IceServer> ice_servers,
        const StreamType media);

    QXmppIq sessionAccept(
        const QString recipient,
        const QString sdp_answer,
        const StreamType media);

    QXmppIq sessionInfo(
        const QString recipient,
        const SessionInfo info,
        const MediaType media);

    QXmppIq sessionTerminate(
        const QString recipient);

    QXmppIq exchangeIceCandidate(
        const QString recipient,
        const QString candidate,
        const QString sdpMid,
        int sdpMLineIndex);

    QXmppIq transportReplace(
        const QString ,
        const QString sdp
        );

    QXmppIq transportAccept(
        const QString recipient,
        const QString sdp
        );


signals:
    void remoteSdpOfferReceived(
        const QString &offer,
        const QList<voip::signaling::IceServer> &iceServers,
        const QString &sid);

    void exchangeIceCandidateReceived(const std::string &sdp,
                                      const std::string &sdpMid,
                                      const int sdpMLineIndex);
    void remoteSdpAnswerReceived(const QString &answer, const QString &sid);
    void sessionMuteReceived(const voip::signaling::jingle::MediaType mute);
    void sessionActiveReceived(bool active);
    void sessionRingingReceived(bool ringing);
    void sessionTerminateReceived(bool terminate);

    void transportReplaceReveived(const QString &sdp, const QString &sid);
    void transportAcceptReceived(const QString &sdp, const QString &sid);

private:
    QString m_currentSid;
    QString m_contentName;
    QString m_contentCreator;


    QXmppClient *m_client = nullptr;
};




} // namespace jingle
} // namespace signaling

} // namespace voip

#endif // VOIP_SIGNALING_JINGLE_ACTION_HPP
