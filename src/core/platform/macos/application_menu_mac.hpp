#pragma once

#include "../application_menu.hpp"
namespace core {
namespace platform {
namespace macos {

class ApplicationMenuMac final : public ApplicationMenu
{
public:
    explicit ApplicationMenuMac();
    ~ApplicationMenuMac();


    // ApplicationMenu interface
public:
    void setup() override;


private:
    class Impl;
    std::unique_ptr<Impl> d;


    // ApplicationMenu interface
public:
    void setAboutCallback(std::__1::function<void ()> callback) override;
    void setSettingCallback(std::__1::function<void ()> callback) override;
    void setQuiteCallback(std::__1::function<void ()> callback) override;

private:
    void createAppMenu();
    void createFileMenu();
    void createEditMenu();
    void createWindowMenu();

    // void addMenu();
};


} // namespace macos
} // namespace platform
} // namespace core
