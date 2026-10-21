#include "network_monitor.hpp"
#include <QtCore/qdebug.h>



namespace core {
namespace network {

NetworkMonitor &NetworkMonitor::instance()
{
    static NetworkMonitor instance;
    return instance;
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

    m_retryCount.setInterval (1000);
    connect(&m_retryCount,
            &QTimer::timeout,
            this,
            [this]() {
                if (m_retry_count_down > 0) {
                    --m_retry_count_down;
                    emit retryCountDownChanged (m_retry_count_down);
                }
                if (m_retry_count_down == m_current_delay - 1) {
                    m_isRetry = true;
                    emit isRetryChanged ();
                }
                if (m_retry_count_down == 0 && m_maximum_retry > 0) {
                    --m_maximum_retry;
                    m_current_delay = m_current_delay * 2;
                    m_retry_count_down = qMax (0, m_current_delay);
                    emit retryCountDownChanged (m_retry_count_down);
                    emit retryTimerChanged ();
                }
                if (m_retry_count_down == 0 && m_maximum_retry ==0) {
                    m_retryCount.stop ();
                }
            });

}

bool NetworkMonitor::isOnline() const
{
    return m_isOnline;
}

void NetworkMonitor::retry()
{
    m_retry_count_down = 10;
    m_isRetry = false;
    emit isRetryChanged ();
    m_retry_count_down = qMax (0, m_initial_delay);
    m_current_delay = m_initial_delay;
    m_retryCount.stop();
    m_retryCount.start ();
    emit retryNowInteract();
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
        // startRetryCountDown ();
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
        stopRetryCountDown ();
        break;
    }

    const bool online = reachbility == QNetworkInformation::Reachability::Online;
    if (m_isOnline == online) {
        return;
    }
}

void NetworkMonitor::startRetryCountDown()
{
    m_retryCount.stop();
    m_retry_count_down = qMax (0, m_current_delay);
    emit retryCountDownChanged (m_retry_count_down);
    if (m_retry_count_down == 0 && m_maximum_retry == 0) {
        return;
    }
    m_retryCount.start ();
}

void NetworkMonitor::setMaximumRetry(int maximum_retry)
{
    m_maximum_retry = maximum_retry;
}

void NetworkMonitor::setInializeDelay(int initial_delay)
{
    m_initial_delay = initial_delay;
    m_current_delay = initial_delay;
}

int NetworkMonitor::retryCountDown()
{
    return m_retry_count_down;
}

bool NetworkMonitor::isRetry()
{
    return m_isRetry;
}

void NetworkMonitor::stopRetryCountDown()
{
    m_retryCount.stop ();
    // m_retry_count_down = m_initial_delay;
    m_current_delay = m_initial_delay;

}



} // namespace network
} // namespace core
