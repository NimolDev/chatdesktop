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

    property int maxBubbleWidth: 400
    property bool isOutgoing: true
    property bool isSeen: false
    property string msg: "Test"
    property string date: ""
    property bool selected: false
    property url fileLocation: ""

    property var messagingViewModel: null
    property string messageId: ""
    property int contentIndex: 0
    property string displayName: ""
    property real downloadProgress: 0
    property bool isDownloading: false
    property url downloadedFile: ""
    property string downloadError: ""
    readonly property bool hasLocalFile: downloadedFile.toString().length > 0 || fileLocation.toString().startsWith("file:")

    property real uploadProgress: 0
    property bool isUploading: false

    property real fileSize: 0
    property bool selectionMode: false
    readonly property string fileName: {
        if (displayName.length > 0) {
            return displayName
        }
        const name = fileLocation.toString().split(/[?#]/)[0].split("/").pop()
        try {
            return decodeURIComponent(name) || qsTr("File")
        } catch (error) {
            return name || qsTr("File")
        }
    }
    readonly property string sizeText: {
        if (fileSize <= 0) {
            return qsTr("File")
        }
        const units = ["B", "KB", "MB", "GB", "TB"]
        const unit = Math.min(units.length - 1, Math.floor(Math.log(fileSize) / Math.log(1024)))
        return (fileSize / Math.pow(1024, unit)).toFixed(unit === 0 ? 0 : 1) + " " + units[unit]
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

            RowLayout {
                Layout.fillWidth: true
                spacing: root.padding

                // Button {
                //     id: openButton
                //     Layout.preferredWidth: 42
                //     Layout.preferredHeight: 42
                //     enabled: !root.selectionMode && root.fileLocation.toString().length > 0
                //     icon.source: AppAssets.icAttactment
                //     icon.color: root.isOutgoing ? Colors.black500 : Colors.white50
                //     hoverEnabled: true
                //     Accessible.name: qsTr("Open %1").arg(root.fileName)
                //     ToolTip.visible: hovered
                //     ToolTip.text: qsTr("Open file")
                //     background: Rectangle {
                //         radius: 21
                //         color: root.isOutgoing ? Colors.white50 : Colors.black500
                //         opacity: openButton.hovered ? 0.8 : 1
                //     }
                //     onClicked: Qt.openUrlExternally(root.fileLocation)
                //     HoverHandler {
                //         cursorShape: openButton.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
                //     }
                // }

                DownloadCircleButton {
                    id: btnDownload
                    objectName: "fileTransferButton"
                    Layout.preferredWidth: 42
                    Layout.preferredHeight: 42
                    iconSource: root.isUploading || root.isDownloading ? AppAssets.icClose
                                : root.hasLocalFile ? AppAssets.icDocument : AppAssets.icArrowDown
                    backgroundColor: root.isOutgoing ? Colors.white50 : Colors.black500
                    iconColor: Colors.black500
                    downloading: root.isUploading || root.isDownloading
                    // Keep a small rotating arc visible while the upload is queued.
                    indeterminate: false
                    trackColor: "transparent"
                    progressColor: Colors.primary
                    strokeWidth: downloading ? 3 : 0
                    progressInset: 1
                    progress: root.isUploading ? Math.max(0.01, root.uploadProgress) : root.downloadProgress
                    onClicked: {
                        if (root.isUploading) {
                            if (root.messagingViewModel) {
                                root.messagingViewModel.cancelFileUpload(root.messageId, root.contentIndex)
                            }
                        } else if (root.isDownloading && root.messagingViewModel) {
                            root.messagingViewModel.cancelFileDownload(root.messageId, root.contentIndex)
                        } else if (root.hasLocalFile) {
                            Qt.openUrlExternally(root.downloadedFile.toString().length > 0 ? root.downloadedFile : root.fileLocation)
                        } else if (root.messagingViewModel) {
                            root.messagingViewModel.downloadFile(root.messageId, root.contentIndex)
                        }
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    spacing: 4

                    Text {
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        text: root.fileName
                        textFormat: Text.PlainText
                        elide: Text.ElideMiddle
                        color: root.isOutgoing ? Colors.textPrimary : Colors.black900
                        font.family: Typography.family
                        font.pixelSize: Typography.body
                        font.weight: Typography.medium
                    }

                    Text {
                        Layout.fillWidth: true
                        text: root.downloadError.length > 0 ? root.downloadError
                              : root.isUploading ? (root.uploadProgress > 0
                                                    ? qsTr("Uploading %1%").arg(Math.round(root.uploadProgress * 100))
                                                    : qsTr("Preparing upload…"))
                              : root.isDownloading ? qsTr("Downloading %1%").arg(Math.round(root.downloadProgress * 100))
                              : root.sizeText
                        elide: Text.ElideRight
                        color: root.isOutgoing ? Colors.textPrimary : Colors.black900
                        font.family: Typography.family
                        font.pixelSize: Typography.caption
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
