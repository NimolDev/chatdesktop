#include "qt_camera_capture.hpp"
#include "qt_camera_frame.hpp"
#include "logger.hpp"
#include <QCamera>
#include <QCameraDevice>
#include <QMediaDevices>
#include <QMediaCaptureSession>
#include <QVideoSink>
#include <QMutexLocker>
#include <QThread>
#include <limits>
#include <cmath>

namespace core {
namespace rtc {

QVideoFrame QtCameraCapture::takeFrame()
{
    QMutexLocker lock(&m_frameMutex);
    QVideoFrame frame = std::move(m_latestFrame);
    m_latestFrame = {};
    return frame;
}

void QtCameraCapture::setDevice(const QCameraDevice &device)
{
    Q_ASSERT(QThread::currentThread() == thread());
    if (m_hasDeviceSelection && m_device == device) {
        return;
    }
    const bool restart = m_requested;
    stop();
    delete m_session;
    delete m_sink;
    delete m_camera;
    m_session = nullptr;
    m_sink = nullptr;
    m_camera = nullptr;
    m_device = device;
    m_hasDeviceSelection = true;
    if (restart) {
        start();
        // Remember capture intent while the selected camera is unplugged.
        if (device.isNull()) {
            m_requested = true;
        }
    }
}

void QtCameraCapture::prepare()
{
    Q_ASSERT(QThread::currentThread() == thread());
    if (!m_camera) {
        const auto device = m_hasDeviceSelection ? m_device : QMediaDevices::defaultVideoInput();
        if (device.isNull()) {
            emit errorOccurred(QStringLiteral("No usable local camera is available"));
            return;
        }
        // Prefer 720p at 30 FPS, falling back to the nearest supported format.
        QCameraFormat selected;
        double bestScore = std::numeric_limits<double>::max();
        for (const auto &format : device.videoFormats()) {
            if (!isCameraPixelFormatSupported(format.pixelFormat())) {
                continue;
            }
            const auto size = format.resolution();
            const double score = std::abs(size.width() - 1280)
                                 + std::abs(size.height() - 720)
                                 + 100 * std::max(0.0, 30.0 - format.maxFrameRate())
                                 + 100 * std::max(0.0, format.minFrameRate() - 30.0);
            if (score < bestScore) {
                bestScore = score;
                selected = format;
            }
        }
        if (selected.isNull()) {
            emit errorOccurred(QStringLiteral("Camera has no supported video format"));
            return;
        }
        m_camera = new QCamera(device, this);
        m_camera->setCameraFormat(selected);
        m_session = new QMediaCaptureSession(this);
        m_sink = new QVideoSink(this);
        m_session->setCamera(m_camera);
        m_session->setVideoSink(m_sink);


        connect(m_camera, &QCamera::activeChanged, this, &QtCameraCapture::activeChanged);
        connect(m_camera, &QCamera::activeChanged, this, [this](bool active) {
            if (active && m_requested) {
                LOG_INFO(QStringLiteral("Qt camera active: %1 ms").arg(m_startTimer.elapsed()));
            }
        });
        connect(m_camera, &QCamera::errorOccurred, this,
                [this](QCamera::Error, const QString &message) {
                    m_requested = false;
                    emit errorOccurred(message);
                });
        connect(m_sink, &QVideoSink::videoFrameChanged, this,
                [this](const QVideoFrame &frame) {
                    if (!m_requested || !frame.isValid()) {
                        return;
                    }
                    const auto receivedMs = m_startTimer.elapsed();
                    const auto preview = copyCameraFrame(frame);
                    if (!preview.isValid()) {
                        return;
                    }
                    if (m_firstFrame) {
                        m_firstFrame = false;
                        LOG_INFO(QStringLiteral("Qt camera first frame: %1 ms").arg(m_startTimer.elapsed()));
                        LOG_INFO(QStringLiteral("Qt camera first frame breakdown: arrival=%1 ms, copy=%2 ms")
                                     .arg(receivedMs)
                                     .arg(m_startTimer.elapsed() - receivedMs));
                    }
                    {
                        QMutexLocker lock(&m_frameMutex);
                        m_latestFrame = preview;
                    }
                    emit frameReady(preview);
                });
        LOG_INFO ("Prepare camera");
    }
}

void QtCameraCapture::start()
{
    Q_ASSERT(QThread::currentThread() == thread());
    if (m_requested) {
        return;
    }
    if (!m_camera) {
        prepare();
    }
    if (!m_camera) {
        return;
    }
    m_startTimer.start();
    m_firstFrame = true;
    m_requested = true;
    m_camera->start();
    LOG_INFO(QStringLiteral("Qt camera start returned: %1 ms").arg(m_startTimer.elapsed()));
}

void QtCameraCapture::stop()
{
    Q_ASSERT(QThread::currentThread() == thread());
    m_requested = false;
    if (m_camera) {
        m_camera->stop();
    }
    {
        QMutexLocker lock(&m_frameMutex);
        m_latestFrame = {};
    }
    emit frameReady({});
}

} // namespace rtc
} // namespace core
