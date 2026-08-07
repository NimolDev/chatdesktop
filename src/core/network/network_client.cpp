#include "network_client.hpp"

#include <QElapsedTimer>
#include <QHttpHeaders>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPromise>
#include <QRandomGenerator>
#include <QSslError>
#include <QTimer>
#include <QUuid>

#include <utility>

namespace core {
namespace network {

using namespace certificate;

struct NetworkClient::RequestContext
{
    NetworkRequest request;

    std::shared_ptr<QPromise<NetworkResponse>> promise;

    int complete_retries = 0;
    int attempt_count = 0;

    QElapsedTimer total_timer;
};

NetworkClient::NetworkClient(QUrl base_url, QObject *parent)
    : QObject(parent),
    m_baseUrl(base_url),
    m_manager(this)
{}

void NetworkClient::addInterceptor(std::shared_ptr<INetworkInterceptor> interceptor)
{
    if(!interceptor) {
        return;
    }

    m_interceptor.push_back (std::move(interceptor));
}

void NetworkClient::clearInterceptors()
{
    m_interceptor.clear ();
}

QFuture<NetworkResponse> NetworkClient::send(NetworkRequest request)
{
    auto promise = std::make_shared<QPromise<NetworkResponse>>();

    promise->start();

    QFuture<NetworkResponse> future = promise->future ();

    request.url = resolveUrl (request.url);

    if (request.request_id.isEmpty ()) {
        request.request_id = createRequestId ();
    }

    applyRequestInterceptors (request);

    if (!request.isValid ()) {
        NetworkResponse response;
        response.request_url = request.url;
        response.request_id = request.request_id;
        response.error = NetworkError::InvalidUrl;
        response.error_message = QStringLiteral ("The request URL is invalid");

        applyResponseInterceptors (request, response);

        promise -> addResult (response);
        promise -> finish ();
        return future;
    }

    auto context = std::make_shared <RequestContext>();
    context->request = std::move(request);
    context->promise = std::move(promise);
    context->total_timer.start ();

    executeAttempt(context);

    return future;
}

QFuture<NetworkResponse> NetworkClient::get(const QString &path, QUrlQuery query)
{
    NetworkRequest request;
    request.url = QUrl(path);
    request.method = HttpMethod::Get;
    request.query = std::move (query);

    return send(std::move (request));
}

QFuture<NetworkResponse> NetworkClient::post(const QString &path, const QJsonObject &body)
{
    NetworkRequest request;
    request.url = QUrl(path);
    request.method = HttpMethod::Post;
    request.setJsonBody (body);

    return send(std::move(request));
}

QFuture<NetworkResponse> NetworkClient::put(const QString &path, const QJsonObject &body)
{
    NetworkRequest request;
    request.url = QUrl(path);
    request.method = HttpMethod::Put;
    request.setJsonBody (body);

    return send(std::move(request));
}

QFuture<NetworkResponse> NetworkClient::patch(const QString &path, const QJsonObject &body)
{
    NetworkRequest request;
    request.url = QUrl(path);
    request.method = HttpMethod::Patch;
    request.setJsonBody (body);

    return send(std::move(request));
}

QFuture<NetworkResponse> NetworkClient::deleteRequest(const QString &path)
{
    NetworkRequest request;
    request.url = QUrl(path);
    request.method = HttpMethod::Delete;

    return send(std::move(request));
}

QUrl NetworkClient::baseUrl() const
{
    return m_baseUrl;
}

void NetworkClient::setBaseUrl(const QUrl &base_url)
{
    m_baseUrl = base_url;
}



void NetworkClient::executeAttempt(const std::shared_ptr<RequestContext> &context)
{
    if (!context || !context->promise) {
        return;
    }

    // Future cancallation does not automatically abort a QNetworkReply
    // In this baseline. It prevents another attempt from stating.
    if (context->promise->isCanceled ()) {
        context->promise->finish ();
        return;
    }
    ++context->attempt_count;

    const QNetworkRequest qt_request = buildQtRequest (context->request);

    QNetworkReply *reply = dispatch (context->request, qt_request);

    if (reply == nullptr) {
        NetworkResponse response;
        response.request_url = context->request.resolvedUrl ();
        response.request_id = context->request.request_id;
        response.attempt_count = context->attempt_count;
        response.duration_ms = context->total_timer.elapsed ();
        response.error = NetworkError::InvalideRequest;
        response.error_message = QStringLiteral ("Unable to dispatch network request");

        applyResponseInterceptors (context->request, response);

        context->promise->addResult (response);
        context->promise->finish ();
        return;
    }

    emit requestStarted (
        context->request.request_id,
        context->request.resolvedUrl ()
        );

    auto timed_out = std::make_shared <bool>(false);

    QTimer *timeout_timer = new QTimer(reply);
    timeout_timer->setSingleShot (true);

    connect (
        timeout_timer,
        &QTimer::timeout,
        reply,
        [reply, timed_out]() {
            *timed_out = true;
            reply->abort ();
        }
        );

    timeout_timer->start (context->request.timeout_ms);


    // NOTE: Check SSL Certificate to accept connection to Protect MITM attacke
    connect(
        reply,
        &QNetworkReply::encrypted,
        reply,
        [this, reply]() {
            const QSslConfiguration configuration = reply->sslConfiguration ();

            const QList<QSslCertificate> chain = configuration.peerCertificateChain ();
            const certificate::ValidatorResult result =
                m_certificateValidator.validate(
                    chain
                    );
            if (!result.isValid()) {
                qWarning() << "TLS certificate validation failed:" << result.message;
                reply->abort();
                return;
            }

            qDebug()  << result.message;
        }
        );

    connect (
        reply,
        &QNetworkReply::finished,
        this,
        [this, context, reply, timeout_timer, timed_out]() {
            timeout_timer->stop ();

            NetworkResponse response = createResponse (
                context->request,
                reply,
                context->total_timer.elapsed (),
                context->attempt_count
                );

            if (*timed_out) {
                response.error = NetworkError::Timeout;
                response.error_message = QStringLiteral ("The request exceeded its timeout");
            }

            emit requestFinished (
                response.request_id,
                response.request_url,
                response.status_code
                );

            const bool should_retry = context->request.retry_policy
                                          .shouldRetry (
                                              context->request.method,
                                              response.error,
                                              context->complete_retries);

            reply->deleteLater ();

            if (should_retry && !context->promise->isCanceled()) {
                const int delay_ms = retryDelay (
                    context->request.retry_policy,
                    context->complete_retries
                    );

                ++context->complete_retries;

                QTimer::singleShot (
                    delay_ms,
                    this,
                    [this, context]() {
                        executeAttempt(context);
                    }
                    );
                return;
            }

            applyResponseInterceptors (context->request, response);
            context->promise->addResult (std::move(response));
            context->promise->finish ();
        }
        );
}

QNetworkRequest NetworkClient::buildQtRequest(const NetworkRequest &request)
{
    QNetworkRequest qtRequest(request.resolvedUrl ());

    qtRequest.setHeader (
        QNetworkRequest::UserAgentHeader,
        QStringLiteral ("Qt-Desktop")
        );

    qtRequest.setRawHeader (
        QByteArrayLiteral ("Accept"),
        QByteArrayLiteral ("application/json")
        );

    for (auto iterator = request.headers.cbegin ();
         iterator != request.headers.cend ();
         ++iterator) {
        qtRequest.setRawHeader (iterator.key (), iterator.value ());
    }
    return qtRequest;
}


QNetworkReply *NetworkClient::dispatch(const NetworkRequest &request, const QNetworkRequest &qt_request)
{
    switch (request.method) {
    case HttpMethod::Get:
        return m_manager.get(qt_request);

    case HttpMethod::Post:
        return m_manager.post(
            qt_request,
            request.body
            );

    case HttpMethod::Put:
        return m_manager.put(
            qt_request,
            request.body
            );

    case HttpMethod::Patch:
        return m_manager.sendCustomRequest(
            qt_request,
            QByteArrayLiteral("PATCH"),
            request.body
            );

    case HttpMethod::Delete:

        if (request.body.isEmpty()) {
            return m_manager.deleteResource(
                qt_request
                );
        }

        return m_manager.sendCustomRequest(
            qt_request,
            QByteArrayLiteral("DELETE"),
            request.body
            );

    case HttpMethod::Head:
        return m_manager.head(qt_request);
    }

    return nullptr;
}

NetworkResponse NetworkClient::createResponse(const NetworkRequest &request, QNetworkReply *reply, qint64 duration_ms, int attempt_count)
{
    NetworkResponse response;
    response.body = reply->readAll();
    response.request_url = request.resolvedUrl();
    response.request_id = request.request_id;
    response.duration_ms = duration_ms;
    response.attempt_count = attempt_count;
    response.status_code = reply->attribute(
                                    QNetworkRequest::HttpStatusCodeAttribute
                                    ).toInt();

    const QList<QByteArray> header_names = reply->rawHeaderList();

    for (const QByteArray &name : header_names) {
        response.headers.insert(
            name,
            reply->rawHeader(name)
            );
    }

    response.error = mapNetworkError(reply);

    if (response.error == NetworkError::None) {
        response.error = mapHttpStatus (response.status_code);
    }

    if (response.error != NetworkError::None) {
        response.error_message = reply->errorString();

        // Prefer a structured backend message when available.
        QJsonParseError parse_error;
        const QJsonDocument error_document = QJsonDocument::fromJson(
            response.body,
            &parse_error
            );

        if (parse_error.error == QJsonParseError::NoError &&
            error_document.isObject()) {

            const QJsonObject error_object = error_document.object();
            const QString backend_message = error_object.value( QStringLiteral("message")).toString();

            if (!backend_message.isEmpty()) {
                response.error_message = backend_message;
            }
        }

        if (response.error_message.isEmpty()) {
            response.error_message =  networkErrorToString(response.error);
        }

    }

    if (request.parse_json_response &&
        !response.body.trimmed().isEmpty()) {

        QJsonParseError parseError;
        response.json = QJsonDocument::fromJson(
            response.body,
            &parseError
            );

        if (parseError.error !=
                QJsonParseError::NoError &&
            response.error == NetworkError::None) {
            response.error =  NetworkError::JsonParseError;

            response.error_message = parseError.errorString();
        }
    }

    return response;
}

NetworkError NetworkClient::mapNetworkError(QNetworkReply *reply) const
{
    switch (reply->error()) {
    case QNetworkReply::NoError:
        return NetworkError::None;
    case QNetworkReply::ConnectionRefusedError:
        return NetworkError::ConnectionRefused;
    case QNetworkReply::RemoteHostClosedError:
        return NetworkError::RemoteHostClose;
    case QNetworkReply::HostNotFoundError:
        return NetworkError::HostNotFound;
    case QNetworkReply::TimeoutError:
        return NetworkError::Timeout;
    case QNetworkReply::OperationCanceledError:
        return NetworkError::OperationCancelled;
    case QNetworkReply::SslHandshakeFailedError:
        return NetworkError::SslHandshakeFailed;
    case QNetworkReply::TemporaryNetworkFailureError:
        return NetworkError::TemporaryNetworkFailure;
    case QNetworkReply::NetworkSessionFailedError:
        return NetworkError::NetworkSessionFailed;
    default:
        break;
    }

    // HTTP errors are mapped separately from status codes.
    if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).isValid()) {
        return NetworkError::None;
    }
    return NetworkError::UnknownNetworkError;
}

