#pragma once

#include <QObject>
#include <QTimer>
#include <QNetworkInformation>


namespace core {
namespace network {

class NetworkMonitor : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isOnline READ isOnline NOTIFY networkChanged)
    Q_PROPERTY(bool isRetry READ isRetry NOTIFY isRetryChanged)
    Q_PROPERTY(int retryCountDown READ retryCountDown NOTIFY retryCountDownChanged)

public:
    static NetworkMonitor &instance();
    NetworkMonitor(const NetworkMonitor&) = delete;
    NetworkMonitor &operator=(const NetworkMonitor) = delete;

    void setMaximumRetry(int maximum_retry);

public slots:

    bool isOnline() const;

    /**
     * @brief call to force try
     */
    void retry();


    /**
     * @brief setInializeDelay
     * @param initial_delay: second
     */
    void setInializeDelay(int initial_delay);

    int retryCountDown();
    bool isRetry();

  void startRetryCountDown();
    void stopRetryCountDown();


signals:
    void networkChanged(bool online);
    void networkConnected();
    void networkDisconnected();
    void retryNowInteract();
    void retryCountDownChanged(int value);
    void retryTimerChanged();
    void isRetryChanged();

private:
    explicit NetworkMonitor(QObject *parent = nullptr);
    void updateStatus(QNetworkInformation::Reachability reachbility);


    QTimer m_retryCount;

private:
    bool m_isOnline = true;
    bool m_isRetry = false;
    int m_initial_delay = 10; // second
    int m_retry_count_down = 0;
    int m_maximum_retry;
    int m_current_delay = m_initial_delay;



};


} // namespace network
} // namespace core
