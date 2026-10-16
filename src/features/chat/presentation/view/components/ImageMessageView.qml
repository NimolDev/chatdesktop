pragma ComponentBehavior: Bound

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
    property bool selectionMode: false
    property url fileLocation: ""
    property var contents: []
    readonly property var imageUrls: {
        const urls = []
        for (const item of contents) {
            if (item.msgType === 4 && item.filePath) {
                urls.push(item.filePath)
            }
            if (urls.length === 5) {
                break
            }
        }
        if (urls.length === 0 && String(fileLocation).length > 0) {
            urls.push(fileLocation)
        }
        return urls
    }

    readonly property int padding: 8
    readonly property real availableBubbleWidth: Math.max(0, Math.min(maxBubbleWidth, width))
    readonly property bool isCollapsedMessage: width > AppLayouts.width

    Bubble {
        id: bubble

        width: root.availableBubbleWidth
        height: content.implicitHeight + root.padding * 2
        states: [
            State {
                name: "outgoing"
                when: root.isOutgoing && !root.isCollapsedMessage
                AnchorChanges {
                    target: bubble
                    anchors.left: undefined
                    anchors.right: bubble.parent.right
                }
            },
            State {
                name: "incoming"
                when: !root.isOutgoing && root.isCollapsedMessage
                AnchorChanges {
                    target: bubble
                    anchors.left: bubble.parent.left
                    anchors.right: undefined
                }
            }
        ]
        outgoing: root.isOutgoing && !root.isCollapsedMessage
        showTail: true
        color: root.selected ? Colors.primary700
                             : root.isOutgoing ? Colors.black500 : Colors.white50

        ColumnLayout {
            id: content

            x: bubble.outgoing ? root.padding : root.padding * 2.5
            y: root.padding
            width: Math.max(0, bubble.width - root.padding * 3.5)
            spacing: root.padding

            GridLayout {
                id: imageGrid
                objectName: "imageGrid"
                Layout.fillWidth: true
                columns: root.imageUrls.length === 1 ? 1 : 2
                rowSpacing: 4
                columnSpacing: 4
                visible: root.imageUrls.length > 0

                Repeater {
                    model: root.imageUrls
                    delegate: Item {
                        id: tile
                        objectName: "imageTile"
                        required property int index
                        required property var modelData
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        Layout.preferredWidth: 1
                        Layout.columnSpan: root.imageUrls.length > 1
                                           && root.imageUrls.length % 2 === 1 && index === 0 ? 2 : 1
                        Layout.preferredHeight: root.imageUrls.length === 1
                            ? (photo.status === Image.Ready && photo.implicitWidth > 0
                               ? Math.min(320, imageGrid.width * photo.implicitHeight / photo.implicitWidth)
                               : Math.min(200, imageGrid.width))
                            : Math.min(160, Math.max(0, (imageGrid.width - imageGrid.columnSpacing) / 2))
                        clip: true

                        Image {
                            id: photo
                            anchors.fill: parent
                            source: root.fileLocation
                            asynchronous: true
                            cache: true
                            fillMode: root.imageUrls.length === 1 ? Image.PreserveAspectFit : Image.PreserveAspectCrop
                            autoTransform: true
                            sourceSize.width: 800
                            sourceSize.height: 800
                        }

                        MouseArea {
                            anchors.fill: parent
                            enabled: !root.selectionMode && photo.status === Image.Ready
                            cursorShape: Qt.PointingHandCursor
                            onClicked:  {
                                ImageViewCoordinator.open(root.fileLocation)
                            }
                        }

                        Text {
                            anchors.centerIn: parent
                            width: parent.width
                            visible: photo.status !== Image.Ready
                            text: photo.status === Image.Loading ? qsTr("Loading image…")
                                                                 : qsTr("Image unavailable")
                            horizontalAlignment: Text.AlignHCenter
                            wrapMode: Text.Wrap
                            color: root.isOutgoing ? Colors.textPrimary : Colors.black900
                            font.family: Typography.family
                            font.pixelSize: Typography.body
                        }
                    }
                }
            }

            Text {
                Layout.fillWidth: true
                visible: text.length > 0
                text: root.msg
                textFormat: Text.PlainText
                wrapMode: Text.Wrap
                color: root.isOutgoing ? Colors.textPrimary : Colors.black900
                font.family: Typography.family
                font.pixelSize: Typography.body
                font.weight: Typography.regular
            }

            RowLayout {
                Layout.alignment: Qt.AlignRight
                spacing: 3

                Text {
                    text: root.date
                    color: root.isOutgoing ? Colors.primary600 : Colors.primary
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
