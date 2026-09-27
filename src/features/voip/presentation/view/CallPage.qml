pragma ComponentBehavior: Bound

import QtQuick
// import QtQuick.Layouts
import QtQuick.Controls


import Theme
// import Shared.UI
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
    property bool _isAcceptCall: false

    width:AppLayouts.minWidth
    height: AppLayouts.minHeight
    minimumWidth: AppLayouts.minWidth
    minimumHeight: AppLayouts.minHeight
    visible: true
    onClosing: {
        CallVM.reset()
        _isStartCall = false
        _isAcceptCall = false
        incomingCall = false
        incomingVideo = false
        signalingError = ""
        _isPinned = false
        closeTimer.stop()
        pinnedDialog.close()
        callState.text = ""
        console.log("Call window close")

    }
    title: "Call"
    color: Colors.background

    // macOS pinning is handled natively by the coordinator to avoid resetting
    // the transparent title bar. Other platforms use the standard Qt hint.
    flags: Qt.Window | Qt.ExpandedClientAreaHint | Qt.NoTitleBarBackgroundHint
           | (Qt.platform.os !== "osx" && _isPinned ? Qt.WindowStaysOnTopHint : 0)
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

    Connections {
        target: CallVM

        function onSessionTerminate()  {
            console.log("Session Terminate");
            callWindow.close()
        }
    }


    Button {
        id: btnPin
        z: 1
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


    Loader {
        id: callLoader
        anchors.fill: parent
        active: callWindow.visible
        sourceComponent: {
               if (callWindow._isStartCall || callWindow._isAcceptCall) {
                   return inStartCallView
               }
               if (callWindow.incomingCall) {
                   return incomingCallView
               }
               return callOutView
           }
    }

    Label {
        id: callState
        anchors {
            top: parent.top
            horizontalCenter: parent.horizontalCenter
        }

        z: 1

        text: CallVM.connectionState

        color: Colors.error
        wrapMode: Text.Wrap
    }

    Component {
        id: callOutView
        CallOutView {
            anchors.fill: parent
            // Component.onCompleted: {
            //     console.log("CallOutView CREATED")
            // }
            // Component.onDestruction: {
            //     console.log("CallOutView DESTROYED")
            // }
            onStartCallRequested: (video) => {
                                    callWindow.signalingError = ""
                                    callWindow.incomingVideo = video
                                    callWindow._isStartCall = true
                                    callWindow.startCallRequested(video)
                                  }
            onCancelCall: {
                callWindow.close()
            }
        }
    }


    Component {
        id: inStartCallView
        InStartCallView {
            anchors.fill: parent
            viewmodel: CallVM
            isVideoCall: callWindow.incomingVideo

            // Component.onCompleted: {
            //     console.log("InStartCall CREATED")
            // }
            // Component.onDestruction: {
            //     console.log("InStartCall DESTROYED")
            // }

            onEndCall: {
                CallVM.endCall()
                callWindow.close()

            }
            onMuteChanged: (mute) => {
                               if (mute) {
                                   console.log("mute mic")
                               } else {
                                   console.log("unmute mic")
                               }
                           }
            onSpeakerChanged: (mute) => {
                                  if (mute) {
                                      console.log("mute speaker")
                                  } else {
                                      console.log("umute speaker")
                                  }

                              }
            onCameraChanged: (enable) => {
                                 if (enable) {
                                     console.log("enable camera")
                                 } else {
                                     console.log("disable camera")
                                 }
                             }
        }
    }

    Component {
        id: incomingCallView
        InComingCallView {
            anchors.fill: parent
            // Component.onCompleted: {
            //     console.log("incomingCallView CREATED")
            // }
            // Component.onDestruction: {
            //     console.log("incomingCallView DESTROYED")
            // }

            isVideoCall: callWindow.incomingVideo

            onAcceptCall:  {
                callWindow.signalingError = ""
                console.log("Accept call")
                callWindow._isAcceptCall = true
                CallVM.accept()
            }
            onDeclineCall:  {
                CallVM.declineCall()
                callWindow.close()
            }
        }
    }


}
