#pragma once

#include "../tray_icon.hpp"
#include <memory>

namespace core {
namespace platform {
namespace win {

class TrayWin final : public TrayIcon
{
public:
    TrayWin();
    ~TrayWin() override;

    void setup(QWindow *window, QApplication *application) override;
    void aboutToShowRequests() override;
    void showFromTrayRequest() override;
    void hideToTrayRequest() override;
    void iconClicked() override;
    void setEnableNotification(bool enable) override;
    bool hasIcon() override;
    void createIcon() override;
    void detroyIcon() override;
    void updateIcon() override;
    void createMenu() override;
    void destroyMenu() override;
    void addAction() override;
    void addSeperate() override;
    void showTrayIcon() override;
    void setTooltip(const QString &tooltip) override;
    void setOpenCallback(std::function<void()> callback) override;
    void setQuitCallback(std::function<void()> callback) override;
    void setTrayClickCallback(std::function<void()> callback) override;

    static void showNotification(const QString &title, const QString &message);

private:
    class Impl;
    std::unique_ptr<Impl> d;
};

} // namespace win
} // namespace platform
} // namespace core
