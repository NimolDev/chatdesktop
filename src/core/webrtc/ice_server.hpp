#ifndef CORE_WEBRTC_ICE_SERVER_HPP
#define CORE_WEBRTC_ICE_SERVER_HPP

#include <QString>

namespace core {
namespace rtc {

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
        return {};
    }

    QString url() const
    {
        QString address = host.trimmed();
        if (address.contains(QLatin1Char(':')) && !address.startsWith(QLatin1Char('['))) {
            address = QLatin1Char('[') + address + QLatin1Char(']');
        }
        QString result = toString(type) + QLatin1Char(':') + address;
        if (!port.isEmpty()) {
            result += QLatin1Char(':') + port;
        }
        return result;
    }

    IceType type;
    QString host;
    QString port;
    QString username;
    QString credential;
};

} // namespace rtc

} // namespace core
#endif // CORE_WEBRTC_ICE_SERVER_HPP
