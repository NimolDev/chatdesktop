#pragma once

#include <functional>

namespace core {
namespace platform {

class ApplicationMenu
{
public:
    virtual ~ApplicationMenu() = default;

    virtual void setup() = 0;

    virtual void setAboutCallback(std::function<void()> callback) = 0;
    virtual void setSettingCallback(std::function<void()> callback) = 0;
    virtual void setQuiteCallback(std::function<void()> callback) = 0;

};

} // namespace platform

} // namespace core