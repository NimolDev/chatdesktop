#include "mac_camera_capture.hpp"
#include "logger.hpp"

#import <AVFoundation/AVFoundation.h>
#include <api/video/nv12_buffer.h>
#include <api/video/video_frame.h>
#include <rtc_base/time_utils.h>
#include <cstring>
#include <optional>

@interface ChatCameraDelegate : NSObject <AVCaptureVideoDataOutputSampleBufferDelegate>
@property(nonatomic, copy) void (^frameHandler)(CMSampleBufferRef);
@end

@implementation ChatCameraDelegate
- (void)captureOutput:(AVCaptureOutput *)output
    didOutputSampleBuffer:(CMSampleBufferRef)sample
    fromConnection:(AVCaptureConnection *)connection
{
    if (self.frameHandler) {
        self.frameHandler(sample);
    }
}
@end

namespace core {
namespace rtc {

namespace {

std::optional<webrtc::VideoFrame> cameraFrameFromSample(CMSampleBufferRef sample)
{
    CVPixelBufferRef pixel = CMSampleBufferGetImageBuffer(sample);
    if (!pixel || CVPixelBufferGetPixelFormatType(pixel) != kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange
        || CVPixelBufferGetPlaneCount(pixel) != 2) {
        return std::nullopt;
    }
    if (CVPixelBufferLockBaseAddress(pixel, kCVPixelBufferLock_ReadOnly) != kCVReturnSuccess) {
        return std::nullopt;
    }
    const int width = static_cast<int>(CVPixelBufferGetWidth(pixel));
    const int height = static_cast<int>(CVPixelBufferGetHeight(pixel));
    auto buffer = webrtc::NV12Buffer::Create(width, height);
    const auto *y = static_cast<const uint8_t *>(CVPixelBufferGetBaseAddressOfPlane(pixel, 0));
    const auto *uv = static_cast<const uint8_t *>(CVPixelBufferGetBaseAddressOfPlane(pixel, 1));
    for (int row = 0; row < height; ++row) {
        std::memcpy(buffer->MutableDataY() + row * buffer->StrideY(),
                    y + row * CVPixelBufferGetBytesPerRowOfPlane(pixel, 0), width);
    }
    for (int row = 0; row < (height + 1) / 2; ++row) {
        std::memcpy(buffer->MutableDataUV() + row * buffer->StrideUV(),
                    uv + row * CVPixelBufferGetBytesPerRowOfPlane(pixel, 1),
                    ((width + 1) / 2) * 2);
    }
    CVPixelBufferUnlockBaseAddress(pixel, kCVPixelBufferLock_ReadOnly);
    auto frame = webrtc::VideoFrame::Builder()
                     .set_video_frame_buffer(buffer->ToI420())
                     .set_timestamp_us(webrtc::TimeMicros())
                     .set_rotation(webrtc::kVideoRotation_0)
                     .build();
    return frame;
}

} // namespace

class MacCameraCapture::Private
{
public:
    FrameHandler handler;
    AVCaptureSession *session = nil;
    AVCaptureVideoDataOutput *output = nil;
    ChatCameraDelegate *delegate = nil;
    dispatch_queue_t frameQueue = dispatch_queue_create("chat.camera.frames", DISPATCH_QUEUE_SERIAL);

    void deliver(CMSampleBufferRef sample)
    {
        if (auto frame = cameraFrameFromSample(sample)) {
            handler(*frame);
        }
    }
};

MacCameraCapture::MacCameraCapture(FrameHandler handler)
    : d(std::make_unique<Private>())
{
    d->handler = std::move(handler);
}

MacCameraCapture::~MacCameraCapture()
{
    stop();
}

bool MacCameraCapture::initialize(int width, int height, int fps)
{
    @autoreleasepool {
        AVCaptureDevice *device = [AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeVideo];
        if (!device) {
            LOG_WARNING("No macOS camera is available");
            return false;
        }
        NSError *error = nil;
        AVCaptureDeviceInput *input = [AVCaptureDeviceInput deviceInputWithDevice:device error:&error];
        if (!input) {
            LOG_WARNING(QString::fromUtf8(error.localizedDescription.UTF8String));
            return false;
        }
        d->session = [[AVCaptureSession alloc] init];
        d->output = [[AVCaptureVideoDataOutput alloc] init];
        d->output.alwaysDiscardsLateVideoFrames = YES;
        d->output.videoSettings = @{
            (NSString *)kCVPixelBufferPixelFormatTypeKey: @(kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange)
        };
        [d->session beginConfiguration];
        if (![d->session canAddInput:input] || ![d->session canAddOutput:d->output]) {
            [d->session commitConfiguration];
            LOG_WARNING("Cannot configure the macOS camera session");
            return false;
        }
        [d->session addInput:input];
        [d->session addOutput:d->output];
        NSString *preset = width >= 1280 && height >= 720
            ? AVCaptureSessionPreset1280x720 : AVCaptureSessionPreset640x480;
        if ([d->session canSetSessionPreset:preset]) {
            d->session.sessionPreset = preset;
        }
        [d->session commitConfiguration];
        if (fps > 0 && [device lockForConfiguration:&error]) {
            for (AVFrameRateRange *range in device.activeFormat.videoSupportedFrameRateRanges) {
                if (range.minFrameRate <= fps && range.maxFrameRate >= fps) {
                    device.activeVideoMinFrameDuration = CMTimeMake(1, fps);
                    device.activeVideoMaxFrameDuration = CMTimeMake(1, fps);
                    break;
                }
            }
            [device unlockForConfiguration];
        }
        d->delegate = [[ChatCameraDelegate alloc] init];
        auto *capture = d.get();
        d->delegate.frameHandler = ^(CMSampleBufferRef sample) {
            capture->deliver(sample);
        };
        return true;
    }
}

bool MacCameraCapture::start()
{
    @autoreleasepool {
        if (!d->session || [AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeVideo]
                               != AVAuthorizationStatusAuthorized) {
            LOG_WARNING("Camera capture requires macOS camera permission");
            return false;
        }
        [d->output setSampleBufferDelegate:d->delegate queue:d->frameQueue];
        [d->session startRunning];
        return d->session.isRunning;
    }
}

void MacCameraCapture::stop()
{
    @autoreleasepool {
        [d->output setSampleBufferDelegate:nil queue:nullptr];
        [d->session stopRunning];
        // Finish any in-flight frame before the source or delegate is released.
        dispatch_sync(d->frameQueue, ^{});
    }
}

} // namespace rtc
} // namespace core
