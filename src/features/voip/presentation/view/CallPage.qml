pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls


import Theme
import Shared.UI
import Features.Voip

Window {

    id: callWindow
    objectName: "callWindow"
    property string receiverId: "Test"
    property string userName: ""
    property string signalingError: ""
    property bool incomingCall: false
    property bool incomingVideo: false
    signal startCallRequested(bool video)

    property bool _isPinned: false
    property bool _isStartCall: false

    width:AppLayouts.minWidth
    height: AppLayouts.minHeight
    minimumWidth: AppLayouts.minWidth
    minimumHeight: AppLayouts.minHeight
    visible: true
    onClosing: CallVM.reset()
    title: "Call"
    color: Colors.background

    // macOS pinning is handled natively by the coordinator to avoid resetting
    // the transparent title bar. Other platforms use the standard Qt hint.
    flags: Qt.Window | (Qt.platform.os !== "osx" && _isPinned
                        ? Qt.WindowStaysOnTopHint : 0)
    MouseArea {
        anchors.fill: parent
        property point pressPosition
        onPressed: mouse => {
                       pressPosition = Qt.point(mouse.x, mouse.y)
                   }
        onPositionChanged: mouse => {
                               if (!pressed) {
                                   return
                               }
                               callWindow.x += mouse.x - pressPosition.x
                               callWindow.y += mouse.y - pressPosition.y
                           }
    }

    // Remote video
    RemoteVideoItem {
        // anchors {
        //     top: parent.top
        //     topMargin: 48
        //     bottom: parent.bottom
        //     bottomMargin: 100
        //     left: parent.left
        //     right: parent.right
        // }
        anchors.fill: parent
        visible: CallVM.hasRemoteVideo
        frame: CallVM.remoteVideoFrame
    }

    // local video
    Rectangle {
        id: localPreview
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.rightMargin: 16
        anchors.bottomMargin: 50
        width: Math.min(180, callWindow.width * 0.32)
        height: width * 9 / 16
        visible: CallVM.hasLocalVideo
        radius: 12
        clip: true
        color: "red"


        z: 1

        RemoteVideoItem {
            id: localPreviewVideo
            anchors.fill: parent
            anchors.margins: 1
            frame: CallVM.localVideoFrame
            radius: 12
            // Mirror only the self-preview, leaving transmitted frames unchanged.
            transform: Scale {
                origin.x: localPreviewVideo.width / 2
                xScale: -1
            }
        }


    }

    Connections {
        target: CallVM
        function onLocalVideoFrameChanged() {
            if (CallVM.hasLocalVideo) {
                callWindow.incomingCall = false
                callWindow._isStartCall = true
            }
        }
        function onRemoteVideoFrameChanged() {
            if (CallVM.hasRemoteVideo) {
                callWindow.incomingCall = false
                callWindow._isStartCall = true
            }
        }
    }

    Button {
        id: btnPin
        anchors {
            top: parent.top
            right: parent.right
            topMargin: AppLayouts.x_padding
            rightMargin: AppLayouts.x_padding
        }
        width: 20
        height:20
        padding: 0
        icon.source: callWindow._isPinned
                     ? AppAssets.icPinFill
                     : AppAssets.icPin
        icon.color: callWindow._isPinned
                    ? Colors.primary300
                    : (hovered ? Colors.primary300 : Colors.primary600)
        background: null
        rotation: 45

        onClicked: {
            callWindow._isPinned = !callWindow._isPinned
            pinnedDialog.text = callWindow._isPinned
                    ? "Call window pinned on top."
                    : "Call window unpinned from top."
            pinnedDialog.open()

        }
        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            onClicked: btnPin.clicked()
            cursorShape: Qt.PointingHandCursor
        }

    }

    Dialog {
        id: pinnedDialog
        anchors.centerIn: parent
        width: implicitWidth

        property string text: "Call window in pinned on top."

        enter: Transition {
           ParallelAnimation {
               NumberAnimation {
                   property: "opacity"
                   from: 0
                   to: 1
                   duration: 200
               }
               NumberAnimation {
                   property: "scale"
                   from: 0.95
                   to: 1.0
                   duration: 200
                   easing.type: Easing.OutCubic
               }
           }
        }
        exit: Transition {
            ParallelAnimation {
                NumberAnimation {
                    property: "opacity"
                    from: 1
                    to: 0
                    duration: 250
                }
                NumberAnimation {
                    property: "scale"
                    from: 1
                    to:  0.95
                    duration: 250
                    easing.type: Easing.InQuad
                }
            }
        }

        background: Rectangle {
            color: Colors.black900.alpha(0.8)
            radius: AppLayouts.m_radius
        }
        contentItem: Text {
            text: pinnedDialog.text
             color: Colors.textPrimary
            font.family: Typography.family
            font.pixelSize: Typography.body
            font.weight: Typography.medium

        }

        onOpened: closeTimer.start()
        Timer {
            id: closeTimer
            interval: 2000
            repeat: false
            onTriggered: pinnedDialog.close()
        }
    }


    ColumnLayout {
        visible: !CallVM.hasRemoteVideo
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
                text: callWindow.userName || callWindow.receiverId
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
                text: CallVM.connectionState ||  (callWindow.incomingCall
                      ? (callWindow.incomingVideo ? "Incoming video call" : "Incoming audio call")
                      : (callWindow._isStartCall
                         ? "Calling..." : "Click on Camera if you want to start video call."))
                color: Colors.textSecond
                font.family: Typography.family
                font.pixelSize: Typography.body
                font.weight: Typography.medium
                wrapMode: Text.WrapAnywhere
                elide: Text.ElideRight
            }
        }
        RowLayout {
            visible: callWindow.incomingCall
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
                      CallVM.declineCall();
                      callWindow.close()
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
                        CallVM.accept()

                    }
                }

            }
        }
    }

    // ---- Call button ----
    RowLayout {
        visible: !callWindow._isStartCall && !callWindow.incomingCall
        // visible: false
        anchors {
            bottom: parent.bottom
            bottomMargin: 12
            horizontalCenter: parent.horizontalCenter
        }

        spacing: 20

        Column {
            // visible: !callWindow.incomingCall
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
                    callWindow.signalingError = ""
                    callWindow._isStartCall = true
                    callWindow.startCallRequested(true)
                    // CallVM.startCall(true)

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
                    callWindow._isStartCall = false
                    callWindow.close()

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
            // visible: !callWindow.incomingCall
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
                    callWindow.signalingError = ""
                    callWindow._isStartCall = true
                    callWindow.startCallRequested(false)
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
    // ---- In Connected Call ----
    RowLayout {
        visible: callWindow._isStartCall
        anchors {
            bottom: parent.bottom
            bottomMargin: 12
            horizontalCenter: parent.horizontalCenter
         }
        spacing: 20

        Column {
            spacing: AppLayouts.s_padding
            CircleButton {
                id: btnSpeaker
                Layout.preferredHeight: 40
                Layout.preferredWidth: 40
                anchors.horizontalCenter: parent.horizontalCenter
                padding: AppLayouts.x_padding
                iconSource: AppAssets.icVideo
                color: Colors.primary700
                borderWidth: 0
            }
            Text {
                text: "Speaker"
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
                id: btnEnd
                Layout.preferredHeight: 40
                Layout.preferredWidth: 40
                anchors.horizontalCenter: parent.horizontalCenter
                padding: AppLayouts.x_padding
                iconSource: AppAssets.icPhoneDown
                color: Colors.error
                borderWidth: 0
                onClicked: {
                    CallVM.endCall();
                    callWindow._isStartCall = false
                    callWindow.close()

                }
            }
            Text {
                text: "End Call"
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
                id: btnMute
                Layout.preferredHeight: 40
                Layout.preferredWidth: 40
                anchors.horizontalCenter: parent.horizontalCenter
                padding: AppLayouts.x_padding
                iconSource: AppAssets.icMicrophone
                color: Colors.primary700
                borderWidth: 0
                onClicked: {

                }

            }
            Text {
                text: "Mute"
                font.family: Typography.family
                font.pixelSize: Typography.body
                font.weight: Typography.medium
                color: Colors.textPrimary
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }
    }


}
