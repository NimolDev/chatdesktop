import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Shared.UI
import Theme

Item {

    id: root
    implicitWidth: 500
    implicitHeight: 500


    property string receiverName

    signal startCallRequested(bool video)
    signal cancelCall()

    ColumnLayout {
        anchors.centerIn: parent
        width: parent.width
        spacing: 8

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
                text: root.receiverName
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
                text: "Click on Camera if you want to start video call."
                color: Colors.textSecond
                font.family: Typography.family
                font.pixelSize: Typography.body
                font.weight: Typography.medium
                wrapMode: Text.WrapAnywhere
                elide: Text.ElideRight
            }
        }

    }
    RowLayout {
        anchors {
            bottom: parent.bottom
            bottomMargin: 50
            horizontalCenter: parent.horizontalCenter
        }
        Layout.alignment: Qt.AlignHCenter

        spacing: 20

        Column {
            spacing: AppLayouts.s_padding
            CircleButton {
                id: btnVideo
                Layout.preferredHeight: 40
                Layout.preferredWidth: 40
                anchors.horizontalCenter: parent.horizontalCenter
                padding: AppLayouts.x_padding
                iconSource: AppAssets.icVideo
                color: Colors.primary700
                borderWidth: 0
                onClicked: {
                    root.startCallRequested(true)
                }
            }
            Text {
                text: "Start Video"
                font.family: Typography.family
                font.pixelSize: Typography.body
                font.weight: Typography.medium
                color: Colors.textPrimary
                anchors.horizontalCenter: parent.horizontalCenter

            }
        }
        Column {
            spacing: AppLayouts.s_padding
            CircleButton {
                id: btnCancel
                Layout.preferredHeight: 40
                Layout.preferredWidth: 40
                anchors.horizontalCenter: parent.horizontalCenter
                padding: AppLayouts.x_padding
                iconSource: AppAssets.icClose
                color: Colors.black300
                borderWidth: 0
                onClicked: {
                    root.cancelCall()
                }
            }
            Text {
                text: "Cancel"
                font.family: Typography.family
                font.pixelSize: Typography.body
                font.weight: Typography.medium
                color: Colors.textPrimary
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }
        Column {
            spacing: AppLayouts.s_padding
            CircleButton {
                id: btnAudio
                Layout.preferredHeight: 40
                Layout.preferredWidth: 40
                anchors.horizontalCenter: parent.horizontalCenter
                padding: AppLayouts.x_padding
                iconSource: AppAssets.icPhone
                color: Colors.primary700
                borderWidth: 0
                onClicked: {
                    root.startCallRequested(false)
                }
            }
            Text {
                text: "Start Call"
                font.family: Typography.family
                font.pixelSize: Typography.body
                font.weight: Typography.medium
                color: Colors.textPrimary
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }
    }

}
