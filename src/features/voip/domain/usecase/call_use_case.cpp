#include "call_use_case.hpp"



domain::CallUseCase::CallUseCase(std::shared_ptr<CallRepository> repository, QObject *parent)
    : m_repository(std::move (repository)),
    QObject(parent)
{
    connect(m_repository.get(), &CallRepository::localVideoFrameReady,
            this, &CallUseCase::localVideoFrameReady);
    connect(m_repository.get(), &CallRepository::remoteVideoFrameReady,
            this, &CallUseCase::remoteVideoFrameReady);
    connect(m_repository.get(), &CallRepository::jingleMessageReceived,
            this, &CallUseCase::jingleMessageReceived);
    connect(m_repository.get(), &CallRepository::signalingFailed,
            this, &CallUseCase::signalingFailed);
    connect(m_repository.get (),
            &CallRepository::proposeReceived,
            this,
            &CallUseCase::proposeReceived);


    connect (m_repository.get (),
            &CallRepository::sessionTerminate,
            this,
            &CallUseCase::sessionTerminate);



    connect(m_repository.get (),
            &CallRepository::connectionStateChanged ,
            this,
            &CallUseCase::connectionStateChanged);
    connect(m_repository.get (),
            &CallRepository::callStateChange,
            this,
            &CallUseCase::callStateChange);
}

void domain::CallUseCase::prepare()
{
    m_repository->prepare ();
}

void domain::CallUseCase::execute(const QString &receiverId, bool video)
{
    m_repository->startCall(receiverId, video);
}

void domain::CallUseCase::declineCall()
{
    m_repository->declineCall ();
}

void domain::CallUseCase::acceptCall()
{
    m_repository->acceptCall ();
}

void domain::CallUseCase::endCall()
{
    m_repository->endCall();
}

void domain::CallUseCase::setAudioInputDevice(const QAudioDevice &device)
{
    m_repository->setAudioInputDevice(device);
}

void domain::CallUseCase::setAudioOutputDevice(const QAudioDevice &device)
{
    m_repository->setAudioOutputDevice(device);
}

void domain::CallUseCase::setCameraDevice(const QCameraDevice &device)
{
    m_repository->setCameraDevice(device);
}
