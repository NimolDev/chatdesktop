import QtQuick 2.15
import QtQuick.Layouts
import QtQuick.Effects

import Features.Chat
import Theme
import Shared.UI

Item {
    id: root

    implicitWidth: 600
    implicitHeight: 50
    property Item bubbleItem: root.isOutgoing ? bubbleOutgoing : bubbleIncoming

    property int maxBubbleWidth: 400
    property bool isOutgoing: true
    property bool isSeen: false
    property string msg: "0"
    property string date: "0"
    property bool selected: false
    property var fileLocation
    property var duration: "0"

    readonly property int padding: 8
    readonly property real availableBubbleWidth: Math.max(0,
                                                          Math.min(maxBubbleWidth, width))
    readonly property bool isCollapsedMessage: width > AppLayouts.width
    readonly property real _sliderHeight: 6

    property var messagingViewModel: null
    readonly property var voiceController: messagingViewModel
                                           ? messagingViewModel.voiceController
                                           : null
    readonly property bool activePlayback: voiceController
                                           && voiceController.voicePlaybackSource.toString() === String(root.fileLocation)
    readonly property bool playing: activePlayback && voiceController.voicePlaybackPlaying

    Bubble {
        id: bubbleIncoming

        readonly property real maxBubbleWidth: root.availableBubbleWidth
        property real minBubbleWidth: 50
        visible: !root.isOutgoing
        anchors.left: parent.left

        width: 250
        height: 50

        outgoing: false
        showTail: true

        color: root.selected
               ? Colors.primary700
               : Colors.white50

        RowLayout {
            id: contentItemIncoming
            width: Math.max(0, parent.width - 8)
            anchors.centerIn: parent

            Rectangle {
                id: btnDonwload
                Layout.preferredHeight: 45
                Layout.preferredWidth: 45
                Layout.alignment: Qt.AlignVCenter
                Layout.leftMargin: AppLayouts.x_padding
                color: Colors.black500
                radius:45/2
                clip: true
                DownloadCircleButton {
                   anchors.fill: parent

                    iconSource: root.playing
                                ? AppAssets.icPause : AppAssets.icPlay
                    iconColor: Colors.white50
                    downloading: root.activePlayback && root.voiceController.voiceDownloading
                    trackColor: "transparent"
                    strokeWidth: 0

                    progress: root.activePlayback ? root.voiceController.voiceDownloadProgress : 0
                    onClicked: {
                        if (root.voiceController) {
                            root.voiceController.playVoice(root.fileLocation)
                            console.log("File path: ", root.fileLocation)
                        }
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.rightMargin: 16
                spacing: 4

                MySlider {
                    id: voiceSlider
                    Layout.fillWidth: true
                    Layout.leftMargin: -5
                    Layout.rightMargin: -5
                    backgroundColor: Colors.white500
                    progressColor: Colors.black500
                    progressHeight: root._sliderHeight
                    showHandler: root.playing
                    from: 0
                    to: Math.max(1,
                                 root.activePlayback
                                 ? root.voiceController.voicePlaybackDuration
                                 : 0)
                    value: root.activePlayback
                           ? root.voiceController.voicePlaybackPosition
                           : 0
                    enabled: root.activePlayback
                             && root.voiceController.voicePlaybackSeekable
                    onMoved: root.voiceController.seekVoice(value)
                }
                RowLayout {
                    spacing: 4

                    Text {
                        id: durationText

                        text: root.duration
                        color: Colors.black500
                        font.family: Typography.family
                        font.pixelSize: Typography.caption
                        font.weight: Typography.regular

                        Layout.alignment: Qt.AlignBaseline
                        verticalAlignment: Text.AlignBottom
                    }
                    Item {
                        Layout.fillWidth: true
                    }

                    Text {
                        id: txtSendDate

                        text: root.date
                        color: Colors.black500
                        font.family: Typography.family
                        font.pixelSize: Typography.caption
                        font.weight: Typography.regular

                        Layout.alignment: Qt.AlignBaseline
                        verticalAlignment: Text.AlignBottom
                    }
                }
            }
        }
    }

    Bubble {
        id: bubbleOutgoing
        readonly property real maxBubbleWidth: root.availableBubbleWidth
        property real minBubbleWidth: 50
        visible: root.isOutgoing
        // anchors.right: parent.right
        states: [
            State {
                name: "collapsed"
                when: root.isCollapsedMessage
                AnchorChanges {
                    target: bubbleOutgoing
                    anchors.left: bubbleOutgoing.parent.left
                    anchors.right: undefined
                }
            },
            State {
                name: "normal"
                when: !root.isCollapsedMessage
                AnchorChanges {
                    target: bubbleOutgoing
                    anchors.left: undefined
                    anchors.right: bubbleOutgoing.parent.right
                }
            }
        ]

        width: 250
        height: 50

        outgoing: root.isCollapsedMessage ? false : true
        showTail: true

        color: root.selected
               ? Colors.primary700
               : Colors.black500

        RowLayout {
            id: contentItemOutgoing
            width: Math.max(0, parent.width - 8)
            anchors.centerIn: parent

            Rectangle {
                id: btnDonwload1
                Layout.preferredHeight: 42
                Layout.preferredWidth: 42
                Layout.alignment: Qt.AlignVCenter
                Layout.leftMargin: root.isCollapsedMessage ?  AppLayouts.x_padding : 0
                color: Colors.white50
                radius:45/2
                clip: true

                DownloadCircleButton {
                    anchors.fill: parent
                    iconSource: root.playing
                                ? AppAssets.icPause : AppAssets.icPlay
                    iconColor: "black"
                    downloading: root.activePlayback && root.voiceController.voiceDownloading
                    trackColor: "transparent"
                    backgroundColor: Colors.white50
                    strokeWidth: 0
                    progress: root.activePlayback ? root.voiceController.voiceDownloadProgress : 0
                    onClicked: {
                        if (root.voiceController) {
                            root.voiceController.playVoice(root.fileLocation)
                        }
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.rightMargin: 16
                spacing: 4

                MySlider {
                    id: voiceSlider1
                    Layout.fillWidth: true
                    Layout.leftMargin: -5
                    Layout.rightMargin: -5
                    backgroundColor: Colors.suface3
                    progressHeight: root._sliderHeight
                    showHandler: root.playing
                    progressColor: Colors.white50
                    from: 0
                    to: Math.max(1, root.activePlayback ? root.voiceController.voicePlaybackDuration : 0)
                    value: root.activePlayback ? root.voiceController.voicePlaybackPosition : 0
                    enabled: root.activePlayback && root.voiceController.voicePlaybackSeekable
                    onMoved: root.voiceController.seekVoice(value)
                }
                RowLayout {
                    spacing: 4
                    Text {
                        id: durationText1

                        text: root.duration
                        color: Colors.textPrimary
                        font.family: Typography.family
                        font.pixelSize: Typography.caption
                        font.weight: Typography.regular

                        Layout.alignment: Qt.AlignBaseline
                        verticalAlignment: Text.AlignBottom
                    }
                    Item {
                        Layout.fillWidth: true
                    }

                    Text {
                        id: txtSendDate1

                        text: root.date
                        color: Colors.textPrimary
                        font.family: Typography.family
                        font.pixelSize: Typography.caption
                        font.weight: Typography.regular

                        Layout.alignment: Qt.AlignBaseline
                        verticalAlignment: Text.AlignBottom
                    }

                    Item {
                        visible: root.isOutgoing

                        Layout.preferredWidth: 15
                        Layout.preferredHeight: 15

                        // Fine-tune visual bottom alignment
                        Layout.alignment: Qt.AlignBottom
                        Layout.bottomMargin: 4

                        Image {
                            id: icon1
                            anchors.fill: parent

                            source: root.isSeen
                                    ? AppAssets.icDuobleCheck
                                    : AppAssets.icCheck

                            visible: false
                            fillMode: Image.PreserveAspectFit
                        }

                        MultiEffect {
                            anchors.fill: icon1
                            source: icon1

                            brightness: 1.0
                            colorization: 1.0
                            colorizationColor: Colors.primary
                        }
                    }
                }
            }
        }
    }

}
