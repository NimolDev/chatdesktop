#include "network_monitor.hpp"
#include <QtCore/qdebug.h>



namespace core {
namespace network {

NetworkMonitor &NetworkMonitor::instance()
{
    static NetworkMonitor instance;
    return instance;
}

bool NetworkMonitor::isOnline() const
{
    return m_isOnline;
}

NetworkMonitor::NetworkMonitor(QObject *parent)
    : QObject(parent)
{
    QNetworkInformation::loadBackendByFeatures (QNetworkInformation::Feature::Reachability);

    auto *network = QNetworkInformation::instance ();
    if (!network) {
        return;
    }
    updateStatus (network->reachability ());
    connect(network,
            &QNetworkInformation::reachabilityChanged,
            this,
            &NetworkMonitor::updateStatus);
}

void NetworkMonitor::updateStatus(QNetworkInformation::Reachability reachbility)
{

    switch(reachbility) {
    case QNetworkInformation::Reachability::Unknown:
        qDebug() << "Network: Unknown";
        emit networkChanged(false);
        emit networkDisconnected ();
        break;
    case QNetworkInformation::Reachability::Disconnected:
        qDebug() << "Network: Disconnected";
         emit networkChanged(false);
        emit networkDisconnected ();
        break;
    case QNetworkInformation::Reachability::Local:
        qDebug() << "Network: Local";
        break;
    case QNetworkInformation::Reachability::Site:
        qDebug() << "Network: Site";
        break;
    case QNetworkInformation::Reachability::Online:
        qDebug() << "Network: Online";
        emit networkChanged(true);
        emit networkConnected ();
        break;
    }

    const bool online = reachbility == QNetworkInformation::Reachability::Online;
    if (m_isOnline == online) {
        return;
    }
}



} // namespace network
} // namespace core
