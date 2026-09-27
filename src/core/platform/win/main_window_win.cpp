#include "main_window_win.hpp"

#include <QDebug>
#include <QWindow>
#include <qt_windows.h>
#include <dwmapi.h>

namespace core {
namespace platform {
namespace win {

MainWindowWin::MainWindowWin() = default;
MainWindowWin::~MainWindowWin() = default;

void MainWindowWin::setup(QWindow *window)
{
    m_window = window;
    if (!window) {
        return;
    }
    window->create();
}

void MainWindowWin::setTitleBarColor(QWindow *window, const QColor &color)
{
    if (!window || !color.isValid()) {
        return;
    }

    const HWND handle = reinterpret_cast<HWND>(window->winId());
    const COLORREF captionColor = RGB(color.red(), color.green(), color.blue());
    // Native caption colors are supported starting with Windows 11.
    const HRESULT result = DwmSetWindowAttribute(handle, DWMWA_CAPTION_COLOR,
                                                 &captionColor, sizeof(captionColor));
    if (FAILED(result)) {
        qWarning() << "Unable to set native title-bar color:" << result;
    }
}

void MainWindowWin::setWindowFillContent()
{
    if (!m_window) {
        return;
    }
    // Extend content into the title bar while retaining native window controls.
    m_window->setFlags(m_window->flags() | Qt::ExpandedClientAreaHint
                       | Qt::NoTitleBarBackgroundHint);
}

void MainWindowWin::pineWindow(bool pinned)
{
    if (!m_window) {
        return;
    }
    // Keep Qt's state in sync so subsequent flag updates preserve pinning.
    m_window->setFlag(Qt::WindowStaysOnTopHint, pinned);
}

} // namespace win
} // namespace platform
} // namespace core
