#include "call_vm.hpp"

#include <QCoreApplication>
#include <QPermission>
#include "domain/usecase/call_use_case.hpp"

CallVM *CallVM::s_instance = nullptr;

CallVM::CallVM(
    std::shared_ptr<domain::CallUseCase> usecase,
    QObject *parent)
    : m_callUseCase(std::move (usecase)),
    QObject(parent)
{
    connect(m_callUseCase.get(), &domain::CallUseCase::localVideoFrameReady,
            this, [this](const QImage &image) {
                if (m_acceptRemoteVideo) {
                    m_localVideoFrame = image;
                    emit localVideoFrameChanged();
                }
            });
    connect(m_callUseCase.get(), &domain::CallUseCase::remoteVideoFrameReady,
            this, [this](const QImage &image) {
                if (m_acceptRemoteVideo) {
                    m_remoteVideoFrame = image;
                    emit remoteVideoFrameChanged();
                }
            });
    connect(m_callUseCase.get(), &domain::CallUseCase::signalingFailed,
            this, &CallVM::callFailed);
    connect(m_callUseCase.get(), &domain::CallUseCase::proposeReceived,
            this, [this](const QString &sender, bool video) {
                m_incomingVideo = video;
                emit proposeReceived(sender, video);
            });
    connect(m_callUseCase.get(), &domain::CallUseCase::jingleMessageReceived,
            this, &CallVM::jingleMessageReceived);
    connect (m_callUseCase.get (),
            &domain::CallUseCase::connectionStateChanged,
            this,
            [this](const QString &state) {
                setConnectionState (state);
                emit connectionStateChanged ();
            });
}

CallVM *CallVM::create(QQmlEngine *engine, QJSEngine *scriptEngine)
{
    Q_UNUSED(scriptEngine);
    Q_ASSERT(s_instance);
    Q_ASSERT(s_instance->thread() == engine->thread());

    QQmlEngine::setObjectOwnership(s_instance, QQmlEngine::CppOwnership);
    return s_instance;
}

void CallVM::setInstance(CallVM *instance)
{
    Q_ASSERT(instance);
    s_instance = instance;
}


void CallVM::startCall(bool video)
{
    if (!m_callUseCase || m_receiverId.trimmed().isEmpty()) {
        emit callFailed(QStringLiteral("Cannot start call: no recipient or call service"));
        return;
    }
    const QString receiver = m_receiverId;
    withCallPermissions(video, [this, receiver, video] {
        m_acceptRemoteVideo = true;
        m_callUseCase->execute(receiver, video);
    });
}



void CallVM::requestCall(const QString &receiverId, const QString &userName)
{
    if (receiverId.trimmed().isEmpty()) {
        return;
    }

    ++m_permissionRequest;
    m_receiverId = receiverId;
    m_userName = userName;
    emit callWindowRequested(m_receiverId, m_userName);
}

void CallVM::declineCall()
{
    ++m_permissionRequest;
    m_callUseCase->declineCall ();
}

void CallVM::reset()
{
    m_acceptRemoteVideo = false;
    m_localVideoFrame = QImage();
    emit localVideoFrameChanged();
    m_remoteVideoFrame = QImage();
    emit remoteVideoFrameChanged();
    ++m_permissionRequest;
    m_callUseCase->endCall();
    m_incomingVideo = false;
    m_receiverId.clear();
    m_userName.clear();
}

void CallVM::accept()
{
    if (!m_callUseCase) {
        emit callFailed(QStringLiteral("Cannot accept call: no call service"));
        return;
    }
    withCallPermissions(m_incomingVideo, [this] {
        m_acceptRemoteVideo = true;
        m_callUseCase->acceptCall();
    });
}



void CallVM::withMicrophonePermission(std::function<void()> onGranted)
{
    const auto request = ++m_permissionRequest;
    const QMicrophonePermission permission;
    switch (qApp->checkPermission(permission)) {
    case Qt::PermissionStatus::Granted:
        onGranted();
        return;
    case Qt::PermissionStatus::Denied:
        emit callFailed(QStringLiteral("Microphone access is denied. Allow microphone access in system settings to make calls."));
        return;
    case Qt::PermissionStatus::Undetermined:
        qApp->requestPermission(permission, this,
            [this, request, onGranted = std::move(onGranted)](const QPermission &result) {
                if (request != m_permissionRequest) {
                    return;
                }
                if (result.status() == Qt::PermissionStatus::Granted) {
                    onGranted();
                } else {
                    emit callFailed(QStringLiteral("Microphone access is required for calls."));
                }
            });
        return;
    }
}

void CallVM::withCallPermissions(bool video, std::function<void()> onGranted)
{
    withMicrophonePermission([this, video, onGranted = std::move(onGranted)] {
        if (!video) {
            onGranted();
            return;
        }
        const auto request = m_permissionRequest;
        const QCameraPermission permission;
        switch (qApp->checkPermission(permission)) {
        case Qt::PermissionStatus::Granted:
            onGranted();
            return;
        case Qt::PermissionStatus::Denied:
            emit callFailed(QStringLiteral("Camera access is denied. Allow camera access in system settings to make video calls."));
            return;
        case Qt::PermissionStatus::Undetermined:
            qApp->requestPermission(permission, this,
                [this, request, onGranted](const QPermission &result) {
                    if (request != m_permissionRequest) {
                        return;
                    }
                    if (result.status() == Qt::PermissionStatus::Granted) {
                        onGranted();
                    } else {
                        emit callFailed(QStringLiteral("Camera access is required for video calls."));
                    }
                });
            return;
        }
    });
}

QString CallVM::connectionState() const
{
    return m_connectionState;
}

void CallVM::endCall()
{
    m_connectionState = "";

}
void CallVM::setConnectionState(const QString &state)
{
    if (state == m_connectionState) {
        return;
    }
    m_connectionState = state;
}

