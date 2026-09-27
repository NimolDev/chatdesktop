#pragma once

#include "../main_window.hpp"
#include <QPointer>

namespace core {
namespace platform {
namespace win {

class MainWindowWin final : public MainWindow
{
public:
    explicit MainWindowWin();
    ~MainWindowWin() override;

    void setup(QWindow *window) override;
    void setTitleBarColor(QWindow *window, const QColor &color) override;
    void setWindowFillContent() override;
    void pineWindow(bool pinned) override;

private:
    QPointer<QWindow> m_window;
};

} // namespace win
} // namespace platform
} // namespace core
