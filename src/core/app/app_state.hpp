#pragma once

#include <QObject>

namespace core {
namespace app {

class AppState final : public QObject
{
    Q_OBJECT
public:
    static AppState &instance();

    [[nodiscard]] bool isInCall() const noexcept;
    [[nodiscard]] int notificationCount() const noexcept;

    void setInCall(bool value);
    void setNotificationCount(int count);
    void appendNotificationCount(int value);
    void resetNotifcationCount();
signals:
    void inCallChanged(bool in_call);
    void notificationCountChanged(int count);

private:
    explicit AppState(QObject *parent = nullptr);

    bool m_isInCall = false;

    std::atomic<int> m_notificationCount{0};

    Q_DISABLE_COPY_MOVE (AppState);

};

} // namespace app
} // namespace core
