#include "jingle_action.hpp"
#include "jingle_message.hpp"
#include "logging/logger.hpp"

#include <QDomElement>
#include <QUuid>

#define kJINGLE_NAME             "urn:xmpp:jingle:1"
#define kJINGLE_APP              "urn:xmpp:jingle:apps:rtp:1"
#define kJINGLE_TRANSPORT        "urn:xmpp:jingle:apps:rtp:info:1"
#define kJINGLE_TRANSPORT_ICE    "urn:xmpp:jingle:transports:ice:0"

voip::signaling::jingle::JingleAction::JingleAction(QXmppClient *client, QObject *parent)
    : m_client(std::move (client)),
    QObject(parent)
{}

bool voip::signaling::jingle::JingleAction::parse(const QDomElement &element)
{
    if (element.isNull ()) {
        return false;
    }
    const auto matches = [](const QDomElement &node, QLatin1StringView name,
                            QLatin1StringView ns) {
        const auto localName = node.localName().isEmpty() ? node.tagName() : node.localName();
        const auto namespaceUri = node.namespaceURI().isEmpty()
            ? node.attribute(QStringLiteral("xmlns")) : node.namespaceURI();
        return localName == name && namespaceUri == ns;
    };

    QDomElement jingle = element;
    if (element.tagName() == QLatin1StringView("iq")
        || element.localName() == QLatin1StringView("iq")) {
        if (element.attribute(QStringLiteral("type")) != QLatin1StringView("set")) {
            return false;
        }
        for (jingle = element.firstChildElement(); !jingle.isNull();
             jingle = jingle.nextSiblingElement()) {
            if (matches(jingle, QLatin1StringView("jingle"), QLatin1StringView(kJINGLE_NAME))) {
                break;
            }
        }
    }
    if (!matches(jingle, QLatin1StringView("jingle"), QLatin1StringView(kJINGLE_NAME))) {
        return false;
    }

    const auto action = fromString(jingle.attribute(QStringLiteral("action")));
    const auto sid = jingle.attribute(QStringLiteral("sid"));
    if (!action || sid.isEmpty()) {
        return false;
    }

    switch (*action) {
    case ActionType::SessionInitiate: {
        for (auto content = jingle.firstChildElement(QStringLiteral("content")); !content.isNull();
             content = content.nextSiblingElement(QStringLiteral("content"))) {
            QString offer;
            QList<IceServer> iceServers;
            for (auto child = content.firstChildElement(); !child.isNull();
                 child = child.nextSiblingElement()) {
                if (matches(child, QLatin1StringView("description"), QLatin1StringView(kJINGLE_APP))) {
                    offer = child.firstChildElement(QStringLiteral("sdp")).text();
                } else if (matches(child, QLatin1StringView("transport"), QLatin1StringView(kJINGLE_TRANSPORT_ICE))) {
                    for (auto ice = child.firstChildElement(QStringLiteral("ice")); !ice.isNull();
                         ice = ice.nextSiblingElement(QStringLiteral("ice"))) {
                        IceServer server;
                        const auto type = ice.attribute(QStringLiteral("type"));
                        if (type == QLatin1StringView("stun")) {
                            server.type = IceServer::Stun;
                        } else if (type == QLatin1StringView("turn")) {
                            server.type = IceServer::Turn;
                        } else {
                            continue;
                        }
                        server.host = ice.attribute(QStringLiteral("host"));
                        server.port = ice.attribute(QStringLiteral("port"));
                        server.username = ice.attribute(QStringLiteral("username"));
                        server.credential = ice.attribute(QStringLiteral("credential"));
                        if (server.host.isEmpty()) {
                            continue;
                        }
                        iceServers.append(server);
                    }
                }
            }
            if (offer.trimmed().isEmpty()) {
                continue;
            }

            m_currentSid = sid;
            m_contentName = content.attribute(QStringLiteral("name"));
            m_contentCreator = content.attribute(QStringLiteral("creator"), QStringLiteral("initiator"));
            emit remoteSdpOfferReceived(offer, iceServers, sid);
            return true;
        }
        break;
    }
    case ActionType::SessionAccept: {
        if (sid != m_currentSid) {
            return false;
        }
        for (auto content = jingle.firstChildElement (QStringLiteral ("content"));
             !content.isNull ();
             content = content.nextSiblingElement ()) {
            auto description = content.firstChildElement (QStringLiteral ("description"));
            auto answer = description.firstChildElement (QStringLiteral ("sdp")).text ();

            if (answer.trimmed().isEmpty()) {
                continue;
            }
            emit remoteSdpAnswerReceived(answer, sid);
            return true;
        }
        break;
    }
    case ActionType::SessionTerminate: {
        // TODO: Parse session termination.
        LOG_INFO("Session terminate");
        emit sessionTerminateReceived (true);
    }
        break;

    case ActionType::SessionInfo: {
        // TODO: Parse session information.
        break;
    }
    case ActionType::SessionTransport: {
        if (sid != m_currentSid) {
            return false;
        }
        bool handled = false;
        for (auto transport = jingle.firstChildElement(); !transport.isNull();
             transport = transport.nextSiblingElement()) {
            if (!matches(transport, QLatin1StringView("transport"), QLatin1StringView(kJINGLE_TRANSPORT))) {
                continue;
            }
            for (auto candidate = transport.firstChildElement(); !candidate.isNull();
                 candidate = candidate.nextSiblingElement()) {
                // Without namespace processing, child elements inherit the
                // transport namespace without exposing it through QDom.
                const auto candidateName = candidate.localName().isEmpty()
                    ? candidate.tagName() : candidate.localName();
                if (candidateName != QLatin1StringView("candidate")) {
                    continue;
                }
                if ((!candidate.namespaceURI().isEmpty() || candidate.hasAttribute(QStringLiteral("xmlns")))
                    && !matches(candidate, QLatin1StringView("candidate"), QLatin1StringView(kJINGLE_TRANSPORT))) {
                    continue;
                }
                const auto foundation = candidate.attribute (QStringLiteral ("foundation"));
                const auto component = candidate.attribute (QStringLiteral ("component"));
                const auto sdp = candidate.attribute(QStringLiteral("sdp"));
                if (sdp.trimmed().isEmpty() || foundation.isEmpty () || component.isEmpty ()) {
                    continue;
                }
                bool validIndex = false;
                const int sdpMLineIndex = component.toInt(&validIndex);
                if (!validIndex || sdpMLineIndex < 0) {
                    continue;
                }
                const std::string sdpMid = foundation.toStdString ();
                emit exchangeIceCandidateReceived (sdp.toStdString (), sdpMid, sdpMLineIndex);
                handled = true;
            }
        }
        return handled;
    }
    case ActionType::TransportReplace:
        // TODO: Parse transport replacement.
        break;
    case ActionType::TransportAccept:
        // TODO: Parse transport acceptance.
        break;
    }
    return false;
}

