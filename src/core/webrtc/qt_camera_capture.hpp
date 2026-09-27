#ifndef CORE_WEBRTC_QT_CAMERA_CAPTURE_HPP
#define CORE_WEBRTC_QT_CAMERA_CAPTURE_HPP

#include <QObject>
#include <QCameraDevice>
#include <QVideoFrame>
#include <QMutex>
#include <QElapsedTimer>

class QCamera;
class QMediaCaptureSession;
class QVideoSink;

namespace core {
namespace rtc {

// All camera operations run on the capture thread. Only takeFrame() may be
// called from another thread; it consumes a single latest-frame mailbox.
class QtCameraCapture final : public QObject
{
    Q_OBJECT
public:
    QVideoFrame takeFrame();

public slots:
    void setDevice(const QCameraDevice &device);
    void prepare();
    void start();
    void stop();

signals:
    void frameReady(const QVideoFrame &frame);
    void errorOccurred(const QString &message);
    void activeChanged(bool active);

private:
    QCameraDevice m_device;
    bool m_hasDeviceSelection = false;
    QCamera *m_camera = nullptr;
    QMediaCaptureSession *m_session = nullptr;
    QVideoSink *m_sink = nullptr;
    bool m_requested = false;
    bool m_firstFrame = false;
    QElapsedTimer m_startTimer;
    QMutex m_frameMutex;
    QVideoFrame m_latestFrame;
};

} // namespace rtc
} // namespace core

#endif // CORE_WEBRTC_QT_CAMERA_CAPTURE_HPP
