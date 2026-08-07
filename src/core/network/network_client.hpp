#ifndef CORE_NETWORK_NETWORK_CLIENT_HPP
#define CORE_NETWORK_NETWORK_CLIENT_HPP

#include "network_interceptor.hpp"
#include "network_request.hpp"
#include "network_response.hpp"
#include "certificate/certificate_chain_validator.hpp"

#include <QFuture>
#include <QNetworkAccessManager>
#include <QObject>

#include <memory>
#include <vector>

class QNetworkReply;
class QNetworkRequest;

namespace core {
namespace network {



class NetworkClient : public QObject
{
    Q_OBJECT
public:
    explicit NetworkClient(
        QUrl base_url,
        QObject *parent = nullptr
        );

    void addInterceptor(std::shared_ptr<INetworkInterceptor> interceptor);
    void clearInterceptors();

    [[nodiscard]]
    QFuture<NetworkResponse> send(NetworkRequest request);

    [[nodiscard]]
    QFuture<NetworkResponse> get(const QString &path, QUrlQuery query = {});

    [[nodiscard]]
    QFuture<NetworkResponse> post(
        const QString &path,
        const QJsonObject &body
        );

    [[nodiscard]]
    QFuture<NetworkResponse> put(
        const QString &path,
        const QJsonObject &body
        );

    [[nodiscard]]
    QFuture<NetworkResponse> patch(
        const QString &path,
        const QJsonObject &body
        );

    [[nodiscard]]
    QFuture<NetworkResponse> deleteRequest(
        const QString &path
        );

    [[nodiscard]]
    QUrl baseUrl() const;

    void setBaseUrl(const QUrl &base_url);

signals:
    void requestStarted(
        const QString &request_id,
        const QUrl &url
        );
    void requestFinished(
        const QString &request_id,
        const QUrl &url,
        int status_code
        );

private:
    struct RequestContext;;

    void executeAttempt(
        const std::shared_ptr<RequestContext> &context
        );

    [[nodiscard]]
    QNetworkRequest buildQtRequest(const NetworkRequest &request);

    [[nodiscard]]
    QNetworkReply *dispatch(
        const NetworkRequest &request,
        const QNetworkRequest &qt_request
        );

    [[nodiscard]]
    NetworkResponse createResponse(
        const NetworkRequest &request,
        QNetworkReply *reply,
        qint64 duration_ms,
        int attempt_count
        );

    [[nodiscard]]
    NetworkError mapNetworkError(QNetworkReply *reply) const;

    [[nodiscard]]
    NetworkError mapHttpStatus(int status_code) const;

    void applyRequestInterceptors(NetworkRequest &request);

    void applyResponseInterceptors(
        const NetworkRequest &request,
        NetworkResponse &response
        );

    [[nodiscard]]
    QUrl resolveUrl(const QUrl &request_url) const;

    [[nodiscard]]
    QString createRequestId() const;

    [[nodiscard]]
    int retryDelay(
        const RetryPolicy &policy,
        int retry_number
        ) const;

    QUrl m_baseUrl;
    QNetworkAccessManager m_manager;

    std::vector<std::shared_ptr<INetworkInterceptor>> m_interceptor;
    certificate::CertificateChainValidator m_certificateValidator;
};

} // namespace network
} // namespace core


#endif // CORE_NETWORK_NETWORK_CLIENT_HPP
