#ifndef CORE_WEBRTC_CAMERA_VIDEO_SOURCE_HPP
#define CORE_WEBRTC_CAMERA_VIDEO_SOURCE_HPP

#include <atomic>
#include <memory>
#include <mutex>
#include <api/media_stream_interface.h>
#include <api/scoped_refptr.h>
#include <api/video/video_frame.h>
#include <media/base/adapted_video_track_source.h>
#include <modules/video_capture/video_capture.h>

namespace core {
namespace rtc {

#if defined(WEBRTC_MAC)
class MacCameraCapture;
#endif

class CameraVideoSource  : public webrtc::AdaptedVideoTrackSource,
                          public  webrtc::VideoSinkInterface<webrtc::VideoFrame>
{
public:
    // explicit CameraVideoSource();
    CameraVideoSource(
        int width,
        int height,
        int fps
        );
static webrtc::scoped_refptr<CameraVideoSource> Create (
    int width = 1280,
    int height = 720,
    int fps = 30
        );

    void OnFrame(const webrtc::VideoFrame &frame) override;
    bool start();
    void stop();


    bool isRunning() const;
    std::optional<webrtc::VideoFrame> takePreviewFrame();

protected:
    ~CameraVideoSource() override;
    SourceState state() const override;
    bool remote() const override;
    bool is_screencast() const override;
    std::optional<bool> needs_denoising() const override;


private:


    bool initialize();

private:
    std::mutex m_previewMutex;
    std::optional<webrtc::VideoFrame> m_previewFrame;
    int m_width;
    int m_height;
    int m_fps;

    std::atomic<bool> m_running{false};
#if defined(WEBRTC_MAC)
    std::unique_ptr<MacCameraCapture> m_macCapture;
#endif
    webrtc::VideoCaptureCapability m_capability;
    webrtc::scoped_refptr<webrtc::VideoCaptureModule> m_captureModule;

};


} // namespace rtc
} // namespace core


#endif // CORE_WEBRTC_CAMERA_VIDEO_SOURCE_HPP
