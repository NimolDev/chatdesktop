#include "call_vm.hpp"

#include <QDebug>


CallVM::CallVM(QObject *parent)
    : QObject(parent)
{
    m_webrtcClient = new core::rtc::WebrtcClient;

    if (m_webrtcClient->initialize ()) {
        if (m_webrtcClient->createPeerConnection ()) {
            m_webrtcClient->createOffer ();

            // m_webrtcClient->createAnswer ();
        }
    }

    connect(
        m_webrtcClient,
        &core::rtc::WebrtcClient::localSdpCreated,
        this,
        [this](const QString &type, const QString &sdp) {
            qDebug() << "local sdp created" << sdp;
            m_webrtcClient->setRemoteSdp (type, sdp);
        }
        );




}
