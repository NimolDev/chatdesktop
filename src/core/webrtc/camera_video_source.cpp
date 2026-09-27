#include "camera_video_source.hpp"
#include "logger.hpp"
#if defined(WEBRTC_MAC)
#include "mac_camera_capture.hpp"
#endif

#include <memory>
#include <api/make_ref_counted.h>
#include <modules/video_capture/video_capture_factory.h>


core::rtc::CameraVideoSource::CameraVideoSource(
    int width,
    int height,
    int fps)
    : m_width(width),
    m_height(height),
    m_fps(fps)
{}


webrtc::scoped_refptr<core::rtc::CameraVideoSource>
core::rtc::CameraVideoSource::Create(int width, int height, int fps)
{
    auto source = webrtc::make_ref_counted<CameraVideoSource> (width, height, fps);
    if (!source->initialize ()) {
        return nullptr;
    }
    return source;
}

bool core::rtc::CameraVideoSource::initialize()
{
#if defined(WEBRTC_MAC)
    m_macCapture = std::make_unique<MacCameraCapture>([this](const webrtc::VideoFrame &frame) {
        OnFrame(frame);
    });
    return m_macCapture->initialize(m_width, m_height, m_fps);
#else
    std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo> device_info(
        webrtc::VideoCaptureFactory::CreateDeviceInfo());

    if (!device_info) {
        LOG_WARNING ("Unable to create camera device info");
        return false;
    }

    const uint32_t count = device_info->NumberOfDevices ();
    LOG_INFO (QStringLiteral ("Camera count: %1").arg (count));

    if (count == 0) {
        LOG_WARNING ("No camera available");
        return false;
    }
    char device_name[256] = {};
    char unique_id[256] = {};

    const int result = device_info->GetDeviceName (0, // select 0: for first camera for some device it have multiple camera
                                                  device_name,
                                                  sizeof(device_name),
                                                  unique_id,
                                                  sizeof(unique_id));

    if (result != 0) {
        LOG_WARNING ("Failed to get camera");
        return false;
    }

    LOG_INFO (QStringLiteral ("Camera: %1").arg (device_name));
    LOG_INFO (QStringLiteral ("Camera ID: %1").arg (unique_id));

    m_captureModule = webrtc::VideoCaptureFactory::Create (unique_id);

    if (!m_captureModule) {
        LOG_WARNING ("Failed to create VideoCaptureModule");
        return false;
    }
    webrtc::VideoCaptureCapability requested;
    requested.width = m_width;
    requested.height = m_height;
    requested.maxFPS = m_fps;
    requested.videoType = webrtc::VideoType::kI420;
    if (device_info->GetBestMatchedCapability(unique_id, requested, m_capability) < 0) {
        m_capability = requested;
    }
    m_captureModule->RegisterCaptureDataCallback (this);
    return true;
#endif
}


void core::rtc::CameraVideoSource::OnFrame(const webrtc::VideoFrame &frame)
{
    webrtc::AdaptedVideoTrackSource::OnFrame(frame);
    std::lock_guard<std::mutex> lock(m_previewMutex);
    m_previewFrame = frame;
}

std::optional<webrtc::VideoFrame> core::rtc::CameraVideoSource::takePreviewFrame()
{
    std::lock_guard<std::mutex> lock(m_previewMutex);
    auto frame = std::move(m_previewFrame);
    m_previewFrame.reset();
    return frame;
}

bool core::rtc::CameraVideoSource::start()
{
#if defined(WEBRTC_MAC)
    if (!m_running) {
        m_running = m_macCapture && m_macCapture->start();
    }
    return m_running;
#else
    if (!m_captureModule) {
        LOG_WARNING("Capture module is null");
        return false;
    }
    if (m_running) {
        return true;
    }
    const int result = m_captureModule->StartCapture(m_capability);
    if (result != 0) {
        LOG_WARNING(QStringLiteral("Failed to start camera capture: %1").arg(result));
        return false;
    }
    m_running = true;
    LOG_INFO(QStringLiteral("Camera started: %1 x %2 @ %3")
                 .arg(m_capability.width).arg(m_capability.height).arg(m_capability.maxFPS));
    return true;
#endif
}

core::rtc::CameraVideoSource::~CameraVideoSource()
{
    stop();
    if (m_captureModule) {
        m_captureModule->DeRegisterCaptureDataCallback();
    }
}

void core::rtc::CameraVideoSource::stop()
{
#if defined(WEBRTC_MAC)
    if (m_macCapture) {
        m_macCapture->stop();
    }
    m_running = false;
#else
    if (!m_captureModule) {
        return;
    }
    if (!m_running) {
        return;
    }
    if (m_captureModule->CaptureStarted ()) {
        m_captureModule->StopCapture ();
    }
    m_running = false;
    LOG_INFO ("Camera stopped");
#endif
    std::lock_guard<std::mutex> lock(m_previewMutex);
    m_previewFrame.reset();
}

bool core::rtc::CameraVideoSource::isRunning() const
{
    return m_running;
}


// MARK: - Video Track source
webrtc::MediaSourceInterface::SourceState
core::rtc::CameraVideoSource::state() const
{
    return m_running
               ? SourceState::kLive
               : SourceState::kMuted;
}
bool core::rtc::CameraVideoSource::remote() const
{
    return false;
}

bool core::rtc::CameraVideoSource::is_screencast() const
{
    return false;
}

std::optional<bool> core::rtc::CameraVideoSource::needs_denoising() const
{
    return false;
}
