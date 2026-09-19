#include "jingle_message.hpp"



#define kJINGLE_MESSAGE     "urn:xmpp:jingle-message:0"
#define kJINGLE_NAME        "urn:xmpp:jingle:1"
#define kJINGLE_TRANSPORT   "urn:xmpp:jingle:apps:rtp:info:1"
#define kJINGLE_APP_NAME    "urn:xmpp:jingle:apps:rtp:1"

namespace voip {
namespace signaling {
namespace jingle {


JingleMessage::JingleMessage(QObject *parent)
    : QObject(parent)
{}

void JingleMessage::parse(const QDomElement &element)
{
    m_streamTypes.clear();
    if (element.isNull()) {
        return;
    }

    QString sender;
    QDomElement payload = element;
    if (element.tagName() == QLatin1StringView("message")) {
        sender = element.attribute(QStringLiteral("from"));
        if (sender.trimmed().isEmpty() || element.attribute(QStringLiteral("type")) == QStringLiteral("error")) {
            return;
        }
        payload = {};
        // Hints, bodies and other extensions may precede the Jingle payload.
        for (auto child = element.firstChildElement(); !child.isNull(); child = child.nextSiblingElement()) {
            if (child.namespaceURI() == QLatin1StringView(kJINGLE_MESSAGE)) {
                payload = child;
                break;
            }
        }
    }
    if (payload.isNull() || payload.namespaceURI() != QLatin1StringView(kJINGLE_MESSAGE)) {
        return;
    }
    const auto type = fromJingleMessageTypeString(payload.tagName());
    const QString sessionId = payload.attribute(QStringLiteral("id"));
    if (!type || sessionId.trimmed().isEmpty()) {
        return;
    }

    if (*type == Propose) {
        for (auto description = payload.firstChildElement(); !description.isNull(); description = description.nextSiblingElement()) {
            if (description.tagName() != QLatin1StringView("description")
                || description.namespaceURI() != QLatin1StringView(kJINGLE_APP_NAME)) {
                continue;
            }
            const auto stream = fromStreamTypeString(description.attribute(QStringLiteral("media")));
            if (stream && !m_streamTypes.contains(*stream)) {
                m_streamTypes.append(*stream);
            }
        }
        if (m_streamTypes.isEmpty()) {
            return;
        }
    }

    QString reasonText;
    const auto reason = payload.firstChildElement(QStringLiteral("reason"));
    if (reason.namespaceURI() == QLatin1StringView(kJINGLE_NAME)) {
        for (auto child = reason.firstChildElement(); !child.isNull(); child = child.nextSiblingElement()) {
            if (child.namespaceURI() != QLatin1StringView(kJINGLE_NAME)) {
                continue;
            }
            if (child.tagName() == QLatin1StringView("text")) {
                reasonText = child.text();
                break;
            }
            if (reasonText.isEmpty()) {
                reasonText = child.tagName();
            }
        }
    }

    emit messageReceived(fromJingleMessageType(*type), sender, sessionId, m_streamTypes, reasonText);
    switch (*type) {
    case Propose:
        emit proposeReceived(m_streamTypes, sender, sessionId);
        break;
    case Accept:
        emit acceptReceived(sender, sessionId);
        break;
    case Ringing:
        emit ringingReceived(sender, sessionId);
        break;
    case Retract:
        emit retractReceived(reasonText, sessionId);
        break;
    case Proceed:
        emit proceedReceived(sender, sessionId);
        break;
    case Reject:
        emit rejectReceived(reasonText, sessionId);
        break;
    case Finish:
        emit finishReceived(reasonText, sessionId);
        break;
    }
}

/**
 * @brief JingleMessage::propose
 * @param recipient
 * @param streams
 * @param mid
 * @return QXmppMessage
 */
QXmppMessage JingleMessage::propose(
    const QString &recipient,
    const QList<StreamType> &streams,
    const QString &mid)
{

    QXmppElement propose;
    propose.setTagName (fromJingleMessageType (JingleMessageType::Propose));
    propose.setAttribute (QStringLiteral ("xmlns"), kJINGLE_MESSAGE);
    propose.setAttribute (QStringLiteral ("id"), mid);

    for (const auto stream : streams) {
        QXmppElement description;
        description.setTagName(QStringLiteral("description"));
        description.setAttribute(
            QStringLiteral("xmlns"),
            kJINGLE_APP_NAME);
        description.setAttribute(
            QStringLiteral("media"),
            fromStreamType(stream));

        propose.appendChild(std::move(description));
    }

    QXmppMessage message;
    message.setType (QXmppMessage::Type::Chat);
    message.setTo (recipient);
    message.setExtensions ({std::move (propose)});
    message.addHint(QXmppMessage::Store);
    return message;

}

/**
 * @brief JingleMessage::accept
 * @param recipient
 * @param mid
 * @return QXmppMesage
 */
QXmppMessage JingleMessage::accept(
    const QString &recipient,
    const QString &mid)
{
    QXmppElement accept;
    accept.setTagName (fromJingleMessageType (JingleMessageType::Accept));
    accept.setAttribute (QStringLiteral ("xmlns"), kJINGLE_MESSAGE);
    accept.setAttribute (QStringLiteral ("id"), mid);

    QXmppMessage message;
    message.setType (QXmppMessage::Type::Chat);
    message.setTo (recipient);
    message.setExtensions ({accept});
    return message;
}

QXmppMessage JingleMessage::proceed(
    const QString &recipient,
    const QString &mid)
{
    QXmppElement proceed;
    proceed.setTagName (fromJingleMessageType (JingleMessageType::Proceed));
    proceed.setAttribute (QStringLiteral ("xmlns"), kJINGLE_MESSAGE);
    proceed.setAttribute (QStringLiteral ("id"), mid);

    QXmppMessage message;
    message.setType (QXmppMessage::Type::Chat);
    message.setTo (recipient);
    message.setExtensions ({proceed});
    return message;
}

QXmppMessage JingleMessage::reject(
    const QString &recipient,
    const QString &mid)
{
    QXmppElement reject;
    reject.setTagName (fromJingleMessageType (JingleMessageType::Reject));
    reject.setAttribute (QStringLiteral ("xmlns"), kJINGLE_MESSAGE);
    reject.setAttribute (QStringLiteral ("id"), mid);

    QXmppElement reason;
    reason.setTagName (QStringLiteral ("reason"));
    reason.setAttribute (QStringLiteral ("xmlns"), kJINGLE_NAME);

    QXmppElement busy;
    busy.setTagName (fromJingleReason (JingleReason::Busy));

    QXmppElement text;
    // text.setAttribute (QStringLiteral ("text"), QStringLiteral ("Busy"));
    text.setTagName (QStringLiteral ("text"));
    text.setValue (QStringLiteral ("Busy"));

    reason.appendChild (busy);
    reason.appendChild (text);
    reject.appendChild (reason);

    QXmppMessage message;
    message.setType (QXmppMessage::Type::Chat);
    message.setTo (recipient);
    message.setExtensions ({std::move (reject)});
    return message;
}

QXmppMessage JingleMessage::retract(
    const QString &recipient,
    const QString &mid)
{

    QXmppElement retract;
    retract.setTagName (fromJingleMessageType (JingleMessageType::Retract));
    retract.setAttribute (QStringLiteral ("xmlns"), kJINGLE_MESSAGE);
    retract.setAttribute (QStringLiteral ("id"), mid);

    QXmppElement reason;
    reason.setTagName (QStringLiteral ("reason"));
    reason.setAttribute (QStringLiteral ("xmlns"), kJINGLE_NAME);

    QXmppElement cancel;
    cancel.setTagName (fromJingleReason (JingleReason::Cancel));

    QXmppElement text;
    // text.setAttribute (QStringLiteral ("text"), QStringLiteral ("Cancel"));
    text.setTagName (QStringLiteral ("text"));
    text.setValue (QStringLiteral ("Cancel"));

    reason.appendChild (std::move (cancel));
    reason.appendChild (std::move (text));
    retract.appendChild (std::move (reason));

    QXmppMessage message;
    message.setType (QXmppMessage::Type::Chat);
    message.setTo (recipient);
    message.setExtensions ({std::move (retract)});
    return message;
}


QXmppMessage JingleMessage::ringing(
    const QString &recipient,
    const QString &mid)
{
    QXmppElement ringing;
    ringing.setTagName (fromJingleMessageType (JingleMessageType::Ringing));
    ringing.setAttribute (QStringLiteral ("xmlns"), kJINGLE_MESSAGE);
    ringing.setAttribute (QStringLiteral ("id"), mid);

    QXmppMessage message;
    message.setType (QXmppMessage::Type::Chat);
    message.setTo (recipient);
    message.setExtensions ({std::move (ringing)});
    return message;
}




QXmppMessage JingleMessage::finish(
    const QString &recipient,
    const QString &mid)
{
    QXmppElement finish;
    finish.setTagName (fromJingleMessageType (JingleMessageType::Finish));
    finish.setAttribute (QStringLiteral ("xmlns"), kJINGLE_MESSAGE);
    finish.setAttribute (QStringLiteral ("id"), mid);

    QXmppElement reason;
    reason.setValue (QStringLiteral ("reason"));
    reason.setAttribute (QStringLiteral ("xmlns"), kJINGLE_NAME);

    QXmppElement success;
    success.setTagName (fromJingleReason (JingleReason::Success));

    QXmppElement text;
    text.setAttribute (QStringLiteral ("text"), QStringLiteral ("Success"));

    reason.appendChild (std::move (success));
    reason.appendChild (std::move (text));
    finish.appendChild (std::move (reason));

    QXmppMessage message;
    message.setType (QXmppMessage::Type::Chat);
    message.setTo (recipient);
    message.setExtensions ({std::move (finish)});
    return message;
}

const QList<StreamType> &JingleMessage::streamTypes() const noexcept
{
    return m_streamTypes;
}

} // namespace jingle
} // namespace signaling
} // namespace voip




/*
 * <message xmlns="jabber:client" to="ios@localhost/fc9a0299-7ddd-4bb3-abee-d81b05af5e3c" type="chat" from="a7b4677b2e374f04bdc5b081c333f05c@localhost/desktop-6e72d56d-b981-43dd-a284-17504e83ae0d"><reject xmlns="urn:xmpp:jingle-message:0" id="35AAA1D0-3DA7-4566-8C53-F9BC9463B652"></reject></message>
 *
 * <message to="ios@localhost/fc9a0299-7ddd-4bb3-abee-d81b05af5e3c" type="chat"><reject id="35AAA1D0-3DA7-4566-8C53-F9BC9463B652" xmlns="urn:xmpp:jingle-message:0"/></message>
 */





















