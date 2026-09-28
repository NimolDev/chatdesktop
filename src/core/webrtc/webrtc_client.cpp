#include "webrtc_client.hpp"
#include "peer_connection_observer.hpp"
#include "sdp_observer.hpp"
#include "set_remote_description_observer.hpp"
#include "logger.hpp"
#include "qt_camera_frame.hpp"
#include "qt_camera_capture.hpp"
#include <QThread>
#include <QElapsedTimer>
#include <QVideoFrame>
#include <rtc_base/time_utils.h>

#include <QMetaObject>
#include <QPointer>
#include <QTimer>

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

    bool resumeRecording = false;
    bool resumePlayout = false;
    QAudioDevice audioInput;
    QAudioDevice audioOutput;
    QCameraDevice cameraDevice;
    bool cameraRequested = false;
    bool peerConnected = false;
    webrtc::AudioOptions audioOptions;

    std::unique_ptr<webrtc::Thread> networkThread;
    std::unique_ptr<webrtc::Thread> workerThread;
    std::unique_ptr<webrtc::Thread> signalingThread;

    webrtc::scoped_refptr<webrtc::AudioDeviceModule> m_audioDevice;
    webrtc::scoped_refptr<webrtc::AudioTrackInterface> m_localAudioTrack;
    // webrtc::scoped_refptr<webrtc::VideoTrackInterface> m_localVideoTrack;

    webrtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> m_factory;
    webrtc::scoped_refptr<webrtc::PeerConnectionInterface> m_peerConnection;

    std::unique_ptr<core::rtc::PeerConnectionObserver> m_peerObserver;
    webrtc::scoped_refptr<core::rtc::SdpObserver> m_sdpObserver;
    webrtc::scoped_refptr<core::rtc::SetRemoteDescriptionObserver> m_remoteSdpObserver;

    webrtc::scoped_refptr<LocalVideoTrackSource> m_cameraSource;
    QThread *captureThread = nullptr;
    QtCameraCapture *capture = nullptr;
    webrtc::scoped_refptr<webrtc::VideoTrackInterface> m_localVideoTrack;

};

WebrtcClient::WebrtcClient(QObject *parent)
    : QObject(parent),
      d(std::make_unique<WebRtcClientPrivate>())
{
    auto *videoTimer = new QTimer(this);
    videoTimer->setInterval(33);
    connect(videoTimer, &QTimer::timeout, this, [this] {
        QVideoFrame image;
        if (d->peerConnected && d->m_peerObserver
            && d->m_peerObserver->takeRemoteVideoFrame(image)) {
            emit remoteVideoFrameReady(image);
        }
        if (d->cameraRequested && d->capture && d->m_cameraSource
            && d->m_localVideoTrack && d->m_localVideoTrack->enabled()) {
            const auto frame = d->capture->takeFrame();
            const auto buffer = cameraFrameToI420(frame);
            if (buffer) {
                d->m_cameraSource->pushFrame(webrtc::VideoFrame::Builder()
                    .set_video_frame_buffer(buffer)
                    .set_timestamp_us(webrtc::TimeMicros())
                    .set_rotation(static_cast<webrtc::VideoRotation>(frame.rotation()))
                    .build());
            }
        }
    });
    videoTimer->start();
}

