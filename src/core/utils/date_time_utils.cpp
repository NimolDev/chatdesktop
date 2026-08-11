#include "date_time_utils.hpp"


namespace  core {
namespace utils {


QString formateEpoch(qint64 epoch_seconds, const QString &formate)
{
    return QDateTime::fromSecsSinceEpoch (epoch_seconds)
        .toLocalTime ()
        .toString (formate);
}

QString formateEpochMs(qint64 epoch_miliseconds, const QString &formate)
{
    return QDateTime::fromMSecsSinceEpoch (epoch_miliseconds)
        .toLocalTime ()
        .toString (formate);

}

QString formatChatTime(quint64 epoch_seconds)
{
    return QDateTime::fromSecsSinceEpoch (epoch_seconds)
        .toLocalTime ()
        .toString ("hh:mm AP");

}

QString formateChatTime(quint64 epoch_sencods)
{
    return QDateTime::fromSecsSinceEpoch (epoch_sencods)
        .toLocalTime ()
        .toString ("dd MMM yyyy");

}

QString formateConversationDate(const QString epoch_string)
{
    // Convert epoch in String to Integer
    bool ok = false;
    qint64 epoch = epoch_string.toLongLong (&ok);
    if (!ok)  return QStringLiteral ("N/A");

    QDateTime dt = QDateTime::fromMSecsSinceEpoch (epoch).toLocalTime ();
    QDate today = QDate::currentDate ();

    if (dt.date () == today) {
        return dt.toString ("hh:mm AP");
    }
    if (dt.date () == today.addDays (-1)) {
        return "Yesterday";
    }
    if(dt.date () > today.addDays (-7)) {
        return dt.toString ("ddd"); // Mon, Tue, ...
    }
    return dt.toString ("dd MMM yyyy");

}


} // namespace utils
} // namespace core