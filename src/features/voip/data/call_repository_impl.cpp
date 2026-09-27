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

    connect(m_signaling.get(), &voip::signaling::JingleService::signalingFailed,
            this, &domain::CallRepository::signalingFailed);

    connect(m_signaling.get (),
            &voip::signaling::JingleService::proposeReceived,
            this,
            [this](const QString &sender,
                   const QList<voip::signaling::jingle::StreamType> &streams) {
                emit proposeReceived (sender, streams.contains (voip::signaling::jingle::StreamType::Video));
            });
    connect(m_signaling.get (),
            &voip::signaling::JingleService::ringingReceived,
            this,
            &domain::CallRepository::ringingReceived);

    connect(m_signaling.get (),
            &voip::signaling::JingleService::acceptReceived,
            this,
            &domain::CallRepository::acceptReceived);
    connect(m_signaling.get (),
            &voip::signaling::JingleService::rejectReceived,
            this,
            [this](const QString &reject,
                   const QString &mid) {
                emit rejectReceived (reject, mid);
            });
    connect(m_signaling.get (),
            &voip::signaling::JingleService::retractReceived,
            this,
            [this](const QString &retract,
                   const QString &mid) {
                emit retractReceived (retract, mid);
            });
    connect(m_signaling.get (),
            &voip::signaling::JingleService::proceedReceived,
            this,
            [this](const QString &proceed,
                   const QString &mid) {
                emit proceedReceived (proceed, mid);
            });
    connect(m_signaling.get (),
            &voip::signaling::JingleService::finishReceived,
            this,
            [this](const QString &finish,
                   const QString &mid) {
                emit finishReceived (finish, mid);
            });


    connect(m_signaling.get (),
            &voip::signaling::JingleService::sessionTerminate ,
            this,
            &domain::CallRepository::sessionTerminate);

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
    connect(m_signaling.get (),
            &voip::signaling::JingleService::callStateChange,
            this,
            [this](const voip::signaling::CallState &state) {
                switch(state) {
                case voip::signaling::CallState::Calling:
                    qDebug() << "Repo: Calling";
                    break;
                case voip::signaling::CallState::Ringing:
                    qDebug() << "Repo: Ringing";
                    break;
                case voip::signaling::CallState::Connected:
                    qDebug() << "Repo: Connected";
                    break;
                case voip::signaling::CallState::Reconnect:
                    qDebug() << "Repo: Reconnect";
                    break;
                case voip::signaling::CallState::Reject:
                    qDebug() << "Repo: Reject";
                    break;
                case voip::signaling::CallState::HandUp:
                    qDebug() << "Repo: Handup";
                    break;
                case voip::signaling::CallState::Exchange:
                    qDebug() << "Repo: Exchange";
                    break;
                }
                emit callStateChange (state);
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

void data::CallRepositoryImpl::prepare()
{
    m_signaling->prepare ();
}

void data::CallRepositoryImpl::rejectCall()
{
    m_signaling->rejectCall ();
}

void data::CallRepositoryImpl::retractCall()
{

}

void data::CallRepositoryImpl::finishCall()
{

}

void data::CallRepositoryImpl::proceedCall()
{

}

void data::CallRepositoryImpl::setAudioInputDevice(const QAudioDevice &device)
{
    QMetaObject::invokeMethod(m_signaling.get(),
        [service = m_signaling.get(), device]() {
            service->setAudioInputDevice(device);
        }, Qt::QueuedConnection);
}

void data::CallRepositoryImpl::setAudioOutputDevice(const QAudioDevice &device)
{
    QMetaObject::invokeMethod(m_signaling.get(),
        [service = m_signaling.get(), device]() {
            service->setAudioOutputDevice(device);
        }, Qt::QueuedConnection);
}

void data::CallRepositoryImpl::setCameraDevice(const QCameraDevice &device)
{
    QMetaObject::invokeMethod(m_signaling.get(),
        [service = m_signaling.get(), device]() {
            service->setCameraDevice(device);
        }, Qt::QueuedConnection);
}
