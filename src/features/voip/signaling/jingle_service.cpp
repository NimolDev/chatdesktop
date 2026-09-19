#include "jingle_service.hpp"
#include "logging/logger.hpp"

#include "xmpp/xmpp_service_discovery.hpp"

#include <QThread>
#include <QXmppClient.h>
#include <QXmppTask.h>
#include <QXmppUtils.h>
#include <QUuid>
#include <QRegularExpression>

namespace voip {
namespace signaling {

JingleService::JingleService(QObject *parent)
    : QObject(parent)

{
    qRegisterMetaType<QList<voip::signaling::jingle::StreamType>>();
    createWebrtcClient();

}

void JingleService::initialize(QXmppClient *client)
{
    Q_ASSERT(QThread::currentThread() == thread());
    Q_ASSERT(!client || client->thread() == thread());
    if (!client || m_jingle) {
        return;
    }

    m_client = client;
    m_jingle = client->findExtension<voip::signaling::JingleExtension>();
    if (!m_jingle) {
        m_jingle = client->addNewExtension<voip::signaling::JingleExtension>(client);
    }

    m_discovery = std::make_unique<core::xmpp::XmppServiceDiscovery> (m_client, this);
    connect(m_jingle, &voip::signaling::JingleExtension::proposeReceived,
            this, &JingleService::onProposeReceived);
    connect(m_jingle,
            &voip::signaling::JingleExtension::acceptReceived,
            this,
            [this](const QString &sender, const QString &) {
                if (QXmppUtils::jidToBareJid(sender) != QXmppUtils::jidToBareJid(m_recipientId)) {
                    return;
                }
                if (QXmppUtils::jidToResource(sender).isEmpty()) {
                    emit signalingFailed(QStringLiteral("Cannot start session: acceptance is missing the peer resource"));
                    return;
                }
                m_recipientId = sender;
                // m_client->removeExtension ();
                LOG_INFO ("Call accept");
                // waiting to external service dicovery response and send sessio initiate.
                m_discovery->requestExtDiscoQuery (QStringLiteral ("localhost"));
            });

    connect(this, &JingleService::localIceCandidateReceived,
            m_jingle, &JingleExtension::iceCandidate);

    connect(m_jingle,
            &voip::signaling::JingleExtension::remoteSdpOfferReceived,
            this,
            [this](const QString &offer,
                   const QList<voip::signaling::IceServer> &iceServers,
                   const QString &sid
                   ) {
                if (!m_webrtc) {
                    emit signalingFailed(QStringLiteral("Cannot answer: WebRTC is not initialized"));
                    return;
                }
                QList<core::rtc::IceServer> rtcIceServers;
                rtcIceServers.reserve(iceServers.size());
                for (const auto &ice : iceServers) {
                    core::rtc::IceServer server;
                    switch (ice.type) {
                    case IceServer::Stun:
                        server.type = core::rtc::IceServer::Stun;
                        break;
                    case IceServer::Turn:
                        server.type = core::rtc::IceServer::Turn;
                        break;
                    case IceServer::Turns:
                        server.type = core::rtc::IceServer::Turns;
                        break;
                    }
                    server.host = ice.host;
                    server.port = ice.port;
                    server.username = ice.username;
                    server.credential = ice.credential;
                    rtcIceServers.append(server);
                }
                // Only send camera video when it was requested and offered.
                static const QRegularExpression videoMedia(QStringLiteral("(?:^|\\n)m=video [1-9][0-9]* "));
                m_webrtc->setCameraEnabled(m_videoCall && videoMedia.match(offer).hasMatch());
                if (!m_webrtc->createPeerConnection(rtcIceServers)) {
                    emit signalingFailed(QStringLiteral("Cannot create the peer connection for the remote offer"));
                    return;
                }
                const QPointer<JingleService> self(this);
                const QPointer<core::rtc::WebrtcClient> rtc(m_webrtc.get());
                if (!rtc->setRemoteSdp(QStringLiteral("offer"), offer, [self, rtc] {
                    if (!self || !rtc) {
                        return;
                    }
                    rtc->createAnswer([self](const QString &answer) {
                        if (self && self->m_jingle) {
                            self->m_jingle->sessionAccept(answer);
                        }
                    });
                })) {
                    emit signalingFailed(QStringLiteral("Cannot apply the remote SDP offer"));
                }
            });

    connect(m_jingle,
            &JingleExtension::exchangeIceCandidateReceived,
            this,
            &JingleService::onExchangeIceCandidateReceived
            );

    connect(m_discovery.get (),
            &core::xmpp::XmppServiceDiscovery::externalServiceReceived,
            this,
            [this](const QVector<QXmppExternalService> &services) {
                if (!m_webrtc) {
                    return;
                }
                LOG_INFO ("Extenal service receive");
                QList<IceServer> ice_servers;
                QList<core::rtc::IceServer> rtcIceServers;
                for (auto const &service : services) {
                    const auto type = IceServer::toType(service.type());
                    if (!type) {
                        continue;
                    }
                    IceServer ice;

                    ice.type = *type;
                    ice.host = service.host ();
                    auto port = service.port();
                    if (port.has_value ()) {
                        ice.port = QString::number(*port);
                    }
                    auto username = service.username ();
                    if (username.has_value ()) {
                        ice.username = username.value ();
                    }
                    auto credential = service.password ();
                    if (credential.has_value ()) {
                        ice.credential = credential.value ();
                    }
                    core::rtc::IceServer rtcIce;
                    switch (ice.type) {
                    case IceServer::Stun:
                        rtcIce.type = core::rtc::IceServer::Stun;
                        break;
                    case IceServer::Turn:
                        rtcIce.type = core::rtc::IceServer::Turn;
                        break;
                    case IceServer::Turns:
                        rtcIce.type = core::rtc::IceServer::Turns;
                        break;
                    }
                    rtcIce.host = ice.host;
                    rtcIce.port = ice.port;
                    rtcIce.username = ice.username;
                    rtcIce.credential = ice.credential;
                    rtcIceServers.append(std::move(rtcIce));
                    ice_servers.append (std::move (ice));
                }
                if (!m_webrtc->createPeerConnection(rtcIceServers)) {
                    emit signalingFailed(QStringLiteral("Cannot create the peer connection for the local offer"));
                    return;
                }
                const QPointer<JingleService> self(this);
                const QPointer<core::rtc::WebrtcClient> rtc(m_webrtc.get ());
                rtc->createOffer ([self, ice_servers](const QString &sdp_offer) {
                    if (self && self->m_jingle) {
                        self->m_jingle->sessionInitaite(self->m_recipientId,sdp_offer, ice_servers);
                    }
                });
            });
    connect (m_jingle,
            &JingleExtension::remoteSdpAnswerReceived,
            this,
            [this](const QString &answer, const QString &) {
                if (!m_webrtc || !m_webrtc->setRemoteSdp(QStringLiteral("answer"), answer)) {
                    emit signalingFailed(QStringLiteral("Cannot apply the remote SDP answer"));
                }
            });

}

void JingleService::createWebrtcClient()
{
    emit localVideoFrameReady(QImage());
    emit remoteVideoFrameReady(QImage());
    m_webrtc = std::make_unique<core::rtc::WebrtcClient>(this);
    connect(m_webrtc.get(), &core::rtc::WebrtcClient::localVideoFrameReady,
            this, &JingleService::localVideoFrameReady);
    connect(m_webrtc.get(), &core::rtc::WebrtcClient::remoteVideoFrameReady,
            this, &JingleService::remoteVideoFrameReady);
    connect(m_webrtc.get(), &core::rtc::WebrtcClient::localIceCandidateGenerated,
            this, &JingleService::onLocalIceCandidateReceived);
    connect(m_webrtc.get(), &core::rtc::WebrtcClient::errorOccurred,
            this, &JingleService::signalingFailed);
    connect(m_webrtc.get (),
            &core::rtc::WebrtcClient::connectionStateChanged,
            this,
            [this](const core::rtc::ConnectionState state) {
                switch(state) {

                case core::rtc::ConnectionState::Checking:
                    qDebug() << "checking";
                    break;
                case core::rtc::ConnectionState::Connecting:
                    qDebug() << "Connecting";
                    break;
                case core::rtc::ConnectionState::Connected:
                    qDebug() << "Connected";
                    break;
                case core::rtc::ConnectionState::Disconnect:
                    qDebug() << "Disconnect";
                    break;
                }
                emit connectionStateChange (state);
            });

}

void JingleService::onLocalIceCandidateReceived(const QString &candidate, const QString &sdpMid, int sdpMLineIndex)
{
    emit localIceCandidateReceived(candidate, sdpMid, sdpMLineIndex);
}

void JingleService::onExchangeIceCandidateReceived(
    const std::string &sdp,
    const std::string &sdpMid,
    const int sdpMLineIndex)
{
    m_webrtc->setRemoteCandidate (sdp, sdpMid, sdpMLineIndex);
    LOG_INFO ("ICE Exchange");
}

void JingleService::acceptCall()
{
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, &JingleService::acceptCall, Qt::QueuedConnection);
        return;
    }
    if (isConnected ()) {
        createWebrtcClient();
        if (!m_webrtc->initialize()) {
            emit signalingFailed(QStringLiteral("Cannot initialize WebRTC for the call"));
            m_webrtc.reset();
            return;
        }
        m_webrtc->setCameraEnabled(m_videoCall);
        m_jingle->sendAccept();
    }

}

