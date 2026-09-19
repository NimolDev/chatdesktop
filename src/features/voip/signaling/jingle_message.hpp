#ifndef VOIP_SIGNALING_JINGLE_MESSAGE_HPP
#define VOIP_SIGNALING_JINGLE_MESSAGE_HPP

#include "QXmppMessage.h"

#include <QObject>
#include <QDomElement>
#include <QString>
#include <QList>

namespace voip {
namespace signaling {
namespace jingle {

enum JingleMessageType {
    Propose,
    Retract,
    Accept,
    Proceed,
    Reject,
    Finish,
    Ringing,
};

inline QString fromJingleMessageType(JingleMessageType type)
{
    switch (type) {
    case Propose:
        return QStringLiteral ("propose");
    case Retract:
        return QStringLiteral ("retract");
    case Accept:
        return QStringLiteral ("accept");
    case Proceed:
        return QStringLiteral ("proceed");
    case Reject:
        return QStringLiteral ("reject");
    case Finish:
        return QStringLiteral ("finish");
    case Ringing:
        return QStringLiteral ("ringing");
    }
    return QStringLiteral ("unkwon");
}
inline std::optional<JingleMessageType> fromJingleMessageTypeString (const QString &str)
{
    if (str == QStringLiteral ("propose")) { return JingleMessageType::Propose; }
    if (str == QStringLiteral ("retract")) { return JingleMessageType::Retract; }
    if (str == QStringLiteral ("accept")) { return JingleMessageType::Accept; }
    if (str == QStringLiteral ("proceed")) { return JingleMessageType::Proceed; }
    if (str == QStringLiteral ("reject")) { return JingleMessageType::Reject; };
    if (str == QStringLiteral ("finish")) { return JingleMessageType::Finish; }
    if (str == QStringLiteral ("ringing")) { return JingleMessageType::Ringing; }

    return std::nullopt;
}
enum class StreamType
{
    Audio,
    Video
};
inline QString fromStreamType(StreamType type)
{
    switch(type) {
    case StreamType::Audio: return QStringLiteral ("audio");
    case StreamType::Video: return QStringLiteral ("video");
    }
}
inline std::optional<StreamType> fromStreamTypeString(const QString &str)
{
    if (str == QStringLiteral ("audio")) { return StreamType::Audio; }
    if (str == QStringLiteral ("video")) { return StreamType::Video; }

    return std::nullopt;
}


enum class JingleReason
{
    Busy,
    Cancel,
    Success
};
inline QString fromJingleReason(JingleReason reason)
{
    switch(reason) {
    case JingleReason::Busy: return QStringLiteral ("busy");
    case JingleReason::Cancel: return QStringLiteral ("cancel");
    case JingleReason::Success: return QStringLiteral ("suceess");
    }
}

class JingleMessage : public QObject
{
    Q_OBJECT
public:
    explicit JingleMessage(QObject *parent = nullptr);
    void parse(const QDomElement &element);

    QXmppMessage propose(
        const QString &recipient,
        const QList<StreamType> &streams,
        const QString &mid);

    QXmppMessage retract(
        const QString &recipient,
        const QString &mid);

    QXmppMessage ringing(
        const QString &recipient,
        const QString &mid);

    QXmppMessage proceed(
        const QString &recipient,
        const QString &mid);

    QXmppMessage reject(
        const QString &recipient,
        const QString &mid);

    QXmppMessage accept(
        const QString &recipient,
        const QString &mid);

    QXmppMessage finish(
        const QString &recipient,
        const QString &mid);

    const QList<StreamType> &streamTypes() const noexcept;


signals:
    void messageReceived(const QString &action, const QString &sender,
                         const QString &sessionId, const QList<voip::signaling::jingle::StreamType> &streams,
                         const QString &reason);
    void proposeReceived(
        const QList<voip::signaling::jingle::StreamType> &streams,
        const QString &from,
        const QString &mid);
    void retractReceived(const QString &retract, const QString &mid);
    void ringingReceived(const QString &ringing, const QString &mid);
    void proceedReceived(const QString &proceed, const QString &mid);
    void rejectReceived(const QString &reject, const QString &mid);
    void acceptReceived(const QString &accept, const QString &mid);
    void finishReceived(const QString &finish, const QString &mid);

private:
    QList<StreamType> m_streamTypes;
};


} // namespace jingle
} // namespace signaling
} // namespace voip








































#endif // VOIP_SIGNALING_JINGLE_MESSAGE_HPP
