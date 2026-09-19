#ifndef FEATURES_VOIP_PRESENTATION_VIEWMODEL_CALL_VM_HPP
#define FEATURES_VOIP_PRESENTATION_VIEWMODEL_CALL_VM_HPP


#include "domain/usecase/call_use_case.hpp"
#include <QObject>
#include <QQmlEngine>
#include <QtQml/qqmlregistration.h>

#include <QString>
#include <functional>


class CallVM : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QImage localVideoFrame READ localVideoFrame NOTIFY localVideoFrameChanged)
    Q_PROPERTY(bool hasLocalVideo READ hasLocalVideo NOTIFY localVideoFrameChanged)
    Q_PROPERTY(QImage remoteVideoFrame READ remoteVideoFrame NOTIFY remoteVideoFrameChanged)
    Q_PROPERTY(bool hasRemoteVideo READ hasRemoteVideo NOTIFY remoteVideoFrameChanged)
    Q_PROPERTY(QString connectionState READ connectionState WRITE setConnectionState NOTIFY connectionStateChanged FINAL)
public:
    // Require an explicit parent so QML uses the singleton factory.
    explicit CallVM(std::shared_ptr<domain::CallUseCase> usecase,
                    QObject *parent = nullptr
                    );

    static CallVM *create(QQmlEngine *engine, QJSEngine *scriptEngine);
    static void setInstance(CallVM *instance);



    QImage localVideoFrame() const { return m_localVideoFrame; }
    bool hasLocalVideo() const { return !m_localVideoFrame.isNull(); }

    QImage remoteVideoFrame() const { return m_remoteVideoFrame; }
    bool hasRemoteVideo() const { return !m_remoteVideoFrame.isNull(); }

    QString receiverId() const { return m_receiverId; }
    QString userName() const { return m_userName; }

    QString connectionState() const;

public:
    // Q_INVOKABLE void startCall(bool video);
    Q_INVOKABLE void endCall();

public slots:
    void startCall(bool video = false);
    void requestCall(const QString &receiverId, const QString &userName);
    void declineCall();
    void reset();
    void accept();

signals:
    void localVideoFrameChanged();
    void remoteVideoFrameChanged();
    void jingleMessageReceived(const QString &action, const QString &sender,
                               const QString &sessionId, bool video, const QString &reason);
    void callFailed(const QString &reason);
    void callWindowRequested(const QString &receiverId, const QString &userName);
    void proposeReceived(const QString &sender,  bool video);

    void connectionStateChanged();

private:
    void setConnectionState(const QString &state);

private:
    QImage m_localVideoFrame;
    QImage m_remoteVideoFrame;
    bool m_acceptRemoteVideo = false;
    void withCallPermissions(bool video, std::function<void()> onGranted);
    bool m_incomingVideo = false;
    void withMicrophonePermission(std::function<void()> onGranted);
    quint64 m_permissionRequest = 0;

    static CallVM *s_instance;
    std::shared_ptr<domain::CallUseCase> m_callUseCase;
    QString m_receiverId;
    QString m_userName;
    QString m_connectionState;
};

#endif // FEATURES_VOIP_PRESENTATION_VIEWMODEL_CALL_VM_HPP