QXmppIq voip::signaling::jingle::JingleAction::ackKnowledge(
    const QString &recipient,
    const QString &sid)
{
    QXmppIq iq;
    iq.setType (QXmppIq::Result);
    iq.setId(sid);
    iq.setTo (recipient);
    return iq;
}

QXmppIq voip::signaling::jingle::JingleAction::sessionInitaite(
    const QString recipient,
    const QString sdp_offer,
    const QList<IceServer> ice_servers,
    const StreamType media)
{

    m_currentSid = QUuid::createUuid ().toString (QUuid::WithoutBraces);
    QXmppElement jingle;
    jingle.setTagName (QStringLiteral ("jingle"));
    jingle.setAttribute (QStringLiteral ("xmlns"),kJINGLE_NAME);
    jingle.setAttribute (QStringLiteral ("action"), fromType (ActionType::SessionInitiate));
    jingle.setAttribute (QStringLiteral ("initiator"), m_client->configuration ().jidBare ());
    jingle.setAttribute (QStringLiteral ("sid"), m_currentSid);

    QXmppElement content;
    content.setTagName (QStringLiteral ("content"));
    content.setAttribute (QStringLiteral ("creatore"), QStringLiteral ("initiator"));
    content.setAttribute (QStringLiteral ("name"), QStringLiteral ("video"));

    QXmppElement description;
    description.setTagName (QStringLiteral ("description"));
    description.setAttribute (QStringLiteral ("xmlns"), kJINGLE_APP);
    description.setAttribute (QStringLiteral ("media"), fromStreamType (media));

    QXmppElement sdpElement;
    sdpElement.setTagName (QStringLiteral ("sdp"));
    sdpElement.setValue (sdp_offer);
    // sdpElement.setAttribute (QStringLiteral ("sdp"), sdp_offer);

    description.appendChild (sdpElement);
    content.appendChild (description);

    QXmppElement transport;
    transport.setTagName (QStringLiteral ("transport"));
    transport.setAttribute (QStringLiteral ("xmlns"),kJINGLE_TRANSPORT_ICE);

    for (auto const &ice : ice_servers) {
        QXmppElement ice_e;
        ice_e.setTagName (QStringLiteral ("ice"));
        ice_e.setAttribute (QStringLiteral ("type"), ice.toString (ice.type));
        ice_e.setAttribute (QStringLiteral ("port"), ice.port);
        ice_e.setAttribute (QStringLiteral ("username"), ice.username);
        ice_e.setAttribute (QStringLiteral ("credential"), ice.credential);
        transport.appendChild (ice_e);
    }

    content.appendChild (transport);
    jingle.appendChild (content);

    QXmppIq iq;
    iq.setType (QXmppIq::Set);
    iq.setTo (recipient);
    iq.setExtensions ({std::move (jingle)});
    return iq;
}

