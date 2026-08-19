#include "xmpp_manager.hpp"
#include "QXmppRosterManager.h"

#include <QTimer>
#include <QUuid>

namespace core {
namespace xmpp {

XmppManager::XmppManager(QObject *parent)
    : QObject(parent)
{
    qDebug() << "instantiate";
    initializeHandlers ();
    initializeSignals ();
}

XmppManager::ConnectionState XmppManager::connectionState() const noexcept
{
    return m_connectionState;
}
bool XmppManager::isConnected() const noexcept
{
    return m_client.isConnected ();
}

void XmppManager::connectToServer(
    const QString &jid,
    const QString &password,
    const QString &host,
    quint16 port)
{
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
        .port = port
    };

    if (m_client.state() != QXmppClient::DisconnectedState) {
        m_pendingConnection = parameters;
        m_client.disconnectFromServer();
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

    m_client.connectToServer(configuration, presence);
}

void XmppManager::closeConnection()
{
    m_pendingConnection.reset();

    if (m_client.state() == QXmppClient::DisconnectedState) {
        updateState(ConnectionState::Disconnected);
        return;
    }

    QXmppPresence presence;
    presence.setType(QXmppPresence::Unavailable);
    presence.setStatusText(QStringLiteral("Offline"));
    m_client.setClientPresence(presence);
    m_client.disconnectFromServer();
}


void XmppManager::initializeHandlers()
{
    m_roster = m_client.findExtension<QXmppRosterManager>();
    if (!m_roster) {
        qWarning() << "QXmppRosterManager not available";
    }

    m_discovery = std::make_unique<core::xmpp::XmppServiceDiscovery> (&m_client, this);
}

void XmppManager::initializeSignals()
{
    connect(
        &m_client,
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
                QXmppPresence presence;
                presence.setType(QXmppPresence::Available);
                presence.setStatusText(QStringLiteral("Online"));
                m_client.clientPresence ();
                break;
            }
            case QXmppClient::ConnectedState:
                updateState (ConnectionState::Connected);
                // m_discovery->requestExtDiscoQuery (QStringLiteral("xabber.org"));
                break;
            }
        }
        );

    connect (
        &m_client,
        &QXmppClient::connected,
        this,
        [this]() {
            setLastError ({});
            m_currentJid = m_client.configuration ().jid ();
            qInfo() << "XMPP connected: " << m_client.configuration ().jid ();
            QXmppPresence presence(QXmppPresence::Available);
            m_client.setClientPresence (presence);
            m_client.setClientPresence (presence);
            emit connectedChanged ();
        }
        );

    connect(
        &m_client,
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
                if (m_client.state() == QXmppClient::DisconnectedState) {
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
        &m_client,
        &QXmppClient::messageReceived,
        this,
        &XmppManager::onMessageReceived
        );
    connect(
        &m_client,
        &QXmppClient::presenceReceived,
        this,
        &XmppManager::onPresenceReceived
        );
    connect (
        &m_client,
        &QXmppClient::iqReceived,
        this,
        &XmppManager::onIQReceived
        );





    // auto *logger = m_client.logger();

    // logger->setLoggingType(QXmppLogger::StdoutLogging);
    // logger->setMessageTypes(QXmppLogger::AnyMessage);
}



void XmppManager::onMessageReceived(const QXmppMessage &message)
{
    const Message msg = Message::map (message);
    emit messageReceived (msg);
}

void XmppManager::onPresenceReceived(const QXmppPresence &presence)
{
    // QString my_bare_jid = m_currentJid.section ("/",0,0);
    // QString from_bare_jid = presence.from ().section ("/", 0,0);
    // if (my_bare_jid == from_bare_jid) {
    //     return;
    // }
    // const Presence pre = Presence::map (presence);
    qDebug() << "Presence:" << presence.from ();
    // emit presenceReceived (pre);
}

void XmppManager::onIQReceived(const QXmppIq &iq)
{
    qDebug() << "---- XmppIQ Received -----";
    qDebug() << "From: " << iq.from ();
    qDebug() << "To: " << iq.to ();
    qDebug() << "Type: " << iq.type ();
    const QXmppElementList extensions = iq.extensions();

    qDebug() << "Extension count:" << extensions.size();

    for (const QXmppElement &extension : extensions) {
        qDebug() << "Tag:" << extension.tagName();
        qDebug() << "Namespace:" << extension.attributeNames ();
        qDebug() << "Value:" << extension.value();
    }
}



QString XmppManager::lastError() const
{
    return m_lastError;
}

QString XmppManager::currentJid() const
{
    return m_currentJid;
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
