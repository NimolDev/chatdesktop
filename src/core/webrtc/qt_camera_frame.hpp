#ifndef CORE_WEBRTC_QT_CAMERA_FRAME_HPP
#define CORE_WEBRTC_QT_CAMERA_FRAME_HPP

#include <QVideoFrame>
#include <api/video/i420_buffer.h>
#include <libyuv/convert.h>
#include <algorithm>
#include <cstring>

namespace core {
namespace rtc {

inline bool isCameraPixelFormatSupported(QVideoFrameFormat::PixelFormat format)
{
    switch (format) {
    case QVideoFrameFormat::Format_NV12:
    case QVideoFrameFormat::Format_NV21:
    case QVideoFrameFormat::Format_YUV420P:
    case QVideoFrameFormat::Format_YV12:
    case QVideoFrameFormat::Format_BGRA8888:
    case QVideoFrameFormat::Format_BGRX8888:
    case QVideoFrameFormat::Format_RGBA8888:
    case QVideoFrameFormat::Format_RGBX8888:
    case QVideoFrameFormat::Format_UYVY:
    case QVideoFrameFormat::Format_YUYV:
        return true;
    default:
        return false;
    }
}

inline webrtc::scoped_refptr<webrtc::I420Buffer> cameraFrameToI420(QVideoFrame frame)
{
    if (!frame.isValid() || !isCameraPixelFormatSupported(frame.pixelFormat())
        || !frame.map(QVideoFrame::ReadOnly)) {
        return nullptr;
    }
    auto buffer = webrtc::I420Buffer::Create(frame.width(), frame.height());
    auto *y = buffer->MutableDataY();
    auto *u = buffer->MutableDataU();
    auto *v = buffer->MutableDataV();
    const int sy = buffer->StrideY();
    const int su = buffer->StrideU();
    const int sv = buffer->StrideV();
    const int w = frame.width();
    const int h = frame.height();
    int result = -1;
    switch (frame.pixelFormat()) {
    case QVideoFrameFormat::Format_NV12:
        result = libyuv::NV12ToI420(frame.bits(0), frame.bytesPerLine(0),
            frame.bits(1), frame.bytesPerLine(1), y, sy, u, su, v, sv, w, h);
        break;
    case QVideoFrameFormat::Format_NV21:
        result = libyuv::NV21ToI420(frame.bits(0), frame.bytesPerLine(0),
            frame.bits(1), frame.bytesPerLine(1), y, sy, u, su, v, sv, w, h);
        break;
    case QVideoFrameFormat::Format_YUV420P:
    case QVideoFrameFormat::Format_YV12: {
        const int up = frame.pixelFormat() == QVideoFrameFormat::Format_YV12 ? 2 : 1;
        const int vp = 3 - up;
        result = libyuv::I420Copy(frame.bits(0), frame.bytesPerLine(0),
            frame.bits(up), frame.bytesPerLine(up), frame.bits(vp), frame.bytesPerLine(vp),
            y, sy, u, su, v, sv, w, h);
        break;
    }
    case QVideoFrameFormat::Format_BGRA8888:
    case QVideoFrameFormat::Format_BGRX8888:
        result = libyuv::ARGBToI420(frame.bits(0), frame.bytesPerLine(0),
            y, sy, u, su, v, sv, w, h);
        break;
    case QVideoFrameFormat::Format_RGBA8888:
    case QVideoFrameFormat::Format_RGBX8888:
        result = libyuv::ABGRToI420(frame.bits(0), frame.bytesPerLine(0),
            y, sy, u, su, v, sv, w, h);
        break;
    case QVideoFrameFormat::Format_UYVY:
        result = libyuv::UYVYToI420(frame.bits(0), frame.bytesPerLine(0),
            y, sy, u, su, v, sv, w, h);
        break;
    case QVideoFrameFormat::Format_YUYV:
        result = libyuv::YUY2ToI420(frame.bits(0), frame.bytesPerLine(0),
            y, sy, u, su, v, sv, w, h);
        break;
    default:
        break;
    }
    frame.unmap();
    return result == 0 ? buffer : nullptr;
}

inline QVideoFrame copyCameraFrame(QVideoFrame frame)
{
    if (!frame.isValid() || !frame.map(QVideoFrame::ReadOnly)) {
        return {};
    }

    // A capture-only Darwin sink has no Metal texture cache. Never call
    // toImage() on its native buffer: Qt may attempt GPU texture conversion.
    // Preserve the surface format but detach the pixels from native resources.
    QVideoFrame pixels(frame.surfaceFormat());
    if (!pixels.map(QVideoFrame::WriteOnly)) {
        frame.unmap();
        return {};
    }
    bool copied = pixels.planeCount() == frame.planeCount();
    for (int plane = 0; copied && plane < pixels.planeCount(); ++plane) {
        const int sourceStride = frame.bytesPerLine(plane);
        const int targetStride = pixels.bytesPerLine(plane);
        if (sourceStride <= 0 || targetStride <= 0) {
            copied = false;
            break;
        }
        const int rows = pixels.mappedBytes(plane) / targetStride;
        const int bytes = std::min(sourceStride, targetStride);
        if (frame.mappedBytes(plane) < (rows - 1) * sourceStride + bytes) {
            copied = false;
            break;
        }
        std::memset(pixels.bits(plane), 0, pixels.mappedBytes(plane));
        for (int row = 0; row < rows; ++row) {
            std::memcpy(pixels.bits(plane) + row * targetStride,
                        frame.bits(plane) + row * sourceStride, bytes);
        }
    }
    pixels.unmap();
    frame.unmap();
    pixels.setRotation(frame.rotation());
    pixels.setStartTime(frame.startTime());
    pixels.setEndTime(frame.endTime());
    // Mirroring is applied only by the local VideoOutput, never to the sender.
    return copied ? pixels : QVideoFrame();
}

} // namespace rtc
} // namespace core

#endif // CORE_WEBRTC_QT_CAMERA_FRAME_HPP
