#pragma once

#include "app_constants.hpp"
#include "local_storage.hpp"

#include <QString>
#include <QtCore/qjsonobject.h>
#include <memory>

namespace core {
namespace storage {

class CacheStorage
{
public:
    static CacheStorage &instance();

    CacheStorage(const CacheStorage&) = delete;
    CacheStorage & operator=(const CacheStorage&) = delete;

    template<typename T>
    QString toString(const T &value)
    {
        using Type = std::decay_t<T>;

        if constexpr (std::is_same_v<Type, QString>) {
            return value;
        } else if constexpr (std::is_same_v<Type, QByteArray>) {
            return QString::fromUtf8(value);
        } else if constexpr (std::is_same_v<Type, QJsonObject>) {
            return QString::fromUtf8(
                QJsonDocument(value).toJson(QJsonDocument::Compact));
        } else {
            // Custom objects must provide: QJsonObject toJson() const.
            return QString::fromUtf8(
                QJsonDocument(value.toJson()).toJson(QJsonDocument::Compact));
        }
    }

    /**
     * @brief save
     * @param value: payload to save
     * @param logical_name: file name in plaintext
     * @param secret_ke: key for encypted text
     * @return
     */
    bool save(const QString &value, const QString &logical_name, const QString &secret_key);
    QString getUser(const QString &logical_name, const QString &secret_key);


private:
    CacheStorage() ;

    QString m_storagePath;
    std::unique_ptr<LocalStorage> m_storage;

};



} // namespace storage
} // namespace core
