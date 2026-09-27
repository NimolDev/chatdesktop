#ifndef FEATURES_VOIP_DOMAIN_USECASE_CALL_USE_CASE_HPP
#define FEATURES_VOIP_DOMAIN_USECASE_CALL_USE_CASE_HPP

// #include "model/jingle_state.hpp"
#include "repository/call_repository.hpp"
#include "signaling/call_state.hpp"

#include <QObject>
#include <QVideoFrame>
#include <memory>

namespace domain {

class CallUseCase : public QObject
{
    Q_OBJECT
public:
    explicit CallUseCase(
        std::shared_ptr<domain::CallRepository> repository,
        QObject *parent = nullptr);

    void prepare();
    void setAudioInputDevice(const QAudioDevice &device);
    void setAudioOutputDevice(const QAudioDevice &device);
    void setCameraDevice(const QCameraDevice &device);
    void execute(const QString &receiverId, bool video = false);
    void declineCall();
    void acceptCall();
    void endCall();

signals:
    void localVideoFrameReady(const QVideoFrame &frame);
    void remoteVideoFrameReady(const QVideoFrame &frame);
    void jingleMessageReceived(const QString &action, const QString &sender,
                               const QString &sessionId, bool video, const QString &reason);
    void signalingFailed(const QString &reason);
    void proposeReceived(const QString &sender, bool video);
    void connectionStateChanged(const QString&state);

    void sessionTerminate();

    void callStateChange(const voip::signaling::CallState &state);

private:
    std::shared_ptr<domain::CallRepository> m_repository;
};

} // namespace domain


#endif // FEATURES_VOIP_DOMAIN_USECASE_CALL_USE_CASE_HPP
