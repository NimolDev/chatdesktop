#include "call_repository_impl.hpp"


data::CallRepositoryImpl::CallRepositoryImpl(
    std::shared_ptr<voip::signaling::JingleService> signaling,
    QObject *parent)
    : m_signaling(std::move(signaling)),
    m_webrtc(std::make_unique<core::rtc::WebrtcClient> ()),
    domain::CallRepository(parent)
{
    connect(m_signaling.get(), &voip::signaling::JingleService::localVideoFrameReady,
            this, &domain::CallRepository::localVideoFrameReady);
    connect(m_signaling.get(), &voip::signaling::JingleService::remoteVideoFrameReady,
            this, &domain::CallRepository::remoteVideoFrameReady);
    connect(m_signaling.get(), &voip::signaling::JingleService::messageReceived,
            this, [this](const QString &action, const QString &sender,
                         const QString &sessionId,
                         const QList<voip::signaling::jingle::StreamType> &streams,
                         const QString &reason) {
                emit jingleMessageReceived(action, sender, sessionId,
                    streams.contains(voip::signaling::jingle::StreamType::Video), reason);
            });
    connect(m_signaling.get(), &voip::signaling::JingleService::signalingFailed,
            this, &domain::CallRepository::signalingFailed);

    connect(m_signaling.get (),
            &voip::signaling::JingleService::proposalReceived,
            this,
            [this](const QString &sender,
                   const QList<voip::signaling::jingle::StreamType> &streams) {
                emit proposeReceived (sender, streams.contains (voip::signaling::jingle::StreamType::Video));
            });

    connect(m_signaling.get (),
            &voip::signaling::JingleService::connectionStateChange,
            this,
            [this](const core::rtc::ConnectionState &state) {
                switch(state) {
                case core::rtc::ConnectionState::Checking:
                    emit connectionStateChanged ("Checking");
                    break;
                case core::rtc::ConnectionState::Connecting:
                    emit connectionStateChanged ("Connecting");
                    break;
                case core::rtc::ConnectionState::Connected:
                    emit connectionStateChanged ("Connected");
                    break;
                case core::rtc::ConnectionState::Disconnect:
                    emit connectionStateChanged ("Disconnected");
                    break;
                }
            });
}

void data::CallRepositoryImpl::startCall(const QString &receiverId, bool is_video)
{
    m_signaling->startCall(receiverId, is_video);
}

void data::CallRepositoryImpl::endCall()
{
    m_signaling->endCall();
}

void data::CallRepositoryImpl::declineCall()
{
    m_signaling->rejectCall ();
}

void data::CallRepositoryImpl::acceptCall()
{
    m_signaling->acceptCall ();
}