QXmppIq voip::signaling::jingle::JingleAction::sessionAccept(
    const QString recipient,
    const QString sdp,
    const StreamType media)
{
    QXmppElement jingle;
    jingle.setTagName (QStringLiteral ("jingle"));
    jingle.setAttribute (QStringLiteral ("xmlns"), kJINGLE_NAME);
    jingle.setAttribute (QStringLiteral ("action"), jingle::fromType (jingle::ActionType::SessionAccept));
    jingle.setAttribute (QStringLiteral ("responder"), m_client->configuration ().jid ());
    jingle.setAttribute (QStringLiteral ("sid"), m_currentSid);

    // Create Content element for media type (audio, video)
    QXmppElement content;
    content.setTagName (QStringLiteral ("content"));
    content.setAttribute (QStringLiteral ("creator"), m_contentCreator);
    content.setAttribute (QStringLiteral ("name"), m_contentName);

    QXmppElement description;
    description.setTagName (QStringLiteral("description"));
    description.setAttribute (QStringLiteral ("xmlns"), kJINGLE_APP);
    description.setAttribute (QStringLiteral ("media"), fromStreamType (media));

    QXmppElement sdpElement;
    sdpElement.setTagName(QStringLiteral("sdp"));
    sdpElement.setValue(sdp);

    description.appendChild (sdpElement);
    content.appendChild (description);
    jingle.appendChild (content);

    QXmppIq iq;
    iq.setType (QXmppIq::Set);
    iq.setTo (recipient);
    iq.setExtensions({jingle});
    return iq;
}