NetworkError NetworkClient::mapHttpStatus(int status_code) const
{
    if (status_code >= 200 &&
        status_code < 300) {
        return NetworkError::None;
    }

    switch (status_code) {
    case 0:
        return NetworkError::UnknownNetworkError;
    case 400:
        return NetworkError::BadRequest;
    case 401:
        return NetworkError::Unauthorized;
    case 403:
        return NetworkError::Forbidden;
    case 404:
        return NetworkError::NotFound;
    case 409:
        return NetworkError::Conflict;
    case 422:
        return NetworkError::ValidationFailed;
    case 429:
        return NetworkError::TooManyRequest;
    case 500:
        return NetworkError::InternalServerError;
    case 502:
        return NetworkError::BadGateway;
    case 503:
        return NetworkError::ServiceUnavailable;
    case 504:
        return NetworkError::GatewayTimeout;
    default:
        break;
    }

    if (status_code >= 500) {
        return NetworkError::InternalServerError;
    }
    if (status_code >= 400) {
        return NetworkError::UnknownHttpError;
    }
    return NetworkError::None;
}

void NetworkClient::applyRequestInterceptors(NetworkRequest &request)
{
    for (const auto &interceptor :
         m_interceptor) {
        if (interceptor) {
            interceptor->interceptRequest(
                request
                );
        }
    }
}

void NetworkClient::applyResponseInterceptors(const NetworkRequest &request, NetworkResponse &response)
{
    // Reverse order mirrors middleware stack unwinding.
    for (auto iterator = m_interceptor.rbegin();
         iterator != m_interceptor.rend();
         ++iterator) {

        if (*iterator) {
            (*iterator)->interceptorResponse(request, response);
        }

    }
}

QUrl NetworkClient::resolveUrl(const QUrl &request_url) const
{
    if (!request_url.isRelative()) {
        return request_url;
    }
    return m_baseUrl.resolved(request_url);
}

QString NetworkClient::createRequestId() const
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

int NetworkClient::retryDelay(const RetryPolicy &policy, int retry_number) const
{
    int delayMs = policy.delayForRetry(retry_number);

    if (!policy.use_jitter) {
        return delayMs;
    }
    // Adds between 0% and 25% random delay.
    const int maximumJitter = qMax(1, delayMs / 4);
    const int jitter = QRandomGenerator::global()->bounded(maximumJitter);
    return delayMs + jitter;
}


} // namespace network
} // namespace core