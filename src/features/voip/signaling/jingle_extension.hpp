#ifndef VOIP_SIGNALING_JINGLE_EXTENSION_HPP
#define VOIP_SIGNALING_JINGLE_EXTENSION_HPP

#include <QXmppClientExtension.h>
#include <QXmppJingleData.h>
#include "jingle_message.hpp"
#include "jingle_action.hpp"
#include "signaling/call_state.hpp"



#include <memory>
#include <optional>

namespace voip {
namespace signaling {

// using voip::signaling::jingle::SessionInfo;
// using voip::signaling::jingle::Type;

class JingleExtension final : public QXmppClientExtension
{
    Q_OBJECT

public:
    explicit JingleExtension(QXmppClient *client, QObject *parent = nullptr);

    QStringList discoveryFeatures() const override;
    bool handleStanza(
        const QDomElement &stanza,
        const std::optional<QXmppE2eeMetadata> &e2eeMetadata) override;

    void sendAccept();
    void ringing();
    void proceed(const QString &recipient, bool video);
    void propose(const QString &recipient, bool video);
    void retract();
    void reject();

    void sessionInitaite(
        const QString &recipient,
        const QString &sdp_offer,
        const QList<IceServer> &ice_servers
        );
    void sessionAccept(const QString &sdp_answer);
    void iceCandidate(const QString &candidate, const QString &sdpMid, int sdpMLineIndex);
    void sessioinInfo();
    void sessionTerminate();


signals:

    // Jingle Message
    void proposeReceived(const QString from,
                         QList<voip::signaling::jingle::StreamType> types);
    void retractReceived(const QString &retract, const QString &mid);
    void ringingReceived(const QString &ringing, const QString &mid);
    void proceedReceived(const QString &proceed, const QString &mid);
    void rejectReceived(const QString &reject, const QString &mid);
    void acceptReceived(const QString &accept, const QString &mid);
    void finishReceived(const QString &finish, const QString &mid);

    // Jinge Action
    void remoteSdpOfferReceived(
        const QString &offer,
        const QList<voip::signaling::IceServer> &ice_server,
        const QString &sid
        );
    void exchangeIceCandidateReceived(
        const std::string &sdp,
        const std::string &sdpMid,
        const int sdpMLineIndex);
    void remoteSdpAnswerReceived(const QString &answer, const QString &sid);
    void sessionMuteReceived(const voip::signaling::jingle::MediaType mute);
    void sessionActiveReceived(bool active);
    void sessionRingingReceived(bool ringing);
    void sessionTerminateReceived(bool terminate);

    void transportReplaceReveived(const QString &sdp, const QString &sid);
    void transportAcceptReceived(const QString &sdp, const QString &sid);

    void callStateChange(const voip::signaling::CallState &state);

private:
    void initialize();
    void initializeJingleMessage();
    void initializeJingleAction();

private:
    std::unique_ptr<voip::signaling::jingle::JingleMessage> m_message;
    std::unique_ptr<voip::signaling::jingle::JingleAction> m_jingleAction;
    QString m_pendingSender;
    QString m_pendingProposalId;
    QString m_currentSid;
    QString m_sessionSender;

    QString proposeFrom;
    QString mid;

};

} // namespace signaling
} // namespace voip

#endif // VOIP_SIGNALING_JINGLE_EXTENSION_HPP