WebrtcClient::~WebrtcClient()
{
    d->cameraRequested = false;
    if (d->captureThread) {
        disconnect(d->capture, nullptr, this, nullptr);
        QMetaObject::invokeMethod(d->capture, &QtCameraCapture::stop, Qt::BlockingQueuedConnection);
        d->captureThread->quit();
        d->captureThread->wait();
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
    d->m_localAudioTrack = nullptr;
    d->m_factory = nullptr;
    d->m_audioDevice = nullptr;
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


}

bool WebrtcClient::initialize()
{
    QElapsedTimer startup;
    startup.start();
    configureAudioSession();
    LOG_INFO(QStringLiteral("WebRTC audio setup: %1 ms").arg(startup.elapsed()));
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

    d->workerThread->BlockingCall([this] {
        d->m_audioDevice = webrtc::CreateAudioDeviceModule(
            webrtc::CreateEnvironment(), webrtc::AudioDeviceModule::kPlatformDefaultAudio);
    });
    if (!d->m_audioDevice) {
        emit errorOccurred(QStringLiteral("Cannot initialize audio devices"));
        return false;
    }

    d->m_factory = webrtc::CreatePeerConnectionFactory(
        d->networkThread.get(),
        d->workerThread.get(),
        d->signalingThread.get(),
        d->m_audioDevice,
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

    applyAudioDevice(d->audioInput, true);
    applyAudioDevice(d->audioOutput, false);
    createLocalAudioTrack ();
    LOG_INFO(QStringLiteral("WebRTC initialization complete: %1 ms").arg(startup.elapsed()));
    return true;
}
void WebrtcClient::prepare()
{
    if (!d->capture) {
        d->captureThread = new QThread(this);
        d->captureThread->setObjectName(QStringLiteral("QtCameraCapture"));
        d->capture = new QtCameraCapture;
        d->capture->moveToThread(d->captureThread);
        connect(d->captureThread, &QThread::finished, d->capture, &QObject::deleteLater);
        // Forward only the signal directly. No WebRTC state is accessed by the
        // camera thread, and the GUI receiver uses its normal queued delivery.
        connect(d->capture, &QtCameraCapture::frameReady,
                this, &WebrtcClient::localVideoFrameReady, Qt::DirectConnection);
        connect(d->capture, &QtCameraCapture::errorOccurred,
                this, &WebrtcClient::errorOccurred);
        connect(d->capture, &QtCameraCapture::activeChanged,
                this, &WebrtcClient::cameraEnabledChanged);
        d->captureThread->start();
    }
    const auto device = d->cameraDevice;
    QMetaObject::invokeMethod(d->capture, [capture = d->capture, device]() {
        capture->setDevice(device);
        capture->prepare();
    }, Qt::QueuedConnection);
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
            d->m_peerObserver.get(), [this](const QString &message) {
                emit errorOccurred(message);
            }, Qt::QueuedConnection);
    connect(d->m_peerObserver.get(),
            &PeerConnectionObserver::iceCandidateChanged,
            d->m_peerObserver.get(),
            [this](const QString &candidate, const QString &sdpMid, int sdpMLineIndex) {
                emit localIceCandidateGenerated(candidate, sdpMid, sdpMLineIndex);
            });

    connect(d->m_peerObserver.get(), &PeerConnectionObserver::connectionStateChanged,
            d->m_peerObserver.get(), [this](webrtc::PeerConnectionInterface::PeerConnectionState state) {
                using State = webrtc::PeerConnectionInterface::PeerConnectionState;
                d->peerConnected = state == State::kConnected;
                if (!d->peerConnected) {
                    emit remoteVideoFrameReady(QVideoFrame());
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
    const QPointer<PeerConnectionObserver> connection(d->m_peerObserver.get());
    d->m_sdpObserver = webrtc::make_ref_counted<core::rtc::SdpObserver>(
        [self, connection, onSuccess = std::move(onSuccess)](std::unique_ptr<webrtc::SessionDescriptionInterface> description) {
            if (!self || !connection || !self->d->m_peerConnection || !description) {
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
                    [self, connection, type, offer, onSuccess](webrtc::RTCError error) {
                        if (!self || !connection) {
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
                            [self, connection, type, offer, onSuccess]() {
                                if (!self || !connection) {
                                    return;
                                }
                                emit self->localSdpCreated(type, offer);
                                if (self && connection && onSuccess) {
                                    onSuccess(offer);
                                }
                            }, Qt::QueuedConnection);
                    });

            // LOG_INFO(QStringLiteral ("SDP Offer: %1").arg (offer));
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
    const QPointer<PeerConnectionObserver> connection(d->m_peerObserver.get());
    d->m_sdpObserver = webrtc::make_ref_counted<core::rtc::SdpObserver>(
        [self, connection, onSuccess = std::move(onSuccess)](std::unique_ptr<webrtc::SessionDescriptionInterface> description) {
            if (!self || !connection || !self->d->m_peerConnection || !description) {
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
                    [self, connection, type, answer, onSuccess](webrtc::RTCError error) {
                        if (!self || !connection) {
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
                            [self, connection, type, answer, onSuccess]() {
                                if (!self || !connection) {
                                    return;
                                }
                                emit self->localSdpCreated(type, answer);
                                if (self && connection && onSuccess) {
                                    onSuccess(answer);
                                }
                            }, Qt::QueuedConnection);
                    });

            // LOG_INFO(QStringLiteral ("SDP Answer: %1").arg (answer));
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
    const QPointer<PeerConnectionObserver> connection(d->m_peerObserver.get());
    d->m_remoteSdpObserver = webrtc::make_ref_counted<core::rtc::SetRemoteDescriptionObserver>(
        [self, connection, onSuccess = std::move(onSuccess)] {
            if (!self || !connection) {
                return;
            }
            QMetaObject::invokeMethod(self.data(), [self, connection, onSuccess] {
                if (self && connection && onSuccess) {
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
            }
            return true;
        });
    return false;
}


void WebrtcClient::closeConnection()
{
    const bool hadConnection = bool(d->m_peerConnection);
    d->peerConnected = false;
    d->cameraRequested = false;

    updateCameraCapture();
    if (d->m_peerObserver) {
        QObject::disconnect(d->m_peerObserver.get(), nullptr, nullptr, nullptr);
    }
    if (d->m_peerConnection) {
        d->m_peerConnection->Close();
    }
    if (d->m_peerObserver && d->signalingThread) {
        d->signalingThread->BlockingCall([this] {
            d->m_peerObserver->detachRemoteVideo();
        });
    }

    // Keep the observer alive until WebRTC has released the connection.
    d->m_peerConnection = nullptr;
    d->m_peerObserver.reset();
    d->m_sdpObserver = nullptr;
    d->m_remoteSdpObserver = nullptr;
    d->m_localVideoTrack = nullptr;
    d->m_cameraSource = nullptr;

    emit localVideoFrameReady(QVideoFrame());
    emit remoteVideoFrameReady(QVideoFrame());
    if (hadConnection) {
        emit connectionStateChanged(ConnectionState::Disconnect);
        emit disconnected();
    }
}

// MARK: -- Audio session ---
void WebrtcClient::setAudioInputDevice(const QAudioDevice &device)
{
    d->audioInput = device;
    applyAudioDevice(device, true);
}

void WebrtcClient::setAudioOutputDevice(const QAudioDevice &device)
{
    d->audioOutput = device;
    applyAudioDevice(device, false);
}

void WebrtcClient::setCameraDevice(const QCameraDevice &device)
{
    d->cameraDevice = device;
    if (d->capture) {
        QMetaObject::invokeMethod(d->capture, [capture = d->capture, device] {
            capture->setDevice(device);
        }, Qt::QueuedConnection);
    }
}

void WebrtcClient::applyAudioDevice(const QAudioDevice &device, bool input)
{
    if (!d->m_audioDevice || !d->workerThread) {
        return;
    }

    QString error;
    d->workerThread->BlockingCall([this, device, input, &error] {
        auto *adm = d->m_audioDevice.get();
        bool &resume = input ? d->resumeRecording : d->resumePlayout;
        resume = resume || (input ? adm->Recording() : adm->Playing());

        // Device selection requires stopping the corresponding audio stream.
        if ((input ? adm->StopRecording() : adm->StopPlayout()) != 0) {
            error = QStringLiteral("Cannot stop audio for device selection");
            return;
        }
        // Remember active capture/playout while a device is unplugged.
        if (device.isNull()) {
            return;
        }

        const int count = input ? adm->RecordingDevices() : adm->PlayoutDevices();
        int selected = -1;
        int nameMatch = -1;
        for (int index = 0; index < count; ++index) {
            char name[webrtc::kAdmMaxDeviceNameSize] = {};
            char guid[webrtc::kAdmMaxGuidSize] = {};
            const int result = input
                ? adm->RecordingDeviceName(index, name, guid)
                : adm->PlayoutDeviceName(index, name, guid);
            if (result != 0) {
                continue;
            }
            if (device.id() == QByteArray(guid)) {
                selected = index;
                break;
            }
            if (nameMatch < 0 && device.description() == QString::fromUtf8(name)) {
                nameMatch = index;
            }
        }
        if (selected < 0) {
            selected = nameMatch;
        }
        if (selected < 0) {
            error = QStringLiteral("WebRTC cannot find audio device: %1")
                        .arg(device.description());
            return;
        }
        const auto index = static_cast<uint16_t>(selected);
        if ((input ? adm->SetRecordingDevice(index) : adm->SetPlayoutDevice(index)) != 0) {
            error = QStringLiteral("Cannot select audio device: %1").arg(device.description());
            return;
        }
        if (resume) {
            const int initialized = input ? adm->InitRecording() : adm->InitPlayout();
            if (initialized != 0
                || (input ? adm->StartRecording() : adm->StartPlayout()) != 0) {
                error = QStringLiteral("Cannot restart audio device: %1").arg(device.description());
                return;
            }
            resume = false;
        }
    });
    if (!error.isEmpty()) {
        LOG_WARNING(error);
        emit errorOccurred(error);
    }
}

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
    if (!d->cameraRequested) {
        return;
    }
    if (!d->m_factory) {
        LOG_WARNING("PeerConnectionFactory is null");
        return;
    }

    if (!d->m_localVideoTrack) {
        d->m_cameraSource = webrtc::make_ref_counted<LocalVideoTrackSource>();
        d->m_localVideoTrack = d->m_factory->CreateVideoTrack(d->m_cameraSource, "local_video");
        if (!d->m_localVideoTrack) {
            LOG_WARNING("Failed to create video track");
            d->m_cameraSource = nullptr;
            return;
        }
    }

    // Local capture can run before the recipient accepts and a peer exists.
    updateCameraCapture();
    if (!d->m_peerConnection) {
        return;
    }
    for (const auto &sender : d->m_peerConnection->GetSenders()) {
        if (sender->track().get() == d->m_localVideoTrack.get()) {
            return;
        }
    }
    auto result = d->m_peerConnection->AddTrack(d->m_localVideoTrack, {"local_stream"});
    if (!result.ok()) {
        LOG_WARNING(QStringLiteral("Failed to add video track: %1")
                        .arg(QString::fromStdString(result.error().message())));
        emit errorOccurred(QStringLiteral("Unable to add the local video track"));
    }
}

void WebrtcClient::setCameraEnabled(bool enable)
{
    // Remember the request before the peer connection and video track exist.
    // The track is added when preparing the SDP offer or answer.
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
    if (d->m_localVideoTrack) {
        d->m_localVideoTrack->set_enabled(d->cameraRequested);
    }
    if (d->cameraRequested && !d->capture) {
        prepare();
    }
    if (d->capture) {
        QMetaObject::invokeMethod(d->capture,
            d->cameraRequested ? &QtCameraCapture::start : &QtCameraCapture::stop,
            Qt::QueuedConnection);
    }
}

} // namespace rtc
} // namespace core