QXmppIq voip::signaling::jingle::JingleAction::sessionInfo(
    const QString recipient,
    const SessionInfo info,
    const MediaType media)
{
    QXmppElement jingle;
    jingle.setTagName ("jingle");
    jingle.setAttribute (QStringLiteral ("xmlns"), kJINGLE_NAME);
    jingle.setAttribute (QStringLiteral ("action"), fromType (ActionType::SessionInfo));
    jingle.setAttribute (QStringLiteral ("sid"), m_currentSid);

    switch(info) {
    case SessionInfo::Mute: {
        QString media_str = voip::signaling::jingle::fromMediaType (media);
        QXmppElement mute;
        mute.setTagName (fromSessionInfo (SessionInfo::Mute));
        mute.setAttribute (QStringLiteral ("xmlns"), kJINGLE_TRANSPORT);
        mute.setAttribute (QStringLiteral ("name"), media_str);
        jingle.appendChild (mute);
        break;
    }
    case SessionInfo::Unmute: {
        QString media_str = voip::signaling::jingle::fromMediaType (media);
        QXmppElement unmute;
        unmute.setTagName (fromSessionInfo (SessionInfo::Unmute));
        unmute.setAttribute (QStringLiteral ("xmlns"), kJINGLE_TRANSPORT);
        unmute.setAttribute (QStringLiteral ("name"), media_str);
        jingle.appendChild (unmute);
        break;
    }
    case SessionInfo::Ringing: {
        QXmppElement ringing;
        ringing.setTagName (fromSessionInfo (SessionInfo::Ringing));
        ringing.setAttribute (QStringLiteral ("xmlns"), kJINGLE_TRANSPORT);
        jingle.appendChild (ringing);
        break;
    }

    case SessionInfo::Active: {
        QXmppElement active;
        active.setTagName (fromSessionInfo (SessionInfo::Active));
        active.setAttribute (QStringLiteral ("xmlns"), kJINGLE_TRANSPORT);
        jingle.appendChild (active);
        break;
    }
    }

    QXmppIq iq;
    iq.setType (QXmppIq::Type::Set);
    iq.setTo (recipient);
    iq.setExtensions ({std::move (jingle)});
    return iq;
}

/*
 * <iq type="set" to="" id="" from="">
 * <jingle xmlns="urn:xmpp:jingle:1" action="session-terminate" sid="">
 *   <reason>
 *      <success/>
 *      <text>Stuck!</text>
 *   </reason>
 * </jingle>
 * </iq>
 */
QXmppIq voip::signaling::jingle::JingleAction::sessionTerminate(const QString recipient)
{
    QXmppElement jingle;
    jingle.setTagName (QStringLiteral ("jingle"));
    jingle.setAttribute (QStringLiteral ("xmlns"), kJINGLE_NAME);
    jingle.setAttribute (QStringLiteral ("sid"), m_currentSid);
    jingle.setAttribute (QStringLiteral ("action"), fromType (ActionType::SessionTerminate));

    QXmppElement reason;
    reason.setTagName (QStringLiteral ("reason"));
    QXmppElement success;
    success.setTagName (QStringLiteral ("success"));
    QXmppElement text;
    text.setTagName (QStringLiteral ("text"));
    text.setValue (QStringLiteral ("Stuck!"));

    reason.appendChild (success);
    reason.appendChild (text);
    jingle.appendChild (reason);

    QXmppIq iq;
    iq.setType (QXmppIq::Type::Set);
    iq.setTo (recipient);
    iq.setExtensions ({std::move (jingle)});
    return iq;
}

QXmppIq voip::signaling::jingle::JingleAction::exchangeIceCandidate(const QString recipient,
                                                                    const QString candidate,
                                                                    const QString sdpMid,
                                                                    int sdpMLineIndex)
{
    const auto parts = candidate.simplified().split(QLatin1Char(' '), Qt::SkipEmptyParts);
    if (recipient.isEmpty() || m_currentSid.isEmpty() || parts.size() < 8
        || parts[6] != QLatin1StringView("typ")) {
        qWarning() << "Cannot build ICE candidate IQ: missing session or invalid candidate SDP";
        return {};
    }

    QString foundation = parts[0];
    if (foundation.startsWith(QLatin1StringView("a="))) {
        foundation.remove(0, 2);
    }
    if (!foundation.startsWith(QLatin1StringView("candidate:"))) {
        qWarning() << "Cannot build ICE candidate IQ: missing candidate prefix";
        return {};
    }
    foundation.remove(0, 10);
    if (foundation.isEmpty()) {
        return {};
    }

    QXmppElement candidateElement;
    candidateElement.setTagName(QStringLiteral("candidate"));
    candidateElement.setAttribute(QStringLiteral("foundation"), sdpMid);
    candidateElement.setAttribute(QStringLiteral("component"), QString::number(sdpMLineIndex));
    candidateElement.setAttribute(QStringLiteral("protocol"), parts[2]);
    candidateElement.setAttribute(QStringLiteral("priority"), parts[3]);
    candidateElement.setAttribute(QStringLiteral("ip"), parts[4]);
    candidateElement.setAttribute(QStringLiteral("port"), parts[5]);
    candidateElement.setAttribute(QStringLiteral("type"), parts[7]);
    candidateElement.setAttribute(QStringLiteral("sdp"), candidate);

    QXmppElement transport;
    transport.setTagName(QStringLiteral("transport"));
    transport.setAttribute(QStringLiteral("xmlns"), kJINGLE_TRANSPORT);
    transport.appendChild(candidateElement);

    QXmppElement jingle;
    jingle.setTagName(QStringLiteral("jingle"));
    jingle.setAttribute(QStringLiteral("xmlns"), kJINGLE_NAME);
    jingle.setAttribute(QStringLiteral("action"), fromType(ActionType::SessionTransport));
    jingle.setAttribute(QStringLiteral("sid"), m_currentSid);
    jingle.appendChild(transport);

    QXmppIq iq(QXmppIq::Set);
    iq.setTo(recipient);
    iq.setExtensions({jingle});
    return iq;
}

