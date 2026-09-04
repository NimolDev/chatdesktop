#include "call_vm.hpp"



CallVM::CallVM(QObject *parent)
    : QObject(parent)
{
    m_webrtcClient = new core::rtc::WebrtcClient;

    if (m_webrtcClient->initialize ()) {
        if (m_webrtcClient->createPeerConnection ()) {
            m_webrtcClient->createOffer ();
        }
    }

}
