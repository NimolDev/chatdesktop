#include "xmpp_manager.hpp"


#include <QTimer>
#include <QPromise>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QHostAddress>
#include <QXmppUploadRequestManager.h>
#include <QXmppHttpUploadIq.h>
#include <QXmppDiscoveryManager.h>
#include <QXmppTask.h>
#include <QXmppSendResult.h>
#include <QThread>
#include <QUuid>

namespace core {
namespace xmpp {

XmppManager::XmppManager(QObject *parent)
    : QObject(parent)
{
    qRegisterMetaType<core::xmpp::Message>();
    qRegisterMetaType<core::xmpp::Presence>();
}

void XmppManager::initialize()
{
    Q_ASSERT(QThread::currentThread() == thread());
    if (m_client) {
        return;
    }
    m_client = new QXmppClient(this);
    initializeHandlers ();
    initializeSignals ();
    emit clientInitialized(m_client);

    qDebug() << "XmppManager thread:"
             << QThread::currentThread();

    qDebug() << "QXmppClient thread:"
             << m_client->thread();
}


XmppManager::ConnectionState XmppManager::connectionState() const noexcept
{
    if (QThread::currentThread() == thread()) {
        return m_connectionState;
    }

    ConnectionState state = ConnectionState::Disconnected;
    QMetaObject::invokeMethod(
        const_cast<XmppManager *>(this),
        [this, &state]() { state = m_connectionState; },
        Qt::BlockingQueuedConnection);
    return state;
}
bool XmppManager::isConnected() const noexcept
{
    if (QThread::currentThread() == thread()) {
        return m_client && m_client->isConnected();
    }

    bool connected = false;
    QMetaObject::invokeMethod(
        const_cast<XmppManager *>(this),
        [this, &connected]() { connected = m_client && m_client->isConnected(); },
        Qt::BlockingQueuedConnection);
    return connected;
}
void XmppManager::requestExternalService()
{
    if (QThread::currentThread () != thread()) {
        QMetaObject::invokeMethod (this,
                                  &XmppManager::requestExternalService,
                                  Qt::QueuedConnection);
    }
    m_discovery->requestExtDiscoQuery (QStringLiteral ("localhost"));
}


void XmppManager::connectToServer(
    const QString &jid,
    const QString &password,
    const QString &host,
    quint16 port)
{
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(
            this,
            [this, jid, password, host, port]() {
                connectToServer(jid, password, host, port);
            },
            Qt::QueuedConnection);
        return;
    }

    Q_ASSERT(m_client);
    const QString normalized_jid = jid.trimmed();
    if (normalized_jid.isEmpty()) {
        const QString error = QStringLiteral("XMPP JID cannot be empty.");
        setLastError(error);
        emit connectionFailed();
        return;
    }

    if (password.isEmpty()) {
        const QString error = QStringLiteral("XMPP password cannot be empty.");
        setLastError(error);
        emit connectionFailed();
        return;
    }

    if (!normalized_jid.contains(QLatin1Char('@'))) {
        const QString error = QStringLiteral("Invalid XMPP JID. Expected user@domain.");
        setLastError(error);
        emit connectionFailed();
        return;
    }

    const ConnectionParameters parameters {
        .jid = normalized_jid,
        .password = password,
        .host = host.trimmed(),
        // .host  = "172.16.28.253",
        .port = port
    };

    if (m_client->state() != QXmppClient::DisconnectedState) {
        m_pendingConnection = parameters;
        m_client->disconnectFromServer();
        return;
    }

    startConnection(parameters);
}

void XmppManager::startConnection(const ConnectionParameters &parameters)
{
    QXmppConfiguration configuration;
    configuration.setJid(parameters.jid);
    configuration.setPassword(parameters.password);
    configuration.setResource (
        QString("desktop-%1").arg (QUuid::createUuid ().toString (QUuid::WithoutBraces))
        );
    if (!parameters.host.isEmpty()) {
        configuration.setHost(parameters.host);
    }
    if (parameters.port != 0) {
        configuration.setPort(parameters.port);
    }
    // QXmpp resolves the server through the JID domain's SRV records.
    configuration.setAutoReconnectionEnabled(true);
    configuration.setStreamSecurityMode (QXmppConfiguration::TLSRequired);

    configuration.setIgnoreSslErrors (true);

    QXmppPresence presence;
    presence.setType(QXmppPresence::Available);
    presence.setStatusText(QStringLiteral("Online"));
    updateState(ConnectionState::Connecting);
    setLastError({});

    m_client->connectToServer(configuration, presence);
}

void XmppManager::closeConnection()
{
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(
            this,
            &XmppManager::closeConnection,
            Qt::QueuedConnection);
        return;
    }

