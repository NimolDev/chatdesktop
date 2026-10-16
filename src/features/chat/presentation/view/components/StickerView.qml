import QtQuick 2.15
import QtQuick.Layouts
import QtQuick.Effects

import Features.Chat
import Theme

Item {
    id: root

    implicitWidth: 600
    implicitHeight: bubble.height
    readonly property Item bubbleItem: bubble

    property int maxBubbleWidth: 400
    property bool isOutgoing: true
    property bool isSeen: false
    property string msg: ""
    property string date: ""
    property bool selected: false
    property url fileLocation: ""

    readonly property int padding: 8
    readonly property real availableBubbleWidth: Math.max(0, Math.min(maxBubbleWidth, width))
    readonly property bool isCollapsedMessage: width > AppLayouts.width

    Item {
        id: bubble

        width: Math.min(220, root.availableBubbleWidth)
        height: content.implicitHeight + root.padding * 2
        anchors.left: outgoing ? undefined : parent.left
        anchors.right: outgoing ? parent.right : undefined
        readonly property bool outgoing: root.isOutgoing && !root.isCollapsedMessage

        Rectangle {
            anchors.fill: parent
            color: "transparent"
            radius: root.padding
            border.width: root.selected ? 2 : 0
            border.color: Colors.primary
        }

        ColumnLayout {
            id: content

            x: root.padding
            y: root.padding
            width: Math.max(0, bubble.width - root.padding * 2)
            spacing: root.padding

            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: photo.status === Image.Ready && photo.implicitWidth > 0
                                        ? Math.min(220, width * photo.implicitHeight / photo.implicitWidth)
                                        : Math.min(200, width)

                Image {
                    id: photo
                    anchors.fill: parent
                    source: root.fileLocation
                    asynchronous: true
                    fillMode: Image.PreserveAspectFit
                    autoTransform: true
                }

                Text {
                    anchors.centerIn: parent
                    width: parent.width
                    visible: photo.status !== Image.Ready
                    text: photo.status === Image.Loading ? qsTr("Loading sticker…")
                                                        : qsTr("Sticker unavailable")
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.Wrap
                    color: Colors.textPrimary
                    font.family: Typography.family
                    font.pixelSize: Typography.body
                }
            }

            RowLayout {
                Layout.alignment: Qt.AlignRight
                spacing: 3

                Text {
                    text: root.date
                    color: Colors.textSecond
                    font.family: Typography.family
                    font.pixelSize: Typography.caption
                    font.weight: Typography.regular
                }

                Item {
                    visible: root.isOutgoing
                    Layout.preferredWidth: root.isSeen ? 20 : 15
                    Layout.preferredHeight: Layout.preferredWidth

                    Image {
                        id: statusIcon
                        anchors.fill: parent
                        source: root.isSeen ? AppAssets.icDuobleCheck : AppAssets.icCheck
                        visible: false
                        fillMode: Image.PreserveAspectFit
                    }

                    MultiEffect {
                        anchors.fill: statusIcon
                        source: statusIcon
                        brightness: 1.0
                        colorization: 1.0
                        colorizationColor: Colors.primary
                    }
                }
            }
        }
    }
}