void JingleService::endCall()
{
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, &JingleService::endCall, Qt::QueuedConnection);
        return;
    }
    m_webrtc.reset();
    emit localVideoFrameReady(QImage());
    emit remoteVideoFrameReady(QImage());
}

void JingleService::rejectCall()
{
    if (QThread::currentThread () != thread()) {
        QMetaObject::invokeMethod (this, &JingleService::rejectCall, Qt::QueuedConnection);
        return;
    }
    if (isConnected ()) {
        m_jingle->reject ();
        m_webrtc = NULL;
    }
}

void JingleService::ringing()
{
    m_jingle->ringing ();
}



void JingleService::startCall(const QString &receiverId, bool video)
{
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, [this, receiverId, video] {
            startCall(receiverId, video);
        }, Qt::QueuedConnection);
        return;
    }
    if (!m_client || !m_jingle || !m_client->isConnected()) {
        emit signalingFailed(QStringLiteral("Cannot start call: XMPP is not connected"));
        return;
    }

    QString recipient = QXmppUtils::jidToBareJid(receiverId.trimmed());
    if (!recipient.isEmpty() && !recipient.contains(QLatin1Char('@'))) {
        recipient += QLatin1Char('@') + m_client->configuration().domain();
    }
    static const QRegularExpression jidPattern(QStringLiteral("^[^@/\\s]+@[^@/\\s]+$"));
    if (!jidPattern.match(recipient).hasMatch()) {
        emit signalingFailed(QStringLiteral("Cannot start call: invalid recipient"));
        return;
    }
    if (recipient == m_client->configuration().jidBare()) {
        emit signalingFailed(QStringLiteral("Cannot call your own account"));
        return;
    }

    m_recipientId = recipient;
    m_videoCall = video;
    createWebrtcClient();
    if (!m_webrtc->initialize()) {
        emit signalingFailed(QStringLiteral("Cannot initialize WebRTC for the outgoing call"));
        m_webrtc.reset();
        return;
    }
    m_webrtc->setCameraEnabled(video);
    using voip::signaling::jingle::StreamType;
    QList<StreamType> streams {StreamType::Audio};
    if (video) {
        streams.append(StreamType::Video);
    }
    const QString proposalId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    voip::signaling::jingle::JingleMessage builder;
    auto message = builder.propose(recipient, streams, proposalId);
    message.setId(QUuid::createUuid().toString(QUuid::WithoutBraces));
    m_client->send(std::move(message)).then(this,
        [this, recipient, proposalId](QXmpp::SendResult result) {
            if (const auto *error = std::get_if<QXmppError>(&result)) {
                emit signalingFailed(QStringLiteral("Could not send call proposal: ") + error->description);
                return;
            }
            emit proposalSent(recipient, proposalId);
        });

}

void JingleService::onProposeReceived(
    const QString &sender, const QList<voip::signaling::jingle::StreamType> &streams)
{
    qDebug() << "===== on Propose received from:"<< sender << "=====";
    m_videoCall = streams.contains(jingle::StreamType::Video);
    emit proposalReceived (sender, streams);
}

bool JingleService::isConnected()
{
    if (!m_client || !m_jingle || !m_client->isConnected()) {
        emit signalingFailed(QStringLiteral("Cannot accept call: XMPP is not connected"));
        return false;
    }
    return true;
}

} // namespace signaling
} // namespace voip