    if (!m_client) {
        return;
    }
    m_pendingConnection.reset();

    if (m_client->state() == QXmppClient::DisconnectedState) {
        updateState(ConnectionState::Disconnected);
        return;
    }

    QXmppPresence presence;
    presence.setType(QXmppPresence::Unavailable);
    presence.setStatusText(QStringLiteral("Offline"));
    m_client->setClientPresence(presence);
    m_client->disconnectFromServer();
}


void XmppManager::initializeHandlers()
{
    if (!m_client->findExtension<QXmppDiscoveryManager>()) {
        m_client->addExtension(new QXmppDiscoveryManager);
    }
    if (!m_client->findExtension<QXmppUploadRequestManager>()) {
        m_client->addExtension(new QXmppUploadRequestManager);
    }
    auto *network = new QNetworkAccessManager(m_client);
    network->setTransferTimeout(60000);
    m_httpUpload = new QXmppHttpUploadManager(network);
    m_client->addExtension(m_httpUpload);

    m_roster = m_client->QXmppClient::findExtension<QXmppRosterManager>();
    if (!m_roster) {
        qWarning() << "QXmppRosterManager not available";
    }
    // m_discovery = std::make_unique<core::xmpp::XmppServiceDiscovery> (m_client, this);
    // connect(m_discovery.get (),
    //         &core::xmpp::XmppServiceDiscovery::externalServiceReceived,
    //         this,
    //         &XmppManager::externalServiceReceived);
}

void XmppManager::initializeSignals()
{
    connect(
        m_client,
        &QXmppClient::stateChanged,
        this,
        [this](QXmppClient::State state) {

            // qDebug() << "Status: " << state;
            switch (state) {
            case QXmppClient::DisconnectedState:
                updateState (ConnectionState::Disconnected);
                qDebug() << "Status: " << state;
                break;
            case QXmppClient::ConnectingState: {
                updateState (ConnectionState::Connecting);
                break;
            }
            case QXmppClient::ConnectedState:
                updateState (ConnectionState::Connected);
                // m_discovery->requestExtDiscoQuery (QStringLiteral("localhost"));
                // QXmppPresence presence;
                // presence.setType(QXmppPresence::Available);
                // presence.setStatusText(QStringLiteral("Online"));
                // m_client->setClientPresence(presence);
                break;
            }
        }
        );

    connect (
        m_client,
        &QXmppClient::connected,
        this,
        [this]() {
            setLastError ({});
            m_currentJid = m_client->configuration ().jid ();
            qInfo() << "XMPP connected: " << m_client->configuration ().jid ();
            emit connectedChanged ();
        }
        );

    connect(
        m_client,
        &QXmppClient::disconnected,
        this,
        [this]() {
            setLastError ({});
            qInfo() << "XMPP Disconnected";
            updateState (ConnectionState::Disconnected);

            if (!m_pendingConnection.has_value()) {
                return;
            }

            const ConnectionParameters parameters =
                std::move(m_pendingConnection.value());
            m_pendingConnection.reset();

            // Let QXmpp finish tearing down its socket before reconnecting.
            QTimer::singleShot(0, this, [this, parameters] {
                if (m_client->state() == QXmppClient::DisconnectedState) {
                    startConnection(parameters);
                } else {
                    m_pendingConnection = parameters;
                }
            });
        }
        );
    if (m_roster) {
        connect(
            m_roster,
            &QXmppRosterManager::subscriptionRequestReceived,
            this,
            [this](const QString &jid, const QXmppPresence &presence) {
                qDebug() << "Subscription request from:" << jid;
                qDebug() << "Presence from:" << presence.from();

                if (!m_roster->acceptSubscription(jid)) {
                    qWarning() << "Failed to accept subscription from:" << jid;
                }
            }
            );
    }


    // connect (
    //     &m_client,
    //     &QXmppClient::error,
    //     this,
    //     [this](QXmppError &error) {
    //         qWarning() << "XMPP Error:" <<error.description;
    //     }
    //     );
    // connect(
    //     &m_client,
    //     &QXmppClient::errorOccurred,
    //     this,
    //     [this](QXmppError &error){
    //         qWarning() << "Xmpp occurred:"<<error.description;
    //     }
    //     );
    // connect(
    //     &m_client,
    //     &QXmppClient::loggerChanged,
    //     this,
    //     [this](QXmppLogger &logger) {
    //         qWarning() << "XMPP Logger:" << logger.AnyMessage;
    //     }
    //     );

    connect(
        m_client,
        &QXmppClient::messageReceived,
        this,
        &XmppManager::onMessageReceived
        );
    connect(
        m_client,
        &QXmppClient::presenceReceived,
        this,
        &XmppManager::onPresenceReceived
        );
    connect (
        m_client,
        &QXmppClient::iqReceived,
        this,
        &XmppManager::onIQReceived
        );


    // auto *logger = m_client->logger();

    // logger->setLoggingType(QXmppLogger::StdoutLogging);
    // logger->setMessageTypes(QXmppLogger::AnyMessage);
}

