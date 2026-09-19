#ifndef FEATURES_VOIP_DATA_CALL_REPOSITORY_IMPL_HPP
#define FEATURES_VOIP_DATA_CALL_REPOSITORY_IMPL_HPP

#include "signaling/jingle_service.hpp"
#include "webrtc/webrtc_client.hpp"
#include "domain/repository/call_repository.hpp"

#include <QObject>


namespace data {

class CallRepositoryImpl : public domain::CallRepository
{
    Q_OBJECT
public:
    explicit CallRepositoryImpl(
        std::shared_ptr<voip::signaling::JingleService> signaling,
        QObject *parent = nullptr
        );


signals:

private:
    std::shared_ptr<voip::signaling::JingleService> m_signaling;

    std::unique_ptr<core::rtc::WebrtcClient> m_webrtc;

    // CallRepository interface
public:
    void startCall(const QString &receiverId, bool is_video) override;
    void endCall() override;
    void declineCall() override;
    void acceptCall() override;
};

} // namespace data


#endif // FEATURES_VOIP_DATA_CALL_REPOSITORY_IMPL_HPP
