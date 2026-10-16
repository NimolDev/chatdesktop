#pragma once
#include <QString>

namespace core {
namespace utils {

enum class DurationFormat {
    MinutesSeconds,
    MinutesSecondsTenths
};

inline QString  voiceRecordingDurationStr(
        quint32 duration,
        DurationFormat formate = DurationFormat::MinutesSecondsTenths
        )
{
    const qint64 milliseconds = std::max<qint64>(0, duration);
    const qint64 minutes = milliseconds / 60000;
    const qint64 seconds = milliseconds / 1000 % 60;

    if (formate == DurationFormat::MinutesSecondsTenths)  {
        const qint64 tenths = milliseconds / 100 % 10;
        return QStringLiteral("%1:%2:%3")
            .arg(minutes, 2, 10, QLatin1Char('0'))
            .arg(seconds, 2, 10, QLatin1Char('0'))
            .arg(tenths);
    } else {
        return QStringLiteral ("%1:%2")
            .arg (minutes, 2, 10, QLatin1Char('0'))
            .arg (seconds, 2, 10, QLatin1Char('0'));
    }
}

} // namespace utils
} // namespace core