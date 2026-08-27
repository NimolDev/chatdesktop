#ifndef CORE_NETWORK_JSON_SERIALIZER_HPP
#define CORE_NETWORK_JSON_SERIALIZER_HPP

#include <QByteArray>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QList>
#include <QString>

#include <optional>

namespace core {
namespace network {

class JsonSerializer
{
public:
    JsonSerializer() = delete;

    static std::optional<QJsonDocument> parse(
        const QByteArray &data,
        QString *error_messaage = nullptr
        )
    {
        QJsonParseError parse_error;

        const QJsonDocument document = QJsonDocument::fromJson (data,&parse_error);

        if (parse_error.error != QJsonParseError::NoError) {
            if (error_messaage  != nullptr) {
                *error_messaage = parse_error.errorString ();
            }
            return std::nullopt;
        }
        return document;
    }
    static QByteArray serialize(
        const QJsonObject &object,
        QJsonDocument::JsonFormat format = QJsonDocument::Compact
        ) {
        return QJsonDocument(object).toJson (format);
    }
    static QByteArray serialize(
        const QJsonArray &array,
        QJsonDocument::JsonFormat format = QJsonDocument::Compact
        ) {
        return QJsonDocument(array).toJson (format);
    }

    template <typename T>
    static std::optional<T> fromObject(
        const QJsonObject &object,
        QString *error_message = nullptr
        ) {
        return T::fromJson(
            object,
            error_message
            );
    }

    template <typename T>
    static std::optional<QList<T>> fromArray(
        const QJsonArray &array,
        QString *error_message = nullptr
        ) {
        QList<T> results;
        results.reserve (array.size ());

        for (qsizetype index = 0; index < array.size (); ++index) {
            const QJsonValue value = array.at (index);
            if (!value.isObject ()) {
                if (error_message != nullptr) {
                    *error_message = QStringLiteral ("Item at index %1 is not an object").arg (index);
                }
                return std::nullopt;
            }

            QString item_error;

            std::optional<T> item = T::fromJson(
                value.toObject (),
                &item_error
                );
            if (!item.has_value()) {
                if (error_message != nullptr) {
                    *error_message = QStringLiteral ("Invalid item at index %1: %2").arg (index). arg(item_error);
                }
                return std::nullopt;
            }
            results.append (std::move (item.value ()));
        }
        return results;
    }

    template<typename T>
    static QJsonObject toObject(const T &value)
    {
        return value.toJson();
    }

    template<typename T>
    static QJsonArray toArray(const QList<T> &items)
    {
        QJsonArray array;
        for (const T &item: items) {
            array.append (item.toJson());
        }
        return array;
    }

};

} // namespace network
} // namespace core


#endif // CORE_NETWORK_JSON_SERIALIZER_HPP
