#ifndef FEATURES_VOIP_PRESENTATION_VIEWMODEL_CALL_VM_HPP
#define FEATURES_VOIP_PRESENTATION_VIEWMODEL_CALL_VM_HPP


#include "domain/usecase/call_use_case.hpp"
#include "media/media_device_manager.hpp"
#include <QObject>
#include <QVideoSink>
#include <QPointer>
#include <QQmlEngine>
#include <QtQml/qqmlregistration.h>

#include <QString>
#include <functional>


class CallVM : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QAbstractItemModel *audioInputs READ audioInputs CONSTANT)
    Q_PROPERTY(int selectedAudioInputIndex READ selectedAudioInputIndex NOTIFY selectedAudioInputChanged)
    Q_PROPERTY(QAbstractItemModel *audioOutputs READ audioOutputs CONSTANT)
    Q_PROPERTY(int selectedAudioOutputIndex READ selectedAudioOutputIndex NOTIFY selectedAudioOutputChanged)
    Q_PROPERTY(QAbstractItemModel *cameras READ cameras CONSTANT)
    Q_PROPERTY(int selectedCameraIndex READ selectedCameraIndex NOTIFY selectedCameraChanged)

    Q_PROPERTY(QVideoFrame localVideoFrame READ localVideoFrame NOTIFY localVideoFrameChanged);
    Q_PROPERTY(QVideoSink *localVideoSink READ localVideoSink WRITE setLocalVideoSink NOTIFY localVideoSinkChanged)
    Q_PROPERTY(bool hasLocalVideo READ hasLocalVideo NOTIFY localVideoFrameChanged);
    Q_PROPERTY(QVideoFrame remoteVideoFrame READ remoteVideoFrame NOTIFY remoteVideoFrameChanged);
    Q_PROPERTY(QVideoSink *remoteVideoSink READ remoteVideoSink WRITE setRemoteVideoSink NOTIFY remoteVideoSinkChanged)
    Q_PROPERTY(bool hasRemoteVideo READ hasRemoteVideo NOTIFY remoteVideoFrameChanged);
    Q_PROPERTY(QString connectionState READ connectionState WRITE setConnectionState NOTIFY connectionStateChanged FINAL);

public:
    enum class CallState
    {
        Idle,
        Calling,
        Ringing,
        Connected,
        Reconnect,
        Reject,
        HandUp,
        Exchange,

    };
    Q_ENUM (CallState);
    Q_PROPERTY(CallState callState READ callState WRITE setCallState NOTIFY callStateChanged FINAL);
public:
    // Require an explicit parent so QML uses the singleton factory.
    explicit CallVM(std::shared_ptr<domain::CallUseCase> usecase,
                    QObject *parent = nullptr
                    );

    static CallVM *create(QQmlEngine *engine, QJSEngine *scriptEngine);
    static void setInstance(CallVM *instance);

    QAbstractItemModel *audioInputs() { return m_mediaDevices.audioInputs(); }
    int selectedAudioInputIndex() const { return m_mediaDevices.selectedAudioInputIndex(); }
    QAbstractItemModel *audioOutputs() { return m_mediaDevices.audioOutputs(); }
    int selectedAudioOutputIndex() const { return m_mediaDevices.selectedAudioOutputIndex(); }
    QAbstractItemModel *cameras() { return m_mediaDevices.cameras(); }
    int selectedCameraIndex() const { return m_mediaDevices.selectedCameraIndex(); }
    Q_INVOKABLE void selectAudioInput(int index);
    Q_INVOKABLE void selectAudioOutput(int index);
    Q_INVOKABLE void selectCamera(int index);




    QVideoFrame localVideoFrame() const { return m_localVideoFrame; }
    bool hasLocalVideo() const { return m_localVideoFrame.isValid(); }

    QVideoFrame remoteVideoFrame() const { return m_remoteVideoFrame; }
    bool hasRemoteVideo() const { return m_remoteVideoFrame.isValid(); }

    QVideoSink *localVideoSink() const { return m_localVideoSink; }
    void setLocalVideoSink(QVideoSink *sink);

    QVideoSink *remoteVideoSink() const { return m_remoteVideoSink; }
    void setRemoteVideoSink(QVideoSink *sink);

    QString receiverId() const { return m_receiverId; }
    QString userName() const { return m_userName; }

    QString connectionState() const;

    CallState callState() const;


public:

    // Q_INVOKABLE void startCall(bool video);
    Q_INVOKABLE void endCall();

public slots:
    void prepareIncoming();
    void startCall(bool video = false);
    void requestCall(const QString &receiverId, const QString &userName);
    void declineCall();
    void reset();
    void accept();

signals:
    void selectedAudioInputChanged();
    void selectedAudioOutputChanged();
    void selectedCameraChanged();
    void localVideoFrameChanged();
    void localVideoSinkChanged();
    void remoteVideoFrameChanged();
    void remoteVideoSinkChanged();
    void jingleMessageReceived(const QString &action, const QString &sender,
                               const QString &sessionId, bool video, const QString &reason);
    void callFailed(const QString &reason);
    void callWindowRequested(const QString &receiverId, const QString &userName);
    void proposeReceived(const QString &sender,  bool video);

    void sessionTerminate();

    void connectionStateChanged();
    void callStateChanged();

private:
    void setConnectionState(const QString &state);
    void setCallState(const CallState &state);

private:
    QVideoFrame m_localVideoFrame;
    QPointer<QVideoSink> m_localVideoSink;
    QVideoFrame m_remoteVideoFrame;
    QPointer<QVideoSink> m_remoteVideoSink;
    bool m_acceptRemoteVideo = false;
    void withCallPermissions(bool video, std::function<void()> onGranted);
    bool m_incomingVideo = false;
    void withMicrophonePermission(std::function<void()> onGranted);
    quint64 m_permissionRequest = 0;

    core::media::MediaDeviceManager m_mediaDevices;
    static CallVM *s_instance;
    std::shared_ptr<domain::CallUseCase> m_callUseCase;
    QString m_receiverId;
    QString m_userName;
    QString m_connectionState;
    CallState m_currentCallState = CallState::Idle;
};

#endif // FEATURES_VOIP_PRESENTATION_VIEWMODEL_CALL_VM_HPP
