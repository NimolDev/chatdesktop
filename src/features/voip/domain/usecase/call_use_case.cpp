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
    connect(m_repository.get (),
            &CallRepository::connectionStateChanged ,
            this,
            &CallUseCase::connectionStateChanged);
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
