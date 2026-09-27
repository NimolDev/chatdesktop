#pragma once

#include <functional>

#include <QFile>
#include <QByteArray>
#include <QWindow>
#include <QApplication>

namespace core {
namespace platform {


class TrayIcon
{
public:
    virtual ~TrayIcon() = default;

    virtual void aboutToShowRequests() = 0;
    virtual void showFromTrayRequest() = 0;
    virtual void hideToTrayRequest() = 0;
    virtual void iconClicked() = 0;
    virtual void setEnableNotification(bool enable) = 0;


    virtual void setup(QWindow *window, QApplication *application) = 0;
    [[nodiscard]] virtual bool hasIcon()  = 0;
    virtual void createIcon() = 0;
    virtual void detroyIcon() = 0;
    virtual void updateIcon() = 0;

    virtual void createMenu() = 0;
    virtual void destroyMenu() = 0;

    virtual void addAction() = 0;
    virtual void addSeperate() = 0 ;

    virtual void showTrayIcon()  = 0;
    virtual void setTooltip(const QString &tooltip) = 0;

    virtual void setOpenCallback(std::function<void()> callback) = 0;
    virtual void setQuitCallback(std::function<void()> callback) = 0;
    virtual void setTrayClickCallback(std::function<void()> callback) = 0;


protected:
    QByteArray getTrayIconData()  {
        QFile file(QStringLiteral (":/images/tray_icon.png"));
        if (file.open (QIODevice::ReadOnly)) {
            const QByteArray bytes = file.readAll ();
            return bytes;
        }
        return {};
    }

};

} // namespace platform
} // namespace core
