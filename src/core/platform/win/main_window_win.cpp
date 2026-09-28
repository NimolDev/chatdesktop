#include "main_window_win.hpp"

#include <QDebug>
#include <QCoreApplication>
#include <QEvent>
#include <QPlatformSurfaceEvent>
#include <QWindow>
#include <qt_windows.h>
#include <dwmapi.h>

#include <uxtheme.h>

namespace core {
namespace platform {
namespace win {

MainWindowWin::MainWindowWin() = default;
MainWindowWin::~MainWindowWin()
{
    if (QCoreApplication::instance()) {
        QCoreApplication::instance()->removeNativeEventFilter(this);
    }
}

void MainWindowWin::setup(QWindow *window)
{
    if (m_window) {
        m_window->removeEventFilter(this);
        QObject::disconnect(m_window, nullptr, this, nullptr);
    }
    m_window = window;
    m_hwnd = nullptr;
    if (!window) {
        return;
    }
    QCoreApplication::instance()->installNativeEventFilter(this);
    window->installEventFilter(this);
    window->create();
    applyCaptionAppearance();
    connect(window, &QWindow::visibleChanged, this, [this](bool visible) {
        if (visible) {
            applyCaptionAppearance();
        }
    });
    connect(window, &QWindow::flagsChanged, this,
            &MainWindowWin::applyCaptionAppearance, Qt::QueuedConnection);
}

void MainWindowWin::applyCaptionAppearance()
{
    if (!m_window || !m_window->handle()) {
        return;
    }
    // Expanded windows use Qt's own caption painter, which ignores WTA_OPTIONS.
    // Customize that painter without dropping the existing expansion/pinning hints.
    auto flags = m_window->flags();
    if (flags.testFlag(Qt::ExpandedClientAreaHint)) {
        if (!flags.testFlag(Qt::CustomizeWindowHint)) {
            // Make Qt's implicit default buttons explicit before customizing.
            flags |= Qt::WindowMinimizeButtonHint | Qt::WindowMaximizeButtonHint
                     | Qt::WindowCloseButtonHint | Qt::WindowSystemMenuHint;
        }
        flags |= Qt::CustomizeWindowHint;
        flags &= ~Qt::WindowTitleHint;
        if (flags != m_window->flags()) {
            m_window->setFlags(flags);
        }
    }
    m_hwnd = reinterpret_cast<HWND>(m_window->winId());
    // Change only non-client appearance, preserving window styles and icons.
    WTA_OPTIONS options{};
    options.dwFlags = WTNCA_NODRAWCAPTION | WTNCA_NODRAWICON | WTNCA_NOSYSMENU;
    options.dwMask = options.dwFlags;
    const HRESULT result = SetWindowThemeAttribute(
        m_hwnd, WTA_NONCLIENT, &options, sizeof(options));
    if (FAILED(result)) {
        qWarning() << "Unable to hide the native caption and icon:" << result;
    }
    RedrawWindow(m_hwnd, nullptr, nullptr, RDW_FRAME | RDW_INVALIDATE);
}

bool MainWindowWin::eventFilter(QObject *object, QEvent *event)
{
    if (object == m_window && event->type() == QEvent::PlatformSurface) {
        const auto *surfaceEvent = static_cast<QPlatformSurfaceEvent *>(event);
        if (surfaceEvent->surfaceEventType() == QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed) {
            m_hwnd = nullptr;
        } else {
            QMetaObject::invokeMethod(this, &MainWindowWin::applyCaptionAppearance,
                                      Qt::QueuedConnection);
        }
    }
    return QObject::eventFilter(object, event);
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
    // Preserve native controls and pinning for both native and Qt-painted captions.
    applyCaptionAppearance();
}

void MainWindowWin::pineWindow(bool pinned)
{
    if (!m_window) {
        return;
    }
    // Keep Qt's state in sync so subsequent flag updates preserve pinning.
    m_window->setFlag(Qt::WindowStaysOnTopHint, pinned);
}

bool MainWindowWin::nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result)
{
    if (eventType != "windows_generic_MSG" || !m_window || !m_hwnd) {
        return false;
    }

    MSG *msg = static_cast<MSG *>(message);
    if (msg->hwnd != m_hwnd) {
        return false;
    }

    switch (msg->message) {
    case WM_NCLBUTTONDOWN:
    case WM_NCLBUTTONUP:
    case WM_NCLBUTTONDBLCLK:
        // The hidden icon must not open a menu or close the window on double-click.
        if (msg->wParam == HTSYSMENU) {
            *result = 0;
            return true;
        }
        break;
    case WM_NCRBUTTONUP:
    case WM_NCRBUTTONDOWN:
    case WM_NCRBUTTONDBLCLK:
        // Right click on the title bar
        if (msg->wParam == HTCAPTION || msg->wParam == HTSYSMENU) {
            *result = 0;
            return true;
        }
        break;

    case WM_SYSCOMMAND:
        // Block mouse invocation, retaining keyboard/native window commands.
        if ((msg->wParam & 0xFFF0) == SC_MOUSEMENU) {
            *result = 0;
            return true;
        }
        break;
    }
    return false;
}

} // namespace win
} // namespace platform
} // namespace core
