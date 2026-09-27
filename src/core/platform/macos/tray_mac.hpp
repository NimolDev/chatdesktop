#pragma once

#include "../tray_icon.hpp"

namespace core {
namespace platform  {
namespace macos {

class TrayMac : public core::platform::TrayIcon
{
public:
    TrayMac();
    ~TrayMac();



    // TrayIcon interface
public:
    void aboutToShowRequests() override;
    void showFromTrayRequest() override;
    void hideToTrayRequest() override;
    void iconClicked() override;
    void setEnableNotification(bool enable) override;

    void showTrayIcon()  override;
    void setup(QWindow *window, QApplication *application) override;
    bool hasIcon()  override;
    void createIcon() override;
    void detroyIcon() override;
    void updateIcon() override;
    void createMenu() override;
    void destroyMenu() override;
    void addAction() override;
    void addSeperate() override;

    void setTooltip(const QString &tooltip) override;
    void setOpenCallback(std::__1::function<void ()> callback) override;
    void setQuitCallback(std::__1::function<void ()> callback) override;
    void setTrayClickCallback(std::function<void()> callback) override;

private:
    class Impl;
    std::unique_ptr<Impl> d;


};


} // namespace macos
} // namespace platform
} // namespace core
