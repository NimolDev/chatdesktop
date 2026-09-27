import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

import Shared.UI
import Theme
import Features.Voip

Item {
    id: root

    property bool isVideoCall: false
    property string callerName

    signal acceptCall()
    signal declineCall()

    ColumnLayout {
        anchors.centerIn: parent
        width: parent.width

        CircularImage {
            id: imgProfile
            Layout.preferredHeight: 80
            Layout.preferredWidth: 80
            Layout.alignment: Qt.AlignHCenter
            source: "qrc:/images/profile.jpeg"
        }
            Column {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignHCenter
                Text {
                    id: txtUserName
                    text: root.callerName
                    font.family: Typography.family
                    font.pixelSize: Typography.title4
                    font.weight: Typography.medium
                    color: Colors.primary
                    anchors.horizontalCenter: parent.horizontalCenter
                }
                Text {
                    id: txtCallStatus
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                    text: root.isVideoCall ? "Incoming video call" : "Incoming audio call"

                    color: Colors.textSecond
                    font.family: Typography.family
                    font.pixelSize: Typography.body
                    font.weight: Typography.medium
                    wrapMode: Text.WrapAnywhere
                    elide: Text.ElideRight
                }
            }

                RowLayout {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.topMargin: 25
                    spacing: 30

                    Column {
                        // visible: !callWindow.incomingCall
                        spacing: AppLayouts.s_padding
                        CircleButton {
                            id: btnDecline
                            Layout.preferredHeight: 40
                            Layout.preferredWidth: 40
                            anchors.horizontalCenter: parent.horizontalCenter
                            padding: AppLayouts.x_padding
                            iconSource: AppAssets.icPhoneDown
                            color: Colors.red600
                            borderWidth: 0
                            onClicked: {
                             root.declineCall()
                            }
                        }

                    }
                    Column {
                        spacing: AppLayouts.s_padding
                        CircleButton {
                            id: btnAccept
                            Layout.preferredHeight: 40
                            Layout.preferredWidth: 40
                            anchors.horizontalCenter: parent.horizontalCenter
                            padding: AppLayouts.x_padding
                            iconSource: AppAssets.icPhone
                            color: Colors.green200
                            borderWidth: 0
                            onClicked: {
                                // callWindow.close()
                                // CallVM.accept()
                                // callWindow._isAcceptCall = true
                                root.acceptCall()

                            }
                        }

                    }
                }

    }


}