void XmppManager::onMessageReceived(const QXmppMessage &message)
{
    const Message msg = Message::map (message);

    if (message.body().isEmpty ()) {
          qWarning()  << "Message body is empty";
          return;
    }
    emit messageReceived (msg);
}

void XmppManager::onPresenceReceived(const QXmppPresence &presence)
{
    // Servers normally echo our initial presence. Ignore only this exact
    // resource, while still accepting presence from our other devices.
    if (presence.from() == m_currentJid) {
        return;
    }


    qDebug() << "Presene: "<< presence.from ();
    const Presence mappedPresence = Presence::map(presence);
    emit presenceReceived(mappedPresence);
}

void XmppManager::onIQReceived(const QXmppIq &iq)
{
    // qDebug() << "---- XmppIQ Received -----";
    // qDebug() << "From: " << iq.from ();
    // qDebug() << "To: " << iq.to ();
    // qDebug() << "Type: " << iq.type ();
    // const QXmppElementList extensions = iq.extensions();

    // qDebug() << "Extension count:" << extensions.size();

    // for (const QXmppElement &extension : extensions) {
    //     qDebug() << "Tag:" << extension.tagName();
    //     qDebug() << "Namespace:" << extension.attributeNames ();
    //     qDebug() << "Value:" << extension.value();
    // }
}

QString XmppManager::lastError() const
{
    if (QThread::currentThread() == thread()) {
        return m_lastError;
    }

    QString error;
    QMetaObject::invokeMethod(
        const_cast<XmppManager *>(this),
        [this, &error]() { error = m_lastError; },
        Qt::BlockingQueuedConnection);
    return error;
}

QString XmppManager::currentJid() const
{
    if (QThread::currentThread() == thread()) {
        return m_currentJid;
    }

    QString jid;
    QMetaObject::invokeMethod(
        const_cast<XmppManager *>(this),
        [this, &jid]() { jid = m_currentJid; },
        Qt::BlockingQueuedConnection);
    return jid;
}

void XmppManager::updateState(ConnectionState state)
{
    if (m_connectionState == state) {
        return;
    }
    m_connectionState = state;
    // qDebug() << "State Changed";
    emit connectionStateChanged ();
}

void XmppManager::setLastError(const QString &error)
{
    if (m_lastError == error) {
        return;
    }
    m_lastError = error;
    emit lastErrorChanged ();
}

QFuture<QString> XmppManager::sendMessage(const QString &receiver_id, const QString &message, const QString &message_id)
{
    auto promise = std::make_shared<QPromise<QString>>();
    promise->start();
    auto future = promise->future();
    QMetaObject::invokeMethod (this,
                              [this, receiver_id, message,message_id, promise] {
                                  if (!m_client || !m_client->isConnected ()
                                      || receiver_id.isEmpty () || message.isEmpty ()) {
                                      promise->addResult (tr("The message could not be sent. Reconnect and retry"));
                                      promise->finish ();
                                      return;
                                  }
                                  const QString body = message.trimmed ();
                                  const QString domain = m_client->configuration().jid().section('@', 1).section('/', 0, 0);
                                  const QString receiver_jid = receiver_id + "@" + domain;
                                  QXmppMessage msg;
                                  msg.setTo (receiver_jid);
                                  msg.setBody (body);
                                  msg.setType (QXmppMessage::Chat);
                                  msg.setId (message_id);
                                  m_client->send (std::move (msg))
                                      .then (this,
                                            [promise](QXmpp::SendResult result) {
                                                const auto *error = std::get_if<QXmppError> (&result);
                                                promise->addResult (error ? error->description : QString());
                                                promise->finish ();
                                            });
                              }, Qt::AutoConnection);

    return future;
}


QFuture<QList<UploadedFile>> XmppManager::fileUploads(const QString &id, const QStringList &file_paths)
{
    auto promise = std::make_shared<QPromise<QList<UploadedFile>>>();
    promise->start();
    auto future = promise->future();
    QMetaObject::invokeMethod(this, [this,id, file_paths, promise] {
        if (promise->isCanceled()) {
            promise->finish();
            return;
        }
        QList<QFuture<UploadedFile>> uploads;
        uploads.reserve(file_paths.size());
        for (const auto &path : file_paths) {
            if (promise->isCanceled()) {
                break;
            }
            uploads.append(fileUpload(id, path));
        }
        watchFileUploads(uploads, promise, this);
    }, Qt::AutoConnection);
    return future;
}

