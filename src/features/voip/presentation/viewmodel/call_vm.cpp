
#include "call_vm.hpp"

#include <QCoreApplication>
#include <QPermission>
#include "domain/usecase/call_use_case.hpp"
#include "logging/logger.hpp"

CallVM *CallVM::s_instance = nullptr;

CallVM::CallVM(
    std::shared_ptr<domain::CallUseCase> usecase,
    QObject *parent)
    : QObject(parent)
    , m_mediaDevices(this)
    , m_callUseCase(std::move(usecase))
{
    connect(&m_mediaDevices, &core::media::MediaDeviceManager::selectedAudioInputChanged,
            this, &CallVM::selectedAudioInputChanged);
    connect(&m_mediaDevices, &core::media::MediaDeviceManager::audioInputDeviceChanged,
            this, [this](const QAudioDevice &device) {
                m_callUseCase->setAudioInputDevice(device);
            });
    m_callUseCase->setAudioInputDevice(m_mediaDevices.selectedAudioInputDevice());
    connect(&m_mediaDevices, &core::media::MediaDeviceManager::selectedAudioOutputChanged,
            this, &CallVM::selectedAudioOutputChanged);
    connect(&m_mediaDevices, &core::media::MediaDeviceManager::audioOutputDeviceChanged,
            this, [this](const QAudioDevice &device) {
                m_callUseCase->setAudioOutputDevice(device);
            });
    m_callUseCase->setAudioOutputDevice(m_mediaDevices.selectedAudioOutputDevice());
    connect(&m_mediaDevices, &core::media::MediaDeviceManager::selectedCameraChanged,
            this, &CallVM::selectedCameraChanged);
    connect(&m_mediaDevices, &core::media::MediaDeviceManager::cameraDeviceChanged,
            this, [this](const QCameraDevice &device) {
                m_callUseCase->setCameraDevice(device);
            });
    m_callUseCase->setCameraDevice(m_mediaDevices.selectedCameraDevice());

    connect(m_callUseCase.get(), &domain::CallUseCase::localVideoFrameReady,
            this, [this](const QVideoFrame &frame) {
                if (m_acceptRemoteVideo) {
                    m_localVideoFrame = frame;
                    if (m_localVideoSink) {
                        m_localVideoSink->setVideoFrame(frame);
                    }
                    emit localVideoFrameChanged();
                }
            });
    connect(m_callUseCase.get(), &domain::CallUseCase::remoteVideoFrameReady,
            this, [this](const QVideoFrame &frame) {
                if (m_acceptRemoteVideo) {
                    m_remoteVideoFrame = frame;
                    if (m_remoteVideoSink) {
                        m_remoteVideoSink->setVideoFrame(frame);
                    }
                    emit remoteVideoFrameChanged();
                }
            });
    connect(m_callUseCase.get(), &domain::CallUseCase::signalingFailed,
            this, &CallVM::callFailed);
    connect(m_callUseCase.get(), &domain::CallUseCase::proposeReceived,
            this, [this](const QString &sender, bool video) {
                m_incomingVideo = video;
                prepareIncoming ();
                emit proposeReceived(sender, video);
            });
    // connect(m_callUseCase.get (),
    //         &domain::CallUseCase::)

    connect(m_callUseCase.get(), &domain::CallUseCase::jingleMessageReceived,
            this, &CallVM::jingleMessageReceived);


    connect(m_callUseCase.get (),
            &domain::CallUseCase::sessionTerminate,
            this,
            &CallVM::sessionTerminate);

    connect (m_callUseCase.get (),
            &domain::CallUseCase::connectionStateChanged,
            this,
            [this](const QString &state) {
                // setConnectionState (state);
            });
    connect(m_callUseCase.get (),
            &domain::CallUseCase::callStateChange ,
            this,
            [this](const voip::signaling::CallState &state) {
                switch(state) {
                case voip::signaling::CallState::Calling:
                    setConnectionState ("Calling");
                    break;
                case voip::signaling::CallState::Ringing:
                     setCallState (CallState::Ringing);
                    setConnectionState ("Ringing");
                    break;
                case voip::signaling::CallState::Connected:
                    setConnectionState ("Connected");
                    break;
                case voip::signaling::CallState::Reconnect:
                    setConnectionState ("Reconnect");
                    break;
                case voip::signaling::CallState::Reject:
                    setConnectionState ("Reject");
                    break;
                case voip::signaling::CallState::HandUp:
                    setConnectionState ("Hand up");
                    break;
                case voip::signaling::CallState::Exchange:
                    setConnectionState ("Exchange");
                    break;
                }
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
    if (!m_callUseCase || receiverId.trimmed().isEmpty()) {
        return;
    }

    ++m_permissionRequest;
    m_receiverId = receiverId;
    m_userName = userName;
    LOG_DEBUG ("Request call");
    emit callWindowRequested(m_receiverId, m_userName);
    // Permission callbacks are invalidated by reset() or a newer start request.
    withCallPermissions(true, [this] {
        m_acceptRemoteVideo = true;
        m_callUseCase->prepare();
    });
}

void CallVM::declineCall()
{
    ++m_permissionRequest;
    m_callUseCase->declineCall ();
}

void CallVM::reset()
{
    m_acceptRemoteVideo = false;
    m_localVideoFrame = QVideoFrame();
    if (m_localVideoSink) {
        m_localVideoSink->setVideoFrame({});
    }
    emit localVideoFrameChanged();
    m_remoteVideoFrame = QVideoFrame();
    if (m_remoteVideoSink) {
        m_remoteVideoSink->setVideoFrame({});
    }
    emit remoteVideoFrameChanged();
    ++m_permissionRequest;
    m_callUseCase->endCall();
    if (!m_connectionState.isEmpty()) {
        m_connectionState.clear();
        emit connectionStateChanged();
    }
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

void CallVM::withCallPermissions(
    bool video,
    std::function<void()> onGranted)
{
    auto completion =  std::make_shared<std::function<void()>>(std::move(onGranted));

    withMicrophonePermission(
        [this, video, completion]() {
            // Audio call only needs microphone permission
            if (!video) {
                (*completion)();
                return;
            }

            const auto request = m_permissionRequest;
            const QCameraPermission permission;

            switch (qApp->checkPermission(permission)) {
            case Qt::PermissionStatus::Granted:
                (*completion)();
                return;

            case Qt::PermissionStatus::Denied:
                emit callFailed(QStringLiteral(
                    "Camera access is denied. Allow camera access in "
                    "system settings to make video calls."));
                return;

            case Qt::PermissionStatus::Undetermined:
                qApp->requestPermission(
                    permission,
                    this,
                    [this, request, completion](const QPermission &result) {
                        if (request != m_permissionRequest) {
                            return;
                        }

                        if (result.status() ==
                            Qt::PermissionStatus::Granted) {
                            (*completion)();
                        } else {
                            emit callFailed(QStringLiteral(
                                "Camera access is required for video calls."));
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
    setConnectionState ("");

}

void CallVM::prepareIncoming()
{
    m_callUseCase->prepare ();
}
void CallVM::setConnectionState(const QString &state)
{
    if (state == m_connectionState) {
        return;
    }
    m_connectionState = state;
    emit connectionStateChanged ();
}
CallVM::CallState CallVM::callState() const
{
    return m_currentCallState;
}

void CallVM::setCallState(const CallState &state)
{
    if (state == m_currentCallState) {
        return;
    }
    m_currentCallState = state;
    emit callStateChanged ();
}

void CallVM::setLocalVideoSink(QVideoSink *sink)
{
    if (m_localVideoSink == sink) {
        return;
    }
    if (m_localVideoSink) {
        m_localVideoSink->setVideoFrame({});
    }
    m_localVideoSink = sink;
    if (sink) {
        sink->setVideoFrame(m_localVideoFrame);
    }
    emit localVideoSinkChanged();
}

void CallVM::setRemoteVideoSink(QVideoSink *sink)
{
    if (m_remoteVideoSink == sink) {
        return;
    }
    if (m_remoteVideoSink) {
        m_remoteVideoSink->setVideoFrame({});
    }
    m_remoteVideoSink = sink;
    if (sink) {
        sink->setVideoFrame(m_remoteVideoFrame);
    }
    emit remoteVideoSinkChanged();
}

void CallVM::selectAudioInput(int index)
{
    m_mediaDevices.selectAudioInput(index);
}

void CallVM::selectAudioOutput(int index)
{
    m_mediaDevices.selectAudioOutput(index);
}

void CallVM::selectCamera(int index)
{
    m_mediaDevices.selectCamera(index);
}
