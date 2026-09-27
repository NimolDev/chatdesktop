#include "webrtc_client.hpp"
#include "peer_connection_observer.hpp"
#include "sdp_observer.hpp"
#include "set_remote_description_observer.hpp"
#include "logger.hpp"
#include "camera_video_source.hpp"

#include <QMetaObject>
#include <QPointer>
#include <QTimer>
#include <QTransform>
#include <common_video/libyuv/include/webrtc_libyuv.h>

#include "api/video/video_frame.h"
#include <api/audio_options.h>
#include <api/create_peerconnection_factory.h>
#include <api/make_ref_counted.h>
#include <api/set_local_description_observer_interface.h>
#include "api/audio/audio_device.h"
#include "api/audio/create_audio_device_module.h"
#include "api/environment/environment.h"
#include "api/environment/environment_factory.h"
#include "api/audio_codecs/builtin_audio_decoder_factory.h"
#include "api/audio_codecs/builtin_audio_encoder_factory.h"
#include "api/video_codecs/builtin_video_encoder_factory.h"
#include "api/video_codecs/builtin_video_decoder_factory.h"
#include "media/base/adapted_video_track_source.h"

namespace core {
namespace rtc {

class SetLocalDescriptionObserver
    : public webrtc::SetLocalDescriptionObserverInterface
{
public:
    using CompletionHandler = std::function<void(webrtc::RTCError)>;

    explicit SetLocalDescriptionObserver(CompletionHandler handler)
        : m_handler(std::move(handler))
    {
    }

    void OnSetLocalDescriptionComplete(webrtc::RTCError error) override
    {
        if (m_handler) {
            m_handler(std::move(error));
        }
    }

protected:
    ~SetLocalDescriptionObserver() override = default;

private:
    CompletionHandler m_handler;
};

class LocalVideoTrackSource : public webrtc::AdaptedVideoTrackSource
{
public:
    webrtc::MediaSourceInterface::SourceState state() const override
    {
        return webrtc::MediaSourceInterface::kLive;
    }

    bool remote() const override { return false; }
    bool is_screencast() const override { return false; }
    std::optional<bool> needs_denoising() const override
    {
        return std::nullopt;
    }

    void pushFrame(const webrtc::VideoFrame &frame) { OnFrame(frame); }

protected:
    ~LocalVideoTrackSource() override = default;
};

class WebRtcClientPrivate

{

public:

    bool cameraRequested = false;
    bool peerConnected = false;
    webrtc::AudioOptions audioOptions;

    std::unique_ptr<webrtc::Thread> networkThread;
    std::unique_ptr<webrtc::Thread> workerThread;
    std::unique_ptr<webrtc::Thread> signalingThread;

    webrtc::scoped_refptr<webrtc::AudioDeviceModule> audioDevice;
    webrtc::scoped_refptr<LocalVideoTrackSource> videoSource;
    webrtc::scoped_refptr<webrtc::AudioTrackInterface> m_localAudioTrack;
    // webrtc::scoped_refptr<webrtc::VideoTrackInterface> m_localVideoTrack;

    webrtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> m_factory;
    webrtc::scoped_refptr<webrtc::PeerConnectionInterface> m_peerConnection;

    std::unique_ptr<core::rtc::PeerConnectionObserver> m_peerObserver;
    webrtc::scoped_refptr<core::rtc::SdpObserver> m_sdpObserver;
    webrtc::scoped_refptr<core::rtc::SetRemoteDescriptionObserver> m_remoteSdpObserver;

