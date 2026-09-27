#include "xmpp_manager.hpp"


#include <QTimer>
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
    };

    // Return to QML immediately so pending UI changes can be rendered before
    // QXmpp performs its connection setup. QXmppClient is event-driven and
    // must remain on this QObject's thread, so QtConcurrent is not appropriate.
    m_pendingConnection = parameters;
    if (m_connectionStartScheduled) {
        return;
    }

    m_connectionStartScheduled = true;
    QTimer::singleShot(0, this, [this]() {
        m_connectionStartScheduled = false;
        if (!m_pendingConnection.has_value()) {
            return;
        }

        if (m_client->state() != QXmppClient::DisconnectedState) {
            m_client->disconnectFromServer();
            return;
        }

        const ConnectionParameters parameters =
            std::move(m_pendingConnection.value());
        m_pendingConnection.reset();
        startConnection(parameters);
    });
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

void XmppManager::sendMessage(const QString &receiver_id, const QString &message)
{
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(
            this,
            [this, receiver_id, message]() { sendMessage(receiver_id, message); },
            Qt::QueuedConnection);
        return;
    }

    Q_ASSERT(m_client);
    if (m_client->state () != QXmppClient::ConnectedState) {
        qWarning() << "XMPP Client is not connected";
        return;
    }
    const QString body = message.trimmed ();

    if (receiver_id.isEmpty () || body.isEmpty ()) {
        return;
    }
    const QString receiver_jid = receiver_id + "@localhost";
    QXmppMessage msg;
    msg.setTo (receiver_jid);
    msg.setBody (body);
    msg.setType (QXmppMessage::Chat);
    msg.setId (QUuid::createUuid ().toString (QUuid::WithoutBraces));
    // QXmppStanza stanza;

    m_client->send (std::move (msg));

}

void XmppManager::initializeHandlers()
{
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



} // namespace xmpp
} // namespace core
