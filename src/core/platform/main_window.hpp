#pragma once

#include <QColor>

class QWindow;
namespace core  {
namespace platform  {

class MainWindow
{
public:
    virtual ~MainWindow() = default;
    virtual void setup(QWindow *window) = 0 ;
    virtual void setTitleBarColor(QWindow *window, const QColor &color) = 0 ;
    virtual void setWindowFillContent() = 0;
    virtual void pineWindow(bool pinned) = 0;
};

} // namespace platfrom
} // namespace core