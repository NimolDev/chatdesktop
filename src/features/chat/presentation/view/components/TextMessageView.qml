import QtQuick
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects

import Features.Chat
import Theme

Item {
    id: root
    implicitWidth: 600
    implicitHeight: root.isOutgoing ? bubbleOutgoing.height : bubbleIncoming.height

    property Item bubbleItem: root.isOutgoing ? bubbleOutgoing : bubbleIncoming

    property var viewmodel
    property var messagingViewModel: null
    readonly property bool messageSending: messagingViewModel ? (messagingViewModel.messageSending ?? false) : false
    property int maxBubbleWidth: 400
    property bool isOutgoing: true
    property bool isSeen: false
    property string msg: ""
    property string date: ""
    property bool selected: false

    readonly property int padding: 8
    readonly property real availableBubbleWidth: Math.max(0, Math.min(maxBubbleWidth, width))
    readonly property bool isMaxWidth: root.isOutgoing
                                       ? txtMessage.implicitWidth > (availableBubbleWidth - (msgStatus.implicitWidth + 8*5))
                                       : txtMessageIncoming.implicitWidth > (availableBubbleWidth - (msgStatusIncoming.implicitWidth + 8*5))
    readonly property bool isCollapsedMessage: width > AppLayouts.width


    Bubble {
        id: bubbleIncoming

        readonly property real maxBubbleWidth: root.availableBubbleWidth
        property real minBubbleWidth: 50
        visible: !root.isOutgoing
        anchors.left: parent.left

        width: Math.min(
            maxBubbleWidth,
            Math.max(minBubbleWidth, contentItemIncoming.implicitWidth + 8)
        )

        height: contentItemIncoming.implicitHeight + 8

        outgoing: root.isOutgoing
        showTail: true

        color: root.selected
               ? Colors.primary700
               : Colors.white50


        GridLayout {
            id: contentItemIncoming

            width: Math.max(0, parent.width - 8)

            columns: root.isMaxWidth ? 1 : 2

            rowSpacing: 0
            columnSpacing: 4

            Text {
                id: txtMessageIncoming

                Layout.topMargin: root.padding
                Layout.bottomMargin: root.padding

                Layout.leftMargin: root.isOutgoing
                                   ? root.padding
                                   : root.padding * 2.5

                Layout.rightMargin: root.padding

                Layout.fillWidth: true
                Layout.minimumWidth: 0
                Layout.maximumWidth: Math.max(0, root.availableBubbleWidth - 40)

                text: root.msg

                wrapMode: Text.Wrap
                color: Colors.black900

                font.family: Typography.family
                font.pixelSize: Typography.body
                font.weight: Typography.regular
            }

            RowLayout {
                id: msgStatusIncoming

                Layout.alignment: root.isMaxWidth
                                  ? Qt.AlignRight
                                  : Qt.AlignBottom

                Layout.rightMargin: root.isOutgoing
                                    ? root.padding
                                    : root.isMaxWidth
                                      ? root.padding
                                      : 0

                spacing: 3

                Text {
                    id: txtDateIncoming

                    text: root.date
                    color: Colors.primary

                    font.family: Typography.family
                    font.pixelSize: Typography.caption
                    font.weight: Typography.regular
                }
            }
        }

        MouseArea {
            anchors.fill: parent

            onClicked: {
                console.log("Hello")
            }
        }
    }


    Bubble {
        id: bubbleOutgoing

        readonly property real maxBubbleWidth: root.availableBubbleWidth
        property real minBubbleWidth: 50
        visible: root.isOutgoing
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

        width: Math.min(
            maxBubbleWidth,
            Math.max(minBubbleWidth, contentItem.implicitWidth + 8)
        )

        height: contentItem.implicitHeight + 8

        outgoing: root.isCollapsedMessage ? false : true
        showTail: true

        color: root.selected
               ? Colors.primary700
               : Colors.black500


        GridLayout {
            id: contentItem

            width: Math.max(0, parent.width - 8)

            columns: root.isMaxWidth ? 1 : 2

            rowSpacing: 0
            columnSpacing: 4

            Text {
                id: txtMessage

                Layout.topMargin: root.padding
                Layout.bottomMargin: root.padding

                Layout.leftMargin: root.isCollapsedMessage
                                   ? root.padding * 2.5
                                   : root.padding

                Layout.rightMargin: root.padding
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                Layout.maximumWidth: Math.max(0, root.availableBubbleWidth - 40)

                text: root.msg

                wrapMode: Text.Wrap
                color: Colors.textPrimary

                font.family: Typography.family
                font.pixelSize: Typography.body
                font.weight: Typography.regular
            }

            RowLayout {
                id: msgStatus

                Layout.alignment: root.isMaxWidth
                                  ? Qt.AlignRight
                                  : Qt.AlignBottom

                Layout.rightMargin: root.isOutgoing
                                    ? root.padding
                                    : root.isMaxWidth
                                      ? root.padding
                                      : 0

                spacing: 3

                Text {
                    id: txtDate

                    text: root.date
                    color: Colors.primary600

                    font.family: Typography.family
                    font.pixelSize: Typography.caption
                    font.weight: Typography.regular
                }



                Item {
                    id: statusIcon

                    visible: root.isOutgoing

                    Layout.preferredWidth: root.isSeen ? 20 : 15
                    Layout.preferredHeight: root.isSeen ? 20 : 15

                    Image {
                        id: icon
                        anchors.fill: parent

                        source: root.viewmodel
                                ? (root.viewmodel.sending
                                   ? AppAssets.icClock
                                   : (root.isSeen
                                      ? AppAssets.icDuobleCheck
                                      : AppAssets.icCheck))
                                : AppAssets.icCheck

                        visible: false
                        fillMode: Image.PreserveAspectFit
                    }
                    ColorOverlay {
                        anchors.fill: icon
                        source: icon
                        color: Colors.primary
                    }
                }
            }
        }

        MouseArea {
            anchors.fill: parent

            onClicked: {
                console.log("Hello")
            }
        }
    }




}
