#pragma once

#include "../main_window.hpp"
#include <QPointer>

#ifdef __OBJC__
@class NSWindow;
#else
class NSWindow;
#endif


namespace core  {
namespace platform {
namespace macos {


class MainWindowMac final : public core::platform::MainWindow
{
public:
    explicit MainWindowMac();
    ~MainWindowMac();


public:
    void setup(QWindow *window) override;
    void setTitleBarColor(QWindow *window, const QColor &color) override;
    void setWindowFillContent() override;
    void pineWindow(bool pinned) override;

private:

    [[nodiscard]]
    NSWindow *getCurrentWindow(QWindow *window) ;

    QPointer<QWindow> m_window;


};


} // namespace macos
} // namespace plaform
} // namespace core
