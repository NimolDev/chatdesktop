#include "webrtc_client.hpp"
#include "logger.hpp"

#include <QPointer>

#include <api/create_peerconnection_factory.h>
#include <api/make_ref_counted.h>
#include <api/set_local_description_observer_interface.h>

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

    std::unique_ptr<webrtc::Thread> networkThread;
    std::unique_ptr<webrtc::Thread> workerThread;
    std::unique_ptr<webrtc::Thread> signalingThread;

    webrtc::scoped_refptr<::webrtc::PeerConnectionFactoryInterface> peerConnectionFactory;
    webrtc::scoped_refptr<LocalVideoTrackSource> videoSource;
};

WebrtcClient::WebrtcClient(QObject *parent)
    : QObject(parent),
      d(std::make_unique<WebRtcClientPrivate>())
{
}

WebrtcClient::~WebrtcClient()
{
    m_peerConnection = nullptr;
    m_factory = nullptr;
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

bool WebrtcClient::initialize()
{
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

    m_factory = webrtc::CreatePeerConnectionFactory(
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

    if (!m_factory) {
        LOG_CRITICAL("Failed to create WebRTC factory");
        return false;
    }

    LOG_INFO("WebRTC PeerConnectionFactory ready");
    return true;
}

bool WebrtcClient::createPeerConnection()
{
    if (!m_factory) {
        LOG_CRITICAL ("PeerConnectionFactory not initialize");
        return false;
    }

    webrtc::PeerConnectionInterface::RTCConfiguration config;
    webrtc::PeerConnectionInterface::IceServer stunServer;

    stunServer.urls.push_back ("stun:stun.l.google.com:19302");

    config.servers.push_back (stunServer);

    m_peerObserver = std::make_unique<core::rtc::PeerConnectionObserver> ();
    webrtc::PeerConnectionDependencies dependencies(m_peerObserver.get());

    auto result = m_factory->CreatePeerConnectionOrError(
        config, std::move(dependencies));

    if (!result.ok()) {
        LOG_DEBUG(QStringLiteral("PeerConnection error: %1")
                      .arg(QString::fromUtf8(result.error().message())));
        return false;
    }

    m_peerConnection = result.MoveValue();


    return true;

}

void WebrtcClient::createOffer()
{
    webrtc::AudioOptions options;
    auto audioSource = m_factory->CreateAudioSource(options);
    auto audioTrack = m_factory->CreateAudioTrack(
        "audio_track", audioSource.get());

    auto addTrackResult = m_peerConnection->AddTrack(
        audioTrack, {"stream_id"});
    if (!addTrackResult.ok()) {
        LOG_WARNING(QStringLiteral("Failed to add audio track: %1")
                        .arg(QString::fromUtf8(addTrackResult.error().message())));
        return;
    }

    d->videoSource = webrtc::make_ref_counted<LocalVideoTrackSource>();
    auto videoTrack = m_factory->CreateVideoTrack(
        d->videoSource, "video_track");
    if (!videoTrack) {
        LOG_WARNING("Failed to create video track");
        return ;
    }

    auto addVideoTrackResult = m_peerConnection->AddTrack(
        videoTrack, {"stream_id"});
    if (!addVideoTrackResult.ok()) {
        LOG_WARNING(QStringLiteral("Failed to add video track: %1")
                        .arg(QString::fromUtf8(
                            addVideoTrackResult.error().message())));
        return ;
    }

    const QPointer<WebrtcClient> self(this);
    m_sdpObserver = webrtc::make_ref_counted<core::rtc::SdpObserver>(
        [self](std::unique_ptr<webrtc::SessionDescriptionInterface> description) {
            if (!self || !self->m_peerConnection || !description) {
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
                    [self, type, offer](webrtc::RTCError error) {
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

                        emit self->localSdpCreated(type, offer);
                    });

            self->m_peerConnection->SetLocalDescription(
                std::move(description), std::move(setObserver));
        },
        [](const webrtc::RTCError &error) {
            LOG_WARNING(QStringLiteral("Failed to create SDP offer: %1")
                            .arg(QString::fromUtf8(error.message())));
        });

    m_peerConnection->CreateOffer(
        m_sdpObserver.get(),
        webrtc::PeerConnectionInterface::RTCOfferAnswerOptions{});
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









} // namespace rtc
} // namespace core
