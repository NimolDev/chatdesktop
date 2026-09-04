#ifndef FEATURES_VOIP_PRESENTATION_VIEWMODEL_CALL_VM_HPP
#define FEATURES_VOIP_PRESENTATION_VIEWMODEL_CALL_VM_HPP

#include <QObject>

#include <webrtc/webrtc_client.hpp>

class CallVM : public QObject
{
    Q_OBJECT
public:
    explicit CallVM(QObject *parent = nullptr);


signals:

private:
    core::rtc::WebrtcClient *m_webrtcClient = nullptr;
};

#endif // FEATURES_VOIP_PRESENTATION_VIEWMODEL_CALL_VM_HPP
