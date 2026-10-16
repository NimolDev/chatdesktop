pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Theme

Popup {
    id: filePopup
    property var viewmodel
    objectName: "attachmentUploadDialog"

    signal send(var urls,string caption, bool compress)
    property var files: []
    property bool compress
    readonly property bool uploading: filePopup.viewmodel ? filePopup.viewmodel.isUploadingAttachments : false
    readonly property bool canceling: filePopup.viewmodel ? filePopup.viewmodel.isCancelingAttachmentUpload : false

    readonly property int fileCount: files.length

    popupType: Popup.Item
    closePolicy: Popup.NoAutoClose
    focus: true
    anchors.centerIn: Overlay.overlay
    width: Math.min(400, Math.max(0, Overlay.overlay.width - 32))
    height: contentItem.implicitHeight + topPadding + bottomPadding
    modal: true
    onOpened: captionInput.forceActiveFocus()

    onClosed: {
        captionInput.clear()
        files = []
    }

    function removeFile(index) {
        if (uploading) {
            return
        }
        const remaining = files.slice()
        remaining.splice(index, 1)
        files = remaining
        if (files.length === 0) {
            if (filePopup.viewmodel) {
                filePopup.viewmodel.resetAttachments();
            }
            close()
        }
    }
    function resetAttachments() {
        if (filePopup.viewmodel) {
            filePopup.viewmodel.resetAttachments();
            close()
        }
    }

    background: Rectangle {
        color: Colors.suface1
        radius: AppLayouts.x_radius
        border {
            color: Colors.suface2
            width: 1
        }
    }
    contentItem: ColumnLayout {
        id: contentItem
        RowLayout {
            Layout.preferredHeight: 35
            Layout.fillWidth: true
            Text {
                id: txtTitle
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter
                text: filePopup.uploading ? qsTr("Uploading…") : filePopup.fileCount < 2
                      ? "Send as file"
                      : filePopup.fileCount + " files selected"
                color: Colors.textPrimary
                font.family: Typography.family
                font.pixelSize: Typography.title4
                font.weight: Typography.semiBold

            }
            Button {
                id: btnClose
                objectName: "attachmentUploadClose"
                Layout.preferredHeight: 35
                Layout.preferredWidth: 35
                padding: 8
                icon.source: AppAssets.icClose
                icon.color: hovered ? Colors.primary  : Colors.textPrimary
                background: Rectangle {
                    color: Colors.suface2
                    radius: width/2
                    border {
                        color: Colors.suface3
                        width: 1
                    }
                }

                onClicked: {
                    filePopup.close()
                    filePopup.files = []
                    if (filePopup.viewmodel) {
                        filePopup.resetAttachments()
                    }
                }
                hoverEnabled: true
                HoverHandler {
                    cursorShape: Qt.PointingHandCursor
                }
            }
        }

        ScrollView {
            id: fileScroll

            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(filePopup.fileCount * 50, 200)
            clip: true
            contentWidth: availableWidth
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
            ColumnLayout {
                width: fileScroll.availableWidth
                spacing: 0
                Repeater {
                    model: filePopup.viewmodel.attachmentModel
                    delegate: Item {
                        id: delegate
                        Layout.fillWidth: true
                        Layout.preferredHeight: 50
                        implicitHeight: 50
                        required property int index
                        required property var modelData
                        readonly property bool showImagePreview: filePopup.compress
                            && /\.(avif|bmp|gif|heic|heif|jfif|jpeg|jpg|jxl|pbm|pgm|png|ppm|webp|xbm|xpm)$/i.test(String(modelData.url).split(/[?#]/)[0])
                        readonly property string fileName: {
                            const path = String(modelData.url).split(/[?#]/)[0]
                            const name = path.substring(path.lastIndexOf("/") + 1)
                            try {
                                return decodeURIComponent(name)
                            } catch (error) {
                                return name
                            }
                        }

                        RowLayout {
                            width: delegate.width
                            height: 50
                            spacing: 8

                            Rectangle {
                                Layout.preferredWidth: 45
                                Layout.preferredHeight: 45
                                Layout.alignment: Qt.AlignVCenter

                                color: Colors.suface2
                                radius: width / 2
                                clip: true

                                Image {
                                    id: fileCompress
                                    anchors.fill: parent
                                    source: delegate.showImagePreview ? delegate.modelData.url : ""
                                    sourceSize.width: 180
                                    sourceSize.height: 180
                                    asynchronous: true
                                    fillMode: Image.PreserveAspectFit
                                    visible: status === Image.Ready
                                }

                                Image {
                                    visible: fileCompress.status !== Image.Ready
                                    // anchors.fill: parent
                                    anchors.centerIn: parent
                                    width: 22
                                    height: 22
                                    source: AppAssets.icDocument
                                    fillMode: Image.PreserveAspectFit
                                }
                            }

                            Text {
                                Layout.fillWidth: true
                                text: delegate.modelData.fileName
                                font.family: Typography.family
                                font.pixelSize: Typography.body
                                font.weight: Typography.semiBold
                                color: Colors.textPrimary

                                elide: Text.ElideRight
                            }
                            Button {
                                id: btnRemove
                                enabled: !filePopup.uploading
                                icon.source: AppAssets.icClose
                                padding: 0
                                Layout.preferredHeight: 15
                                Layout.preferredWidth: 15
                                Layout.rightMargin: AppLayouts.m_padding
                                background: null
                                hoverEnabled: true
                                icon.color: hovered ? Colors.error : Colors.textPrimary
                                onClicked:  {
                                    console.log("File id: ", delegate.modelData.id)
                                    filePopup.removeFile(delegate.index)
                                }
                                HoverHandler {
                                    cursorShape: Qt.PointingHandCursor
                                }
                            }
                        }

                    }
                }
            }
        }

        ScrollView {
            id: captionScroll
            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(captionInput.contentHeight,
                                            captionInput.textLineHeight * 5)
                                    + captionInput.topPadding + captionInput.bottomPadding
            clip: true
            contentWidth: availableWidth
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
            ScrollBar.vertical.policy: captionInput.lineCount > 5
                                       ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff

            TextArea {
                id: captionInput
                objectName: "attachmentCaption"
                readOnly: filePopup.uploading
                readonly property real textLineHeight: contentHeight / Math.max(1,lineCount)
                width: captionScroll.availableWidth
                placeholderText: qsTr("Add a caption…")
                textFormat: TextEdit.PlainText
                wrapMode: TextEdit.Wrap
                selectByMouse: true
                color: Colors.textPrimary
                placeholderTextColor: Colors.textSecond
                font.family: Typography.family
                font.pixelSize: Typography.body
                padding: 10
                rightPadding: 18
                background: Rectangle {
                    color: "transparent"
                }
            }

        }
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: Colors.suface3
            Layout.leftMargin: AppLayouts.x_padding
            Layout.rightMargin: AppLayouts.x_padding
        }




        Button {
            id: btnSend
            objectName: "attachmentUploadAction"
            Layout.alignment: Qt.AlignRight
            text:  qsTr("Send")
            enabled: !filePopup.canceling && filePopup.fileCount > 0
            palette.buttonText: Colors.primary
            hoverEnabled: true
            font.family: Typography.family
            font.pixelSize: Typography.body
            font.weight: Typography.semiBold
            background: Rectangle {
                color: btnSend.hovered ? Colors.primary950 : "transparent"
                radius: AppLayouts.m_radius
            }
            onClicked: {

                if (filePopup.viewmodel) {
                    const  urls = filePopup.files
                    .map(file => file.url)
                    .filter(url => url !== undefined && url !== null && url !== "")
                    if (filePopup.files[0]["compress"] === true) {
                        filePopup.send(urls, captionInput.text, true)

                    } else {
                        filePopup.send(urls, captionInput.text, false)

                    }
                     filePopup.close()
                }

            }
            HoverHandler {
                cursorShape: Qt.PointingHandCursor
            }
        }

    }
}