    webrtc::scoped_refptr<core::rtc::CameraVideoSource> m_cameraSource;
    webrtc::scoped_refptr<webrtc::VideoTrackInterface> m_localVideoTrack;

};

WebrtcClient::WebrtcClient(QObject *parent)
    : QObject(parent),
      d(std::make_unique<WebRtcClientPrivate>())
{
    auto *videoTimer = new QTimer(this);
    videoTimer->setInterval(33);
    connect(videoTimer, &QTimer::timeout, this, [this] {
        QImage image;
        if (d->peerConnected && d->m_peerObserver
            && d->m_peerObserver->takeRemoteVideoFrame(image)) {
            emit remoteVideoFrameReady(image);
        }
        if (d->peerConnected && d->m_cameraSource && d->m_cameraSource->isRunning()) {
            const auto frame = d->m_cameraSource->takePreviewFrame();
            if (!frame) {
                return;
            }
            const auto buffer = frame->video_frame_buffer()->ToI420();
            if (!buffer) {
                return;
            }
            QImage preview(buffer->width(), buffer->height(), QImage::Format_RGBA8888);
            const auto i420Frame = webrtc::VideoFrame::Builder()
                                       .set_video_frame_buffer(buffer).build();
            if (preview.isNull() || webrtc::ConvertFromI420(i420Frame, webrtc::VideoType::kABGR,
                    preview.bytesPerLine(), preview.bits()) != 0) {
                return;
            }
            if (frame->rotation() != webrtc::kVideoRotation_0) {
                preview = preview.transformed(QTransform().rotate(static_cast<int>(frame->rotation())));
            }
            emit localVideoFrameReady(preview);
        }
    });
    videoTimer->start();
}

WebrtcClient::~WebrtcClient()
{
    if (d->m_cameraSource) {
        d->m_cameraSource->stop();
    }
    if (d->m_peerConnection) {
        d->m_peerConnection->Close();
    }
    if (d->m_peerObserver && d->signalingThread) {
        d->signalingThread->BlockingCall([this] {
            d->m_peerObserver->detachRemoteVideo();
        });
    }
    d->m_peerConnection = nullptr;
    d->m_localVideoTrack = nullptr;
    d->m_cameraSource = nullptr;
    d->videoSource = nullptr;
    d->m_localAudioTrack = nullptr;
    d->m_factory = nullptr;
    d->audioDevice = nullptr;
    if (d->networkThread) {
        d->networkThread->Stop ();
    }
    if (d->workerThread) {
        d->workerThread->Stop ();
    }
    if (d->signalingThread) {
        d->signalingThread->Stop ();
    }


}

void WebrtcClient::configureAudioSession()
{
    // The native desktop ADM handles default input/output devices and starts
    // capture/playout as the negotiated audio tracks become active.
    d->audioOptions.echo_cancellation = true;
    d->audioOptions.auto_gain_control = true;
    d->audioOptions.noise_suppression = true;
    d->audioOptions.highpass_filter = true;
    d->audioOptions.init_recording_on_send = true;

    webrtc::Environment m_env = webrtc::CreateEnvironment ();
    d->audioDevice = webrtc::CreateAudioDeviceModule (m_env,
                                                     webrtc::AudioDeviceModule::kPlatformDefaultAudio);

    if (!d->audioDevice) {
        LOG_CRITICAL (QStringLiteral ("Failed to create AudioDeviceMode"));
        return;
    }
    qDebug() << "Recording devices:"
             << d->audioDevice->RecordingDevices();

    qDebug() << "Playback devices:"
             << d->audioDevice->PlayoutDevices();


}

bool WebrtcClient::initialize()
{
    configureAudioSession();
    d->networkThread = webrtc::Thread::CreateWithSocketServer ();
    d->workerThread = webrtc::Thread::Create ();
    d->signalingThread = webrtc::Thread::Create ();

    d->networkThread->SetName ("WebRTCNetwork", nullptr);
    d->workerThread->SetName ("WebRTCWorkder", nullptr);
    d->signalingThread->SetName ("WebRTCSignaling", nullptr);

    if (!d->networkThread->Start ()) {
        LOG_WARNING("WebRTCNetwork not start");
        return false;
    }
    if (!d->workerThread->Start ()) {
        LOG_WARNING ("WebRTCWorker not start");
        return false;
    }
    if(!d->signalingThread->Start ()) {
        LOG_WARNING ("WebRTCSignaling not start");
        return false;
    }

    d->m_factory = webrtc::CreatePeerConnectionFactory(
        d->networkThread.get(),
        d->workerThread.get(),
        d->signalingThread.get(),
        nullptr,
        webrtc::CreateBuiltinAudioEncoderFactory(),
        webrtc::CreateBuiltinAudioDecoderFactory(),
        webrtc::CreateBuiltinVideoEncoderFactory(),
        webrtc::CreateBuiltinVideoDecoderFactory(),
        nullptr,
        nullptr);

    if (!d->m_factory) {
        LOG_CRITICAL("Failed to create WebRTC factory");
        return false;
    }

    createLocalAudioTrack ();
    LOG_INFO("WebRTC PeerConnectionFactory ready");
    return true;
}

bool WebrtcClient::createPeerConnection(const QList<core::rtc::IceServer> &iceServers)
{
    if (!d->m_factory) {
        LOG_CRITICAL ("PeerConnectionFactory not initialize");
        return false;
    }

    if (d->m_peerConnection) {
        LOG_WARNING("Peer connection already exists for this call");
        return false;
    }

    webrtc::PeerConnectionInterface::RTCConfiguration config;
    for (const auto &ice : iceServers) {
        if (ice.host.trimmed().isEmpty()) {
            LOG_WARNING("Cannot configure an ICE server without a host");
            return false;
        }
        webrtc::PeerConnectionInterface::IceServer server;
        server.urls.push_back(ice.url().toStdString());
        server.username = ice.username.toStdString();
        server.password = ice.credential.toStdString();
        config.servers.push_back(std::move(server));
    }

    d->m_peerObserver = std::make_unique<core::rtc::PeerConnectionObserver> ();
    connect(d->m_peerObserver.get(), &PeerConnectionObserver::errorOccurred,
            this, &WebrtcClient::errorOccurred, Qt::QueuedConnection);
    connect(d->m_peerObserver.get(),
            &PeerConnectionObserver::iceCandidateChanged,
            this,
            [this](const QString &candidate, const QString &sdpMid, int sdpMLineIndex) {
                emit localIceCandidateGenerated(candidate, sdpMid, sdpMLineIndex);
            });

    connect(d->m_peerObserver.get(), &PeerConnectionObserver::connectionStateChanged,
            this, [this](webrtc::PeerConnectionInterface::PeerConnectionState state) {
                using State = webrtc::PeerConnectionInterface::PeerConnectionState;
                d->peerConnected = state == State::kConnected;
                if (!d->peerConnected) {
                    emit remoteVideoFrameReady(QImage());
                }
                updateCameraCapture();
                if (d->peerConnected) {
                    emit connectionStateChanged(ConnectionState::Connected);
                    emit connected();
                    setMuteMicrophone (false);
                } else if (state == State::kConnecting) {
                    emit connectionStateChanged(ConnectionState::Connecting);
                } else if (state == State::kNew) {
                    emit connectionStateChanged(ConnectionState::Checking);
                } else {
                    emit connectionStateChanged(ConnectionState::Disconnect);
                    emit disconnected();
                }
            }, Qt::QueuedConnection);

    webrtc::PeerConnectionDependencies dependencies(d->m_peerObserver.get());

    auto result = d->m_factory->CreatePeerConnectionOrError(
        config, std::move(dependencies));

    if (!result.ok()) {
        LOG_DEBUG(QStringLiteral("PeerConnection error: %1")
                      .arg(QString::fromUtf8(result.error().message())));
        return false;
    }

    d->m_peerConnection = result.MoveValue();

    return true;
}

void WebrtcClient::createOffer(SdpCompletionHandler onSuccess)
{
    if (!d->m_factory || !d->m_peerConnection) {
        LOG_WARNING("Cannot create SDP before creating the peer connection");
        return;
    }

    auto addTrackResult = d->m_peerConnection->AddTrack(
        d->m_localAudioTrack, {"stream_id"});

    if (!addTrackResult.ok()) {
        LOG_WARNING(QStringLiteral("Failed to add audio track: %1")
                        .arg(QString::fromUtf8(addTrackResult.error().message())));
        return;
    }

    createLocalCameraTrack();

    const QPointer<WebrtcClient> self(this);
    d->m_sdpObserver = webrtc::make_ref_counted<core::rtc::SdpObserver>(
        [self, onSuccess = std::move(onSuccess)](std::unique_ptr<webrtc::SessionDescriptionInterface> description) {
            if (!self || !self->d->m_peerConnection || !description) {
                return;
            }

            std::string sdp;
            if (!description->ToString(&sdp)) {
                LOG_WARNING("Failed to serialize the local SDP offer");
                return;
            }

            const QString type = QString::fromStdString(description->type());
            const QString offer = QString::fromStdString(sdp);
            auto setObserver =
                webrtc::make_ref_counted<SetLocalDescriptionObserver>(
                    [self, type, offer, onSuccess](webrtc::RTCError error) {
                        if (!self) {
                            return;
                        }
                        if (!error.ok()) {
                            LOG_WARNING(QStringLiteral(
                                            "Failed to set local description: %1")
                                            .arg(QString::fromUtf8(
                                                error.message())));
                            return;
                        }

                        QMetaObject::invokeMethod(self.data(),
                            [self, type, offer, onSuccess]() {
                                if (!self) {
                                    return;
                                }
                                emit self->localSdpCreated(type, offer);
                                if (self && onSuccess) {
                                    onSuccess(offer);
                                }
                            }, Qt::QueuedConnection);
                    });

            LOG_INFO(QStringLiteral ("SDP Offer: %1").arg (offer));
            self->d->m_peerConnection->SetLocalDescription(
                std::move(description), std::move(setObserver));
        },
        [](const webrtc::RTCError &error) {
            LOG_WARNING(QStringLiteral("Failed to create SDP offer: %1")
                            .arg(QString::fromUtf8(error.message())));
        });

    d->m_peerConnection->CreateOffer(
        d->m_sdpObserver.get(),
        webrtc::PeerConnectionInterface::RTCOfferAnswerOptions{});
}

void WebrtcClient::createAnswer(SdpCompletionHandler onSuccess)
{
    if (!d->m_factory || !d->m_peerConnection) {
        LOG_WARNING("Cannot create SDP before creating the peer connection");
        return;
    }

    auto addTrackResult = d->m_peerConnection->AddTrack(
        d->m_localAudioTrack, {"stream_id"});

    if (!addTrackResult.ok()) {
        LOG_WARNING(QStringLiteral("Failed to add audio track: %1")
                        .arg(QString::fromUtf8(addTrackResult.error().message())));
        return;
    }

    createLocalCameraTrack();

    const QPointer<WebrtcClient> self(this);
    d->m_sdpObserver = webrtc::make_ref_counted<core::rtc::SdpObserver>(
        [self, onSuccess = std::move(onSuccess)](std::unique_ptr<webrtc::SessionDescriptionInterface> description) {
            if (!self || !self->d->m_peerConnection || !description) {
                return;
            }

            std::string sdp;
            if (!description->ToString(&sdp)) {
                LOG_WARNING("Failed to serialize the local SDP answer");
                return;
            }

            const QString type = QString::fromStdString(description->type());
            const QString answer = QString::fromStdString(sdp);
            auto setObserver =
                webrtc::make_ref_counted<SetLocalDescriptionObserver>(
                    [self, type, answer, onSuccess](webrtc::RTCError error) {
                        if (!self) {
                            return;
                        }
                        if (!error.ok()) {
                            LOG_WARNING(QStringLiteral(
                                            "Failed to set local description: %1")
                                            .arg(QString::fromUtf8(
                                                error.message())));
                            return;
                        }

                        QMetaObject::invokeMethod(self.data(),
                            [self, type, answer, onSuccess]() {
                                if (!self) {
                                    return;
                                }
                                emit self->localSdpCreated(type, answer);
                                if (self && onSuccess) {
                                    onSuccess(answer);
                                }
                            }, Qt::QueuedConnection);
                    });

            LOG_INFO(QStringLiteral ("SDP Answer: %1").arg (answer));
            self->d->m_peerConnection->SetLocalDescription(
                std::move(description), std::move(setObserver));
        },
        [](const webrtc::RTCError &error) {
            LOG_WARNING(QStringLiteral("Failed to create SDP Answer: %1")
                            .arg(QString::fromUtf8(error.message())));
        });


    d->m_peerConnection->CreateAnswer (d->m_sdpObserver.get (),
                                   webrtc::PeerConnectionInterface::RTCOfferAnswerOptions{});


}

bool WebrtcClient::setRemoteSdp(const QString &type, const QString &sdp,
                                std::function<void()> onSuccess)
{
    if (!d->m_peerConnection) {
        LOG_WARNING ("PeerConnection is null");
        return false;
    }
    webrtc::SdpParseError parseError;

    auto sdpType = webrtc::SdpTypeFromString (type.toStdString ());
    if (!sdpType) {
        LOG_WARNING("Invalid remote SDP type");
        return false;
    }
    std::unique_ptr<webrtc::SessionDescriptionInterface> description = webrtc::CreateSessionDescription (sdpType.value (), sdp.toStdString (), &parseError);
    if (!description) {
        LOG_WARNING (QStringLiteral ("Failed to parse remote SDP: %1").arg (QString::fromStdString (parseError.description)));
        return false;
    }
    const QPointer<WebrtcClient> self(this);
    d->m_remoteSdpObserver = webrtc::make_ref_counted<core::rtc::SetRemoteDescriptionObserver>(
        [self, onSuccess = std::move(onSuccess)] {
            if (!self) {
                return;
            }
            QMetaObject::invokeMethod(self.data(), [self, onSuccess] {
                if (self && onSuccess) {
                    onSuccess();
                }
            }, Qt::QueuedConnection);
        });

    d->m_peerConnection->SetRemoteDescription (d->m_remoteSdpObserver.get (), description.release ());
    return true;
}

bool WebrtcClient::setRemoteCandidate(const std::string &sdp,
                                      const std::string &sdpMid,
                                      const int sdpMLineIndex)
{
    if (!d->m_peerConnection) {
        LOG_WARNING ("Peerconnection is null");
        return false;
    }
    auto ice_candidate = parseIceCandidate (sdp, sdpMid, sdpMLineIndex);

    d->m_peerConnection->AddIceCandidate (
        std::move (ice_candidate),
        [](webrtc::RTCError error) {
            if (!error.ok ()) {
                LOG_CRITICAL (
                    QStringLiteral ("Add remote ICE Candidate failed: %1")
                        .arg (QString::fromStdString (error.message ()))
                    );
            } else {
                LOG_INFO ("ADD remote ICE Candidate");
            }
            return true;
        });
    return false;
}

bool WebrtcClient::pushVideoFrame(const webrtc::VideoFrame &frame)
{
    if (!d->videoSource) {
        LOG_WARNING("Cannot push a video frame before creating the peer connection");
        return false;
    }

    d->videoSource->pushFrame(frame);
    return true;
}

// MARK: -- Audio session ---
void WebrtcClient::setEnableSpeaker(bool speaker)
{

}

void WebrtcClient::setMuteMicrophone(bool mute)
{
    if (!d->m_localAudioTrack) {
        LOG_WARNING ("Local audio track is null");
        return;
    }

    if (d->m_localAudioTrack->enabled () == mute) {
        return;
    }
    d->m_localAudioTrack->set_enabled (!mute);
    qDebug() << "Micropone:"
             << (mute ? "mute" : "unmute");
}


void WebrtcClient::createLocalAudioTrack()
{
    if (!d->m_factory) {
        LOG_WARNING (QStringLiteral ("PeerConnectionFactory is null"));
        return;
    }
    webrtc::AudioOptions options;
    auto audio_source = d->m_factory->CreateAudioSource (options);
    if (!audio_source) {
        LOG_WARNING (QStringLiteral ("Failed to create audio source"));
        return;
    }

    d->m_localAudioTrack = d->m_factory->CreateAudioTrack ("local_audio", audio_source.get ());
    if (!d->m_localAudioTrack) {
        LOG_WARNING (QStringLiteral ("Failed to create audio track"));
        return;
    }
    LOG_INFO (QStringLiteral ("Local audio track created"));
}

void WebrtcClient::createLocalCameraTrack()
{
    if (!d->cameraRequested || d->m_localVideoTrack) {
        return;
    }
    if (!d->m_factory || !d->m_peerConnection) {
        LOG_WARNING ("PeerConnectionFactory or peer connection is null");
        return;
    }

    d->m_cameraSource = core::rtc::CameraVideoSource::Create (1280, 720, 30);

    if (!d->m_cameraSource) {
        LOG_WARNING ("Failed to create camera source");
        emit errorOccurred(QStringLiteral("No usable local camera is available"));
        return;
    }
    d->m_localVideoTrack = d->m_factory->CreateVideoTrack (d->m_cameraSource, "local_video");
    if (!d->m_localVideoTrack) {
        LOG_WARNING (QStringLiteral ("Failed to create  video track"));
        d->m_cameraSource = nullptr;
        return;
    }

    auto result = d->m_peerConnection->AddTrack (d->m_localVideoTrack, {"local_stream"});

    if (!result.ok ()) {
        LOG_WARNING (QStringLiteral ("Failed to add video track: %1")
                        .arg (QString::fromStdString (result.error ().message ())));
        d->m_localVideoTrack = nullptr;
        d->m_cameraSource = nullptr;
        return;
    }
    d->m_localVideoTrack->set_enabled(false);
    updateCameraCapture();
}

void WebrtcClient::setCameraEnabled(bool enable)
{
    d->cameraRequested = enable;
    updateCameraCapture();
}

std::unique_ptr<webrtc::IceCandidateInterface> WebrtcClient::parseIceCandidate(
    const std::string &sdp,
    const std::string &sdpMid,
    int sdpMLineIndex)
{
    webrtc::SdpParseError error;
    auto ice_candidate = webrtc::IceCandidate::Create (sdpMid, sdpMLineIndex, sdp, &error);
    if (!ice_candidate) {
        LOG_WARNING (QStringLiteral ("Failed to parse ICE candidate: %1").arg (QString::fromStdString (error.description)));
        return nullptr;
    }
    return ice_candidate;
}

void WebrtcClient::updateCameraCapture()
{
    if (!d->m_localVideoTrack || !d->m_cameraSource) {
        return;
    }
    const bool wasEnabled = d->m_cameraSource->isRunning();
    bool enabled = d->cameraRequested && d->peerConnected;
    if (enabled) {
        enabled = d->m_cameraSource->start();
        if (!enabled) {
            emit errorOccurred(QStringLiteral("Unable to start the local camera"));
        }
    } else {
        d->m_cameraSource->stop();
    }
    d->m_localVideoTrack->set_enabled(enabled);
    if (!enabled) {
        emit localVideoFrameReady(QImage());
    }
    if (wasEnabled != enabled) {
        emit cameraEnabledChanged(enabled);
    }
}

} // namespace rtc
} // namespace core

