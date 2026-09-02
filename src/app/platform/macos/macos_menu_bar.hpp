#pragma once

class QApplication;
class QWindow;

namespace platform::macos {

class MacosMenuBar
{
public:
    MacosMenuBar(QApplication *app, QWindow *window);
    ~MacosMenuBar();

    MacosMenuBar(const MacosMenuBar &) = delete;
    MacosMenuBar &operator=(const MacosMenuBar &) = delete;

private:
    class Private;
    Private *d;
};

} // namespace platform::macos
