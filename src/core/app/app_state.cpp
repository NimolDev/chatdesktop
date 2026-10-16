#include "app_state.hpp"


core::app::AppState &core::app::AppState::instance()
{
    static AppState instance;
    return instance;
}
core::app::AppState::AppState(QObject *parent)
    : QObject(parent)
{}

bool core::app::AppState::isInCall() const noexcept
{
    return m_isInCall;
}

int core::app::AppState::notificationCount() const noexcept
{
    return m_notificationCount.load (std::memory_order_relaxed);
}

void core::app::AppState::setInCall(bool value)
{
    if (m_isInCall == value) {
        return;
    }
    m_isInCall = value;
    emit inCallChanged (value);
}

void core::app::AppState::setNotificationCount(int count)
{
    if (m_notificationCount == count) {
        return;
    }
    m_notificationCount = count;
    emit notificationCountChanged (count);
}

void core::app::AppState::appendNotificationCount(int value)
{
    const int old_value = m_notificationCount.fetch_add (value, std::memory_order_relaxed);
    const int new_value =  old_value + value;
    emit notificationCountChanged (new_value);
}

void core::app::AppState::resetNotifcationCount()
{
    m_notificationCount.store (0, std::memory_order_relaxed);
    emit notificationCountChanged (0);
}


