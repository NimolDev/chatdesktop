#pragma once

#include <QObject>
#include <QNetworkInformation>


namespace core {
namespace network {

class NetworkMonitor : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isOnline READ isOnline NOTIFY networkChanged )

public:
    static NetworkMonitor &instance();
    NetworkMonitor(const NetworkMonitor&) = delete;
    NetworkMonitor &operator=(const NetworkMonitor) = delete;

    bool isOnline() const;




signals:
    void networkChanged(bool online);
    void networkConnected();
    void networkDisconnected();

private:
    explicit NetworkMonitor(QObject *parent = nullptr);
    void updateStatus(QNetworkInformation::Reachability reachbility);

private:
    bool m_isOnline = true;
};


} // namespace network
} // namespace core