QFuture<UploadedFile> XmppManager::fileUpload(const QString &id,const QString &file_path)
{
    auto promise = std::make_shared<QPromise<UploadedFile>> ();
    promise->start ();
    auto future = promise->future ();
    QMetaObject::invokeMethod (this, [this, file_path, promise, id] {
        if (promise->isCanceled()) {
            promise->finish();
            return;
        }
        const auto reject = [promise](const QString &error) {
            promise->addResult (UploadedFile{{}, error});
            promise->finish ();
        };
        if (!m_client || !m_client->isConnected () || !m_httpUpload) {
            reject (tr("Connect to XMPP before uploading a file."));
            return;
        }
        const QUrl file = QUrl(file_path);
        const QFileInfo info(file.toLocalFile ());
        if (!file.isLocalFile () || !info.isFile () || info.size () <= 0) {
            reject(tr("File missing or empty"));
            return;
        }
        if (m_httpUpload->support () != QXmppHttpUploadManager::Support::Supported) {
            reject(m_httpUpload->support () == QXmppHttpUploadManager::Support::Unknown
                   ? tr("Upload service discovery is still running. Please retry.")
                       : tr("Enable the HTTP File upload plugin"));
            return;
        }
        auto *request = m_client->findExtension<QXmppUploadRequestManager>();
        const auto services = m_httpUpload->services ();
        if (!request || services.isEmpty ()) {
            reject(tr("File upload service is not available"));
            return;
        }
        auto *operation = new FileUploadOperation(promise, this);
        if (!operation->open(info.absoluteFilePath())) {
            return;
        }
        const auto complete = [operation](const UploadedFile &result) {
            operation->finish(result);
        };
        connect(m_client, &QXmppClient::disconnected, operation, [complete] {
            complete({{}, QObject::tr("Disconnected during file upload. Reconnect and retry.")});
        });
        /*
         * Request the XMPP slot seperately: the application's HTTP transport
         * Suppot local delvelopment server
         */
        const auto mimType = QMimeDatabase().mimeTypeForFile (info);
        // const QString file_name = QUuid::createUuid ().toString (QUuid::WithoutBraces)
        //                           + "." + info.suffix ();

        const QString file_name = info.fileName ();
        request->requestSlot (
            file_name,
            info.size (),
            mimType,
            services.first ().jid ()
            ).then (operation,
                  [this,id, operation, complete, mimType]
                  (QXmppUploadRequestManager::SlotResult result) {
                      if (operation->isStopped()) {
                          return;
                      }
                      if (const auto *error = std::get_if<QXmppError> (&result)) {
                          complete ({{}, error->description});
                          return;
                      }
                      const auto &slot = std::get<QXmppHttpUploadSlotIq>(result);
                      const auto allow_url = [](const QUrl &url) {
                          if (!url.isValid () || url.host ().isEmpty () || !url.userInfo ().isEmpty ()) {
                              return false;
                          };
                          if (url.scheme () == QStringLiteral ("https")) {
                              return true;
                          }
                          return url.scheme () == QStringLiteral ("http")
                                 && (url.host ().compare (QStringLiteral ("localhost"), Qt::CaseInsensitive) == 0
                                     || QHostAddress(url.host ()).isLoopback ()) ;
                      };
                      if (!allow_url (slot.putUrl ()) || !allow_url (slot.getUrl ()))  {
                          complete ({{}, QObject::tr ("Upload URLs must use HTTPS or HTTP on localhost.")});
                          return;
                      }
                      QNetworkRequest request(slot.putUrl ());
                      request.setHeader (QNetworkRequest::ContentTypeHeader, mimType.name ());
                      // Use the server-issued URL directly; do not forward slot credentials on redirects.
                      request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
                      const auto headers = slot.putHeaders();
                      for (auto it = headers.cbegin(); it != headers.cend(); ++it) {
                          request.setRawHeader(it.key().toUtf8(), it.value().toUtf8());
                      }
                      operation->put(request, slot.getUrl(),
                                     [this, id](qint64 bytesSent, qint64 bytesTotal) {
                          if (bytesTotal <= 0) {
                              return;
                          }
                          const double progress = double(bytesSent) / double(bytesTotal);
                          emit uploadProgressChanged(id,progress );
                      });
                  });

    }, Qt::AutoConnection);
    return future;
}

























} // namespace xmpp
} // namespace core
