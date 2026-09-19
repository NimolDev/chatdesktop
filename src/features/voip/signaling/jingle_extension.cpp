#include "jingle_extension.hpp"
#include "logging/logger.hpp"

#include <QDomElement>
#include <QXmppClient.h>
#include <QXmppElement.h>
#include <QXmppMessage.h>
#include <QXmppTask.h>
#include <QUuid>


#define kNAME_SPACE  "urn:xmpp:jingle-message:0"
#define kJINGLE_NAME "urn:xmpp:jingle:1"
#define kJINGLE_APP "urn:xmpp:jingle:apps:rtp:1"
namespace voip {
namespace signaling {
namespace {

constexpr auto JingleMessageNamespace = "urn:xmpp:jingle-message:0";


std::optional<QDomElement> findJingleElement(const QDomElement &message)
{
    for (auto child = message.firstChildElement(); !child.isNull();
        child = child.nextSiblingElement()) {
        // qDebug() << "Child:" << child.tagName () << "xmlns:"<< child.namespaceURI ();
        QString namespaceURI = child.namespaceURI ();
        if (namespaceURI != kNAME_SPACE && namespaceURI != kJINGLE_NAME) {
            continue;
        }
        return child;
    }

    return std::nullopt;
}

} // namespace




JingleExtension::JingleExtension(QXmppClient *client, QObject *parent)
    : QXmppClientExtension()
    , m_message(std::make_unique<jingle::JingleMessage>())
    , m_jingleAction(std::make_unique<jingle::JingleAction> (client))

{
    setParent(parent);
    initializeJingleMessage ();
    initializeJingleAction ();
}



QStringList JingleExtension::discoveryFeatures() const
{
    return { QLatin1String(JingleMessageNamespace) };
}

bool JingleExtension::handleStanza(
    const QDomElement &stanza,
    const std::optional<QXmppE2eeMetadata> &)
{
    if (stanza.tagName() == QLatin1StringView("message")) {
        const auto jingle = findJingleElement(stanza);
        if (!jingle) {
            return false;
        }

        m_message->parse(stanza);
    } else if (stanza.tagName () == QLatin1StringView("iq")) {

        const auto jingle = findJingleElement(stanza);
        if (!jingle) {
            return false;
        }
        if (jingle->attribute(QStringLiteral("action")) == QLatin1StringView("session-initiate")
            && stanza.attribute(QStringLiteral("type")) == QLatin1StringView("set")) {
            m_sessionSender = stanza.attribute(QStringLiteral("from"));
        }
        if (m_jingleAction->parse(stanza)) {
            QXmppIq reply(QXmppIq::Result);
            reply.setId(stanza.attribute(QStringLiteral("id")));
            reply.setTo(stanza.attribute(QStringLiteral("from")));
            client()->send(std::move(reply));
            return true;
        }
    }

    return false;
}

// --- Jingle Message ---
void JingleExtension::initializeJingleMessage()
{
    connect(m_message.get (),
            &jingle::JingleMessage::proposeReceived,
            this,
            [this](const QList<jingle::StreamType> streams,
                   const QString &from,
                   const QString &mid) {
                this->proposeFrom = from;
                this->mid = mid;
                this->ringing ();
                emit proposeReceived (from, streams);
            }
            );
    connect(m_message.get (),
            &jingle::JingleMessage::retractReceived,
            this,
            &JingleExtension::retractReceived);
    connect (m_message.get (),
            &jingle::JingleMessage::ringingReceived,
            this,
            &JingleExtension::ringingReceived);
    connect (m_message.get (),
            &jingle::JingleMessage::acceptReceived,
            this,
            &JingleExtension::acceptReceived);
    connect(m_message.get (),
            &jingle::JingleMessage::proceedReceived,
            this,
            &JingleExtension::proceedReceived);
    connect (m_message.get (),
            &jingle::JingleMessage::rejectReceived,
            this,
            &JingleExtension::rejectReceived);
    connect (m_message.get (),
            &jingle::JingleMessage::finishReceived,
            this,
            &JingleExtension::finishReceived);

}
void JingleExtension::sendAccept()
{
    if (!client() || proposeFrom.isEmpty() || mid.isEmpty()) {
        qWarning() << "Cannot accept Jingle proposal: sender or proposal ID is missing";
        return;
    }

    client() ->send (m_message->accept (proposeFrom, mid));
}

void JingleExtension::ringing()
{
    client()->send (m_message->ringing (proposeFrom, mid));
}

void JingleExtension::proceed(const QString &recipient, bool video)
{

}

void JingleExtension::propose(const QString &recipient, bool video)
{
    QList<jingle::StreamType> streams = {jingle::StreamType::Audio};
    if (video) {
        streams.append (jingle::StreamType::Video);
    }
    mid = QUuid::createUuid ().toString (QUuid::WithoutBraces);
    m_sessionSender = recipient;
    client()->send (m_message->propose (recipient, streams, mid));
}

void JingleExtension::retract()
{
    client()->send(m_message->retract (proposeFrom, mid));
}

void JingleExtension::reject()
{
    client()->send ( m_message->reject (proposeFrom, mid));
}

// ---- Jingle Action ----
void JingleExtension::initializeJingleAction()
{
    connect(m_jingleAction.get (),
            &jingle::JingleAction::remoteSdpOfferReceived,
            this,
            [this](const QString &offer,
                   const QList<voip::signaling::IceServer> &ice_servers,
                   const QString &sid) {
                qDebug() << "offfer" << offer;
                qDebug() << "ICE server count" << ice_servers.size();
                qDebug() << "sid" << sid;
                m_currentSid = sid;
                emit remoteSdpOfferReceived (offer, ice_servers, sid);
            }
            );
    connect(m_jingleAction.get (),
            &jingle::JingleAction::exchangeIceCandidateReceived,
            this,
            &JingleExtension::exchangeIceCandidateReceived);
    connect(m_jingleAction.get (),
            &jingle::JingleAction::remoteSdpAnswerReceived,
            this,
            &JingleExtension::remoteSdpAnswerReceived);
    connect(m_jingleAction.get (),
            &jingle::JingleAction::sessionMuteReceived,
            this,
            &JingleExtension::sessionMuteReceived);
    connect(m_jingleAction.get (),
            &jingle::JingleAction::sessionActiveReceived,
            this,
            &JingleExtension::sessionActiveReceived);
    connect(m_jingleAction.get (),
            &jingle::JingleAction::sessionRingingReceived,
            this,
            &JingleExtension::sessionRingingReceived);
    connect(m_jingleAction.get (),
            &jingle::JingleAction::sessionTerminateReceived,
            this,
            &JingleExtension::sessionTerminateReceived);
}

void JingleExtension::sessionInitaite(
    const QString &recipient,
    const QString &sdp_offer,
    const QList<IceServer> &ice_servers)
{
    qDebug() << "SessionInitaite";
    if (!client() || !client()->isConnected () || sdp_offer.isEmpty ()) {
        LOG_WARNING (QStringLiteral ("Cannot send session initiate"));
        return;
    }
    if (recipient.isEmpty ()) {
         LOG_WARNING (QStringLiteral ("Receipient cannot be empty"));
        return;
    }
    m_sessionSender = recipient;
    const auto media = sdp_offer.contains(QStringLiteral("\nm=video "))
                           ? jingle::StreamType::Video : jingle::StreamType::Audio;
    QXmppIq iq = m_jingleAction->sessionInitaite (recipient, sdp_offer, ice_servers, media);
    client()->sendIq (std::move (iq)).then (this, [](QXmppClient::IqResult result) {
        if (const auto *error = std::get_if<QXmppError> (&result)) {
            LOG_WARNING (QStringLiteral ("Session initaite sending failed: %1").arg (error->description));
        }
    });
}

void JingleExtension::sessionAccept(const QString &sdp_answer)
{
    if (!client() || !client()->isConnected() || m_sessionSender.isEmpty()
        || m_currentSid.isEmpty() || sdp_answer.trimmed().isEmpty()) {
        qWarning() << "Cannot send session-accept: connection, session, or SDP is missing";
        return;
    }

    const auto media = sdp_answer.contains(QStringLiteral("\nm=video "))
        ? jingle::StreamType::Video : jingle::StreamType::Audio;
    auto iq = m_jingleAction->sessionAccept(m_sessionSender, sdp_answer, media);
    qInfo() << "Sending session-accept for session" << m_currentSid;
    client()->sendIq(std::move(iq)).then(this, [](QXmppClient::IqResult result) {
        if (const auto *error = std::get_if<QXmppError>(&result)) {
            qWarning() << "session-accept failed:" << error->description;
        } else {
            qInfo() << "session-accept acknowledged";
        }
    });

}

void JingleExtension::iceCandidate(const QString &candidate, const QString &sdpMid, int sdpMLineIndex)
{
    if (!client() || !client()->isConnected() || m_sessionSender.isEmpty()) {
        qWarning() << "Cannot send ICE candidate: connection or session is missing";
        return;
    }
    auto iq = m_jingleAction->exchangeIceCandidate(m_sessionSender, candidate, sdpMid, sdpMLineIndex);
    if (iq.extensions().isEmpty()) {
        return;
    }
    client()->sendIq(std::move(iq)).then(this, [](QXmppClient::IqResult result) {
        if (const auto *error = std::get_if<QXmppError>(&result)) {
            qWarning() << "ICE candidate IQ failed:" << error->description;
        }
    });
}



} // namespace signaling
} // namespace voip
