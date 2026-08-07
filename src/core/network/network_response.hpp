#ifndef CORE_NETWORK_NETWORK_RESPONSE_HPP
#define CORE_NETWORK_NETWORK_RESPONSE_HPP

#include "network_error.hpp"

#include <QByteArray>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>
#include <QUrl>

namespace core {
namespace network {

using namespace error;

struct NetworkResponse
{
    int status_code = 0;

    QByteArray body;
    QJsonDocument json;

    QHash<QByteArray, QByteArray> headers;

    NetworkError error = NetworkError::None;
    QString error_message;

    QUrl request_url;
    QString request_id;

    int attempt_count =1;
    qint64 duration_ms = 0;

    [[nodiscard]]
    bool isSuccess() const
    {
        return error == NetworkError::None &&
               status_code >= 200 &&
               status_code < 300;

    }
    [[nodiscard]]
    bool hasError() const
    {
        return !isSuccess ();
    }

    [[nodiscard]]
    bool hasJsonObject() const
    {
        return json.isObject ();
    }

    [[nodiscard]]
    bool hasJsonArray() const
    {
        return json.isArray ();
    }

    [[nodiscard]]
    QJsonObject jsonObject() const
    {
        return json.isObject ()
        ? json.object ()
        : QJsonObject {};
    }

    [[nodiscard]]
    QJsonArray jsonArray() const
    {
        return json.isArray ()
        ? json.array ()
        : QJsonArray {};
    }

    [[nodiscard]]
    QByteArray header (const QByteArray &name) const
    {
        return headers.value (name);
    }
};


} // namespace network
} // namespace core

#endif // CORE_NETWORK_NETWORK_RESPONSE_HPP
