#ifndef FEATURES_CHAT_DATA_DTO_MESSAGE_STORE_HPP
#define FEATURES_CHAT_DATA_DTO_MESSAGE_STORE_HPP

#include <QString>
#include <QJsonObject>

namespace data {
namespace dto {
struct MesssageStore
{
    QString recipient_id;
    QString body;

    QJsonObject toJson() {
        QJsonObject object;
        object.insert (QStringLiteral ("recipient_id"), recipient_id);
        object.insert (QStringLiteral ("body"), body);
        return object;
    }

};
} // namespace dto
} // namespace data

#endif // FEATURES_CHAT_DATA_DTO_MESSAGE_STORE_HPP
