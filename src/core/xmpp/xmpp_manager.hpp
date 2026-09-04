#ifndef CORE_XMPP_XMPP_MANAGER_HPP
#define CORE_XMPP_XMPP_MANAGER_HPP


#include "xmpp_service_discovery.hpp"

#include <QObject>
#include <QString>
#include <QXmppPresence.h>
#include <QXmppClient.h>
#include <QXmppMessage.h>
#include <QXmppRosterManager.h>

#include <optional>

namespace core {
namespace xmpp {

struct Message {
    QString from;
    QString to;
    QXmppMessage::Type type;
    QString body;

    static Message map(const QXmppMessage &message) {
        return {
            message.from (),
            message.to (),
            message.type (),
            message.body ()
        };
    }
};

struct Presence
{

    QString from;
    QString to;
    QXmppPresence::Type type;
    QString status;


    static core::xmpp::Presence map(const QXmppPresence &presence) {

        return {
            .from = presence.from (),
            .to = presence.to(),
            .type = presence.type (),
            .status = presence.statusText ()
        };
    }
};


class XmppManager final : public QObject
{
  Q_OBJECT
public:
    enum class ConnectionState {
        Disconnected,
        Connecting,
        Connected
    };
    Q_ENUM (ConnectionState);
    explicit XmppManager(QObject *parent = nullptr);
    [[nodiscard]]
    ConnectionState connectionState() const noexcept;

    [[nodiscard]]
    bool isConnected() const noexcept;

    [[nodiscard]]
    QString lastError() const;

    QString currentJid() const;

    // void connectToServer(const QString &jid, const QString &password);


public slots:

    void initialize();
    void connectToServer(
        const QString &jid,
        const QString &password,
        const QString &host,
        quint16 port
        );
    void closeConnection();
    void sendMessage(
        const QString &receiver_id,
        const QString &message);

signals:
    void connectionStateChanged();
    void connectionFailed();
    void connectedChanged();
    void lastErrorChanged();

    void messageReceived(const core::xmpp::Message &message);
    void presenceReceived(const core::xmpp::Presence &presence);

private slots:
    void onMessageReceived(const QXmppMessage &message);
    void onPresenceReceived(const QXmppPresence &presence);
    void onIQReceived(const QXmppIq &iq);

private:
    struct ConnectionParameters {
        QString jid;
        QString password;
        QString host;
        quint16 port = 0;
    };

    void initializeHandlers();
    void initializeSignals();
    void startConnection(const ConnectionParameters &parameters);
    void updateState(ConnectionState state);
    void setLastError(const QString &error);

private:
    QXmppClient *m_client = nullptr;
    QXmppRosterManager *m_roster = nullptr;

    ConnectionState m_connectionState = ConnectionState::Disconnected;
    QString m_lastError;
    QString m_currentJid;
    std::optional<ConnectionParameters> m_pendingConnection;
    bool m_connectionStartScheduled = false;

    std::unique_ptr<core::xmpp::XmppServiceDiscovery> m_discovery;

};

} // namespace xmpp
} // namespace core

Q_DECLARE_METATYPE(core::xmpp::Message)
Q_DECLARE_METATYPE(core::xmpp::Presence)


#endif // CORE_XMPP_XMPP_MANAGER_HPP
