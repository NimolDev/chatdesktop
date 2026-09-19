#pragma once

// #include <functional>
#include <functional>
#include <memory>

namespace webrtc {
class VideoFrame;
}

namespace core {
namespace rtc {

class MacCameraCapture
{
public:
    using FrameHandler = std::function<void(const webrtc::VideoFrame &)>;
    explicit MacCameraCapture(FrameHandler handler);
    ~MacCameraCapture();
    bool initialize(int width, int height, int fps);
    bool start();
    void stop();

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace rtc
} // namespace core
