#pragma once

class QApplication;
class QWindow;

class MacosTrayIcon
{
public:
    class Private;

    MacosTrayIcon(QApplication *app, QWindow *window);
    ~MacosTrayIcon();

    MacosTrayIcon(const MacosTrayIcon &) = delete;
    MacosTrayIcon &operator=(const MacosTrayIcon &) = delete;

    void setBadgeNumber( int count);

private:
    // Private *d;
    Private *d;
};
