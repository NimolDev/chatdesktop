#include "win_tray.hpp"

#include <QAction>
#include <QDebug>
#include <QIcon>
#include <QMenu>
#include <QPixmap>
#include <QPointer>
#include <QSystemTrayIcon>
#include <utility>

namespace core {
namespace platform {
namespace win {

namespace {
QPointer<QSystemTrayIcon> notificationTray;
} // namespace

class TrayWin::Impl
{
public:
    QPointer<QWindow> window;
    QPointer<QApplication> application;
    std::unique_ptr<QSystemTrayIcon> tray;
    std::unique_ptr<QMenu> menu;
    QAction *openAction = nullptr;
    QAction *notificationAction = nullptr;
    bool notificationsEnabled = true;
    QString tooltip;
    std::function<void()> openCallback;
    std::function<void()> quitCallback;
    std::function<void()> clickCallback;
};

TrayWin::TrayWin() : d(std::make_unique<Impl>()) {}

TrayWin::~TrayWin()
{
    detroyIcon();
    destroyMenu();
}

void TrayWin::setup(QWindow *window, QApplication *application)
{
    d->window = window;
    d->application = application;
    createIcon();
    createMenu();
}

void TrayWin::createIcon()
{
    if (!d->tray) {
        d->tray = std::make_unique<QSystemTrayIcon>();
        notificationTray = d->tray.get();
        d->tray->setProperty("notificationsEnabled", d->notificationsEnabled);
        QObject::connect(d->tray.get(), &QSystemTrayIcon::messageClicked, d->tray.get(),
                         [this] { showFromTrayRequest(); });
        QObject::connect(d->tray.get(), &QSystemTrayIcon::activated, d->tray.get(),
                         [this](QSystemTrayIcon::ActivationReason reason) {
            if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
                iconClicked();
            }
        });
        d->tray->setToolTip(d->tooltip);
        d->tray->setContextMenu(d->menu.get());
    }
    updateIcon();
}

void TrayWin::updateIcon()
{
    if (!d->tray) {
        return;
    }
    QPixmap pixmap;
    pixmap.loadFromData(getTrayIconData());
    d->tray->setIcon(pixmap.isNull() ? QApplication::windowIcon() : QIcon(pixmap));
}

bool TrayWin::hasIcon()
{
    return d->tray && !d->tray->icon().isNull();
}

void TrayWin::detroyIcon()
{
    if (d->tray) {
        d->tray->hide();
        d->tray.reset();
    }
}

void TrayWin::createMenu()
{
    if (d->menu) {
        return;
    }
    d->menu = std::make_unique<QMenu>();
    QObject::connect(d->menu.get(), &QMenu::aboutToShow, d->menu.get(),
                     [this] { aboutToShowRequests(); });
    addAction();
    if (d->tray) {
        d->tray->setContextMenu(d->menu.get());
    }
}

void TrayWin::destroyMenu()
{
    if (d->tray) {
        d->tray->setContextMenu(nullptr);
    }
    d->menu.reset();
    d->openAction = nullptr;
    d->notificationAction = nullptr;
}

void TrayWin::addAction()
{
    if (!d->menu || d->openAction) {
        return;
    }
    d->openAction = d->menu->addAction(QObject::tr("Open"));
    QObject::connect(d->openAction, &QAction::triggered, d->menu.get(), [this] {
        if (d->window && d->window->isVisible() && !d->window->windowStates().testFlag(Qt::WindowMinimized)) {
            hideToTrayRequest();
        } else {
            showFromTrayRequest();
        }
    });
    d->notificationAction = d->menu->addAction(QObject::tr("Enable notifications"));
    d->notificationAction->setCheckable(true);
    d->notificationAction->setChecked(d->notificationsEnabled);
    QObject::connect(d->notificationAction, &QAction::triggered, d->menu.get(),
                     [this](bool checked) { setEnableNotification(checked); });
    addSeperate();
    QAction *quit = d->menu->addAction(QObject::tr("Quit"));
    QObject::connect(quit, &QAction::triggered, d->menu.get(), [this] {
        if (d->quitCallback) {
            const auto callback = d->quitCallback;
            callback();
        } else if (d->application) {
            d->application->quit();
        }
    });
}

void TrayWin::addSeperate()
{
    if (d->menu) {
        d->menu->addSeparator();
    }
}

void TrayWin::aboutToShowRequests()
{
    if (d->openAction) {
        const bool visible = d->window && d->window->isVisible() && !d->window->windowStates().testFlag(Qt::WindowMinimized);
        d->openAction->setText(visible ? QObject::tr("Minimize to tray") : QObject::tr("Open"));
        d->openAction->setEnabled(d->window || bool(d->openCallback));
    }
}

void TrayWin::showFromTrayRequest()
{
    if (d->window) {
        d->window->setWindowStates(d->window->windowStates() & ~Qt::WindowMinimized);
        d->window->show();
        d->window->raise();
        d->window->requestActivate();
    }
    if (d->openCallback) {
        const auto callback = d->openCallback;
        callback();
    }
}

void TrayWin::hideToTrayRequest()
{
    if (d->window) {
        d->window->hide();
    }
}

void TrayWin::iconClicked()
{
    if (d->clickCallback) {
        const auto callback = d->clickCallback;
        callback();
    } else {
        showFromTrayRequest();
    }
}

void TrayWin::setEnableNotification(bool enable)
{
    d->notificationsEnabled = enable;
    if (d->tray) {
        d->tray->setProperty("notificationsEnabled", enable);
    }
    if (d->notificationAction) {
        d->notificationAction->setChecked(enable);
    }
}

void TrayWin::showTrayIcon()
{
    createIcon();
    createMenu();
    if (hasIcon()) {
        d->tray->show();
    }
}

void TrayWin::setTooltip(const QString &tooltip)
{
    d->tooltip = tooltip;
    if (d->tray) {
        d->tray->setToolTip(tooltip);
    }
}

void TrayWin::setOpenCallback(std::function<void()> callback)
{
    d->openCallback = std::move(callback);
}

void TrayWin::setQuitCallback(std::function<void()> callback)
{
    d->quitCallback = std::move(callback);
}

void TrayWin::setTrayClickCallback(std::function<void()> callback)
{
    d->clickCallback = std::move(callback);
}

void TrayWin::showNotification(const QString &title, const QString &message)
{
    if (!notificationTray || !notificationTray->isVisible()) {
        qWarning() << "Cannot show notification without a visible system tray icon";
        return;
    }
    if (!notificationTray->property("notificationsEnabled").toBool()) {
        return;
    }
    notificationTray->showMessage(title, message, QSystemTrayIcon::Information);
}

} // namespace win
} // namespace platform
} // namespace core
