#pragma once

#include "../main_window.hpp"
#include <QPointer>
#include <QAbstractNativeEventFilter>
#include <QObject>
#include <qt_windows.h>
namespace core {
namespace platform {
namespace win {

class MainWindowWin final : public QObject, public MainWindow, public QAbstractNativeEventFilter
{
public:
    explicit MainWindowWin();
    ~MainWindowWin() override;

    void setup(QWindow *window) override;
    void setTitleBarColor(QWindow *window, const QColor &color) override;
    void setWindowFillContent() override;
    void pineWindow(bool pinned) override;

    bool nativeEventFilter(const QByteArray &eventType,
                           void *message, qintptr *result) override;
private:
    bool eventFilter(QObject *object, QEvent *event) override;
    void applyCaptionAppearance();
    QPointer<QWindow> m_window;
    HWND m_hwnd = nullptr;
};

} // namespace win
} // namespace platform
} // namespace core
