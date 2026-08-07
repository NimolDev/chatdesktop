#ifndef CORE_XMPP_XMPP_SERVICE_DISCOVERY_HPP
#define CORE_XMPP_XMPP_SERVICE_DISCOVERY_HPP

#include <QObject>
#include <QXmppClient.h>
#include <QXmppExternalServiceDiscoveryManager.h>
#include <QXmppExternalService.h>
#include <QXmppError.h>

namespace core {
namespace xmpp {


class XmppServiceDiscovery final : public QObject
{
    Q_OBJECT
public:
    explicit XmppServiceDiscovery(
        QXmppClient *client,
        QObject *parent = nullptr
        );

    void requestExtDiscoQuery(const QString &server);
signals:
    // service discovery
    void externalServiceReceived(const QVector<QXmppExternalService> &sevices);
    void externalServicesFailed(const QString &reason);

private:

    QXmppExternalServiceDiscoveryManager *m_manager = nullptr;

};

} // namespace xmpp
} // namespace core

#endif // CORE_XMPP_XMPP_SERVICE_DISCOVERY_HPP
