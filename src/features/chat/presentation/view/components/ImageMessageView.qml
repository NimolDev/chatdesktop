pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Layouts
import QtQuick.Effects

import Features.Chat
import Theme
import Shared.UI

Item {
    id: root

    implicitWidth: 600
    implicitHeight: bubble.height

    readonly property Item bubbleItem: bubble

    property var viewmodel
    property var mediaControl: null
    property int maxBubbleWidth: 400
    property int maxImageHeight: 320
    property bool selected: false
    property bool selectionMode: false

    readonly property bool isOutgoing: viewmodel ? (viewmodel.isMine ?? false) : false
    readonly property bool isSeen: viewmodel ? (viewmodel.seen ?? false) : false
    readonly property string msg: viewmodel ? (viewmodel.body ?? "") : ""
    readonly property string date: viewmodel ? (viewmodel.sentAt ?? "") : ""
    readonly property var fileData: viewmodel && viewmodel.contents
                                    && viewmodel.contents.length > 0
                                    ? viewmodel.contents[0] : viewmodel
    readonly property string messageId: viewmodel ? (viewmodel.id ?? "") : ""
    readonly property int contentIndex: fileData ? (fileData.contentIndex ?? 0) : 0
    readonly property url fileLocation: fileData ? (fileData.filePath ?? "") : ""
    readonly property url imageSource: {
        return fileLocation
    }
    // media control
    readonly property real downloadProgress: fileData ? (fileData.downloadProgress ?? 0) : 0
    readonly property bool isDownloading: fileData ? (fileData.isDownloading ?? false) : false
    readonly property url downloadedFile: fileData ? (fileData.downloadedFile ?? "") : ""
    readonly property string downloadError: fileData ? (fileData.downloadError ?? "") : ""
    readonly property real uploadProgress: fileData ? (fileData.uploadProgress ?? 0) : 0
    readonly property bool isUploading: fileData ? (fileData.isUploading ?? false) : false
    readonly property bool hasLocalFile: downloadedFile.toString().length > 0 || fileLocation.toString().startsWith("file:")

    // view layouts
    readonly property int padding: AppLayouts.m_padding
    readonly property real availableBubbleWidth: Math.max(0, Math.min(maxBubbleWidth, width))
    readonly property real imageAspectRatio: tile.aspectRatio
    readonly property real imageHorizontalPadding: padding * 3.5
    readonly property real sizedBubbleWidth: imageAspectRatio > 0 && imageAspectRatio < 1
        ? Math.min(availableBubbleWidth, maxImageHeight * imageAspectRatio + imageHorizontalPadding)
        : availableBubbleWidth
    readonly property bool isCollapsedMessage: width > AppLayouts.width

    Bubble {
        id: bubble

        width: root.sizedBubbleWidth
         height: content.implicitHeight

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
            x: 0
            y: 0
            width: Math.max(0, bubble.width - 12)

            spacing: root.padding

            Item {
                id: tile
                objectName: "imageTile"
                readonly property real aspectRatio: photo.status === Image.Ready && photo.implicitHeight > 0
                                                    ? photo.implicitWidth / photo.implicitHeight : 0
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                Layout.preferredWidth: 1
                Layout.preferredHeight: tile.aspectRatio > 0
                    ? width / tile.aspectRatio : Math.min(200, width)
                visible: String(root.imageSource).length > 0


                clip: true

                Image {
                    id: photo
                    anchors.fill: parent
                    source: root.imageSource
                    asynchronous: true
                    cache: true
                    fillMode: Image.PreserveAspectFit
                    autoTransform: true
                    sourceSize.width: 800
                    sourceSize.height: 800
                    layer.enabled: true
                    layer.effect: MultiEffect {
                        maskEnabled: true
                        maskSource: imageMask
                    }
                }

                DownloadCircleButton {
                    width: 50
                    height: 50
                    z: 2
                    anchors.centerIn: parent
                    backgroundColor: "transparent"
                    iconSource: AppAssets.icClose
                    iconColor: Colors.white50
                    progressColor: Colors.white50
                    strokeWidth: 3
                    progress: 0.01
                    downloading: true
                    onClicked: {
                        console.log("MessageId: ", root.messageId,
                                    "ContentIndex: ", root.contentIndex)
                    }

                }

                Rectangle {
                    id: imageMask
                    anchors.fill: parent
                    topLeftRadius: Math.min(bubble.radius, width / 2, height / 2)
                    topRightRadius: Math.min(bubble.radius, width / 2, height / 2)

                    color: "white"
                    antialiasing: true
                    visible: false
                    layer.enabled: true
                }

                MouseArea {
                    anchors.fill: parent
                    enabled: !root.selectionMode && photo.status === Image.Ready
                    cursorShape: Qt.PointingHandCursor
                    onClicked:  {
                        ImageViewCoordinator.open(root.imageSource)
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
                Layout.rightMargin: root.padding
                Layout.leftMargin: root.padding
                Layout.bottomMargin: root.padding
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
