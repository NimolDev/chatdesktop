#ifndef CORE_UTILS_DATE_TIME_UTILS_HPP
#define CORE_UTILS_DATE_TIME_UTILS_HPP

#include <QString>
#include <QDateTime>

namespace core {
namespace utils {

QString formateEpoch(qint64 epoch_seconds, const QString &formate = "yyyy-MM-dd HH:mm:ss");
QString formateEpochMs(qint64 epoch_miliseconds, const QString &formate = "yyyy-MM-dd HH:mm:ss");
QString formatChatTime(quint64 epoch_seconds);
QString formateChatTime(quint64 epoch_sencods);
QString formateConversationDate(const QString epoch_string);

} // namespace utils
} // namespace core
#endif // CORE_UTILS_DATE_TIME_UTILS_HPP