QXmppIq voip::signaling::jingle::JingleAction::transportReplace(const QString recipient, const QString sdp)
{
    QXmppElement jingle;
    jingle.setTagName (QStringLiteral ("jingle"));
    jingle.setAttribute (QStringLiteral ("xmlns"), kJINGLE_NAME);
    jingle.setAttribute (QStringLiteral ("action"), fromType (ActionType::TransportReplace));
    jingle.setAttribute (QStringLiteral ("initiator"), m_client->configuration ().jidBare ());
    jingle.setAttribute (QStringLiteral ("sid"), m_currentSid);

    QXmppElement content;
    content.setTagName (QStringLiteral ("content"));
    content.setAttribute (QStringLiteral ("creator"), QStringLiteral ("initiator"));
    content.setAttribute (QStringLiteral ("name"), QStringLiteral ("voice"));

    QXmppElement description;
    description.setTagName (QStringLiteral ("description"));
    description.setAttribute (QStringLiteral ("xmlns"), kJINGLE_APP);

    QXmppElement sdpElement;
    sdpElement.setTagName (QStringLiteral ("sdp"));
    sdpElement.setValue(sdp);

    description.appendChild (sdpElement);
    content.appendChild (description);
    jingle.appendChild (content);

    QXmppIq iq;
    iq.setType (QXmppIq::Type::Set);
    iq.setTo (recipient);
    iq.setExtensions ({std::move (jingle)});

    return iq;
}

QXmppIq voip::signaling::jingle::JingleAction::transportAccept(const QString recipient, const QString sdp)
{
    QXmppElement jingle;
    jingle.setTagName (QStringLiteral ("jingle"));
    jingle.setAttribute (QStringLiteral ("xmlns"), kJINGLE_NAME);
    jingle.setAttribute (QStringLiteral ("action"), fromType (ActionType::TransportAccept));
    jingle.setAttribute (QStringLiteral ("initiator"), m_client->configuration ().jidBare ());
    jingle.setAttribute (QStringLiteral ("sid"), m_currentSid);

    QXmppElement content;
    content.setTagName (QStringLiteral ("content"));
    content.setAttribute (QStringLiteral ("creator"), QStringLiteral ("initiator"));
    content.setAttribute (QStringLiteral ("name"), QStringLiteral ("voice"));

    QXmppElement description;
    description.setTagName (QStringLiteral ("description"));
    description.setAttribute (QStringLiteral ("xmlns"), kJINGLE_APP);

    QXmppElement sdpElement;
    sdpElement.setTagName (QStringLiteral ("sdp"));
    sdpElement.setValue(sdp);

    description.appendChild (sdpElement);
    content.appendChild (description);
    jingle.appendChild (content);

    QXmppIq iq;
    iq.setType (QXmppIq::Type::Set);
    iq.setTo (recipient);
    iq.setExtensions ({std::move (jingle)});

    return iq;
}
























































