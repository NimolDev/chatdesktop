#include "xmpp_service_discovery.hpp"

namespace core {
namespace xmpp {

XmppServiceDiscovery::XmppServiceDiscovery(QXmppClient *client, QObject *parent)
    : QObject(parent)
{
    Q_ASSERT (client);
    m_manager = client->addNewExtension<QXmppExternalServiceDiscoveryManager> ();

}

void XmppServiceDiscovery::requestExtDiscoQuery(const QString &server)
{
    Q_ASSERT (m_manager);
    m_manager ->requestServices (server)
    .then (
            this,
            [this](
                QXmppExternalServiceDiscoveryManager::ServicesResult result
                ) {
                if (const auto *error = std::get_if<QXmppError> (&result)) {
                    qWarning() << "External serice discovery failed:" << error->description;
                    emit externalServicesFailed (error->description);
                    return;
                }
                const auto &services = std::get<QVector<QXmppExternalService>>(result);

                for (const QXmppExternalService &service : services) {
                    qDebug() << "Type:" << service.type();
                    qDebug() << "Host:" << service.host();
                    qDebug() << "Port:" << service.port();
                    qDebug() << "Transport :" << static_cast<int>(service.transport ().value ());
                    qDebug() << "Username:" << service.username();
                    qDebug() << "Password:" << service.password();
                }
                emit externalServiceReceived (services);
            });
}

} // namespace xmpp
} // namespace core


