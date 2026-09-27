#ifndef FEATURES_VOIP_DOMAIN_REPOSITORY_CALL_REPOSITORY_HPP
#define FEATURES_VOIP_DOMAIN_REPOSITORY_CALL_REPOSITORY_HPP

#include "signaling/call_state.hpp"
#include <domain/model/jingle_state.hpp>
#include <QAudioDevice>
#include <QCameraDevice>
#include <QObject>
#include <QVideoFrame>

namespace domain {

class CallRepository : public QObject
{
    Q_OBJECT
public:
    explicit CallRepository(QObject *parent = nullptr) : QObject(parent) {};
    virtual ~CallRepository() = default;
    virtual void prepare() = 0;

    virtual void setAudioInputDevice(const QAudioDevice &device) = 0;
    virtual void setAudioOutputDevice(const QAudioDevice &device) = 0;
    virtual void setCameraDevice(const QCameraDevice &device) = 0;

    virtual void startCall(const QString &receiverId, bool is_video = false) = 0;
    virtual void endCall() = 0;
    virtual void declineCall() = 0;
    virtual void acceptCall() = 0;
    virtual void rejectCall() = 0;
    virtual void retractCall() = 0;
    virtual void finishCall() = 0;
    virtual void proceedCall() = 0;

signals:
    void localVideoFrameReady(const QVideoFrame &frame);
    void remoteVideoFrameReady(const QVideoFrame &frame);
    void jingleMessageReceived(const QString &action, const QString &sender,
                               const QString &sessionId, bool video, const QString &reason);
    void signalingFailed(const QString &reason);

    void connectionStateChanged(const QString &state);
    void callStateChange(const voip::signaling::CallState &state);

    void sessionTerminate();

    // Jingle Message
    void proposeReceived(const QString &sender, bool video);
    void retractReceived(const QString &retract, const QString &mid);
    void ringingReceived(const QString &ringing, const QString &mid);
    void proceedReceived(const QString &proceed, const QString &mid);
    void rejectReceived(const QString &reject, const QString &mid);
    void acceptReceived(const QString &accept, const QString &mid);
    void finishReceived(const QString &finish, const QString &mid);

};

} // namespace domain


#endif // FEATURES_VOIP_DOMAIN_REPOSITORY_CALL_REPOSITORY_HPP
