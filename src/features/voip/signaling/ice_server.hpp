#ifndef VOIP_SIGNALING_ICE_SERVER_HPP
#define VOIP_SIGNALING_ICE_SERVER_HPP

#include <QString>
#include <QtGlobal>

namespace voip {
namespace signaling {

struct IceServer
{
    enum IceType {
        Turn,
        Turns,
        Stun
    };
    static QString toString(IceType type) {
        switch(type) {
        case Turn:
            return QStringLiteral ("turn");
        case Stun:
            return QStringLiteral ("stun");
        case Turns:
            return QStringLiteral ("turns");
            break;
        }
    }
    static std::optional<IceType> toType(const QString &str)
    {
        if (str == QStringLiteral ("turn")) { return IceType::Turn; }
        if (str == QStringLiteral ("turns")) { return IceType::Turns; }
        if (str == QStringLiteral ("stun")) { return IceType::Stun; }
        return std::nullopt;
    }

    IceType type;
    QString host;
    QString port;
    QString username;
    QString credential;
};

} // namespace signaling
} // namespace voip

#endif // VOIP_SIGNALING_ICE_SERVER_HPP
