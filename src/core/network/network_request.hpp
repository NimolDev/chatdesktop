#ifndef CORE_NETWORK_NETWORK_REQUEST_HPP
#define CORE_NETWORK_NETWORK_REQUEST_HPP

#include "http_method.hpp"
#include "retry_policy.hpp"

#include <QByteArray>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>
#include <QUrlQuery>

namespace core {
namespace network {

struct NetworkRequest
{
    QUrl url;
    HttpMethod method = HttpMethod::Get;

    QByteArray body;

    QHash<QByteArray, QByteArray> headers;
    QUrlQuery query;

    int timeout_ms = 30000;

    bool parse_json_response = true;
    bool requires_authentication = true;

    RetryPolicy retry_policy;

    // Useful for logs and tracing.
    QString request_id;

    // Optional descriptive name:
    // "Login" "FetchDevices", ...
    QString operation_name;

    void setJsonBody(const QJsonObject &object)
    {
        body = QJsonDocument(object).toJson (QJsonDocument::Compact);

        headers.insert (
            QByteArrayLiteral ("Content-Type"),
            QByteArrayLiteral ("application/json")
            );
    }

    void setJsonBody(const QJsonArray &array)
    {
        body = QJsonDocument(array).toJson (QJsonDocument::Compact);

        headers.insert (
            QByteArrayLiteral ("Content-Type"),
            QByteArrayLiteral ("application/json")
            );
    }

    void addHeader(
        const QByteArray &name,
        const QByteArray &value
        )
    {
        headers.insert (name, value);
    }

    void addQueryItem(
        const QString &name,
        const QString &value
        )
    {
        query.addQueryItem (name, value);
    }

    [[deprecated("Use addQueryItem")]]
    void addQueryItme(
        const QString &name,
        const QString &value
        )
    {
        addQueryItem(name, value);
    }

    [[nodiscard]]
    QUrl resolvedUrl() const
    {
        QUrl result = url;
        if (!query.isEmpty ()) {
            result.setQuery (query);
        }
        return result;
    }

    [[nodiscard]]
    bool isValid() const
    {
        const QUrl finalUrl = resolvedUrl ();

        return finalUrl.isValid () &&
               !finalUrl.scheme ().isEmpty () &&
               !finalUrl.host ().isEmpty ();
    }
};

} // namespace network
} // namespace core

#endif // CORE_NETWORK_NETWORK_REQUEST_HPP
