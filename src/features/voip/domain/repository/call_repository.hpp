#ifndef FEATURES_VOIP_DOMAIN_REPOSITORY_CALL_REPOSITORY_HPP
#define FEATURES_VOIP_DOMAIN_REPOSITORY_CALL_REPOSITORY_HPP

#include <domain/model/jingle_state.hpp>
#include <QObject>
#include <QImage>

namespace domain {

class CallRepository : public QObject
{
    Q_OBJECT
public:
    explicit CallRepository(QObject *parent = nullptr) : QObject(parent) {};
    virtual ~CallRepository() = default;
    virtual void startCall(const QString &receiverId, bool is_video = false) = 0;
    virtual void endCall() = 0;
    virtual void declineCall() = 0;
    virtual void acceptCall() = 0;

signals:
    void localVideoFrameReady(const QImage &image);
    void remoteVideoFrameReady(const QImage &image);
    void jingleMessageReceived(const QString &action, const QString &sender,
                               const QString &sessionId, bool video, const QString &reason);
    void signalingFailed(const QString &reason);
    void proposeReceived(const QString &sender, bool video);
    void connectionStateChanged(const QString &state);
};

} // namespace domain


#endif // FEATURES_VOIP_DOMAIN_REPOSITORY_CALL_REPOSITORY_HPP
