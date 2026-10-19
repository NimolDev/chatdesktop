pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Controls

import Features.Chat
import Theme

Item {

    id: root
    implicitWidth: 600
    implicitHeight: messageLoader.implicitHeight

    // Exposed for MessagingBody's hit testing and context-menu positioning.
    readonly property Item bubbleItem: messageLoader.item ? messageLoader.item.bubbleItem : null


    required property var viewmodel
    property var messagingViewModel: null

    property int maxBubbleWidth: 400
    property bool selectionMode: false
    property real removalProgress: 0

    // Allocate an offscreen texture only while this bubble is disappearing.
    readonly property bool supportsDissolve: GraphicsInfo.api !== GraphicsInfo.Software
    opacity: supportsDissolve ? 1 : 1 - removalProgress
    layer.enabled: removalProgress > 0 && supportsDissolve
    layer.effect: ShaderEffect {
        property var source
        property real progress: root.removalProgress
        property vector2d itemSize: Qt.vector2d(root.width, root.height)
        fragmentShader: "qrc:/chat/shaders/message_dissolve.frag.qsb"
    }

    property bool isOutgoing: viewmodel ? viewmodel.isMine :  false
    property bool isSeen: viewmodel ? viewmodel.seen : false
    property string msg: viewmodel ? viewmodel.body : ""
    property string date: viewmodel ? viewmodel.sentAt : ""
    property int msgType: viewmodel ? viewmodel.msgType : -1
    property var fileLocation: viewmodel ? viewmodel.filePath : ""

    readonly property int padding: 8

    // --- selection state ---
    property bool selected: false


    // --- Check Box ---
    CheckBox {
        id: checkbox
        visible: root.selectionMode
        checked: root.selected
        anchors.left: parent.left
        anchors.leftMargin: root.padding
        anchors.verticalCenter: parent.verticalCenter
        opacity: visible ? 1 : 0
        enabled: false

        indicator: Rectangle {
            implicitWidth: 22
            implicitHeight: 22
            // anchors.fill: parent
            radius: width / 2
            color: checkbox.checked ? Colors.primary : "transparent"
            border.width: 2
            border.color: checkbox.checked
                          ? Colors.primary
                          : Colors.primary300

            Text {
                anchors.centerIn: parent
                text: "\u2713"
                visible: checkbox.checked
                color: Colors.textPrimary
                font.pixelSize: 14
                font.bold: true
            }

            Behavior on color {
                ColorAnimation { duration: 100 }
            }
        }

        Behavior on opacity {
            NumberAnimation { duration: 120 }
        }
    }


    Loader {
        id: messageLoader
        anchors.fill: parent
        anchors.leftMargin: root.selectionMode ? checkbox.width : 0

        Behavior on anchors.leftMargin {
            NumberAnimation { duration: 120 }
        }

        sourceComponent: {

            switch (root.msgType) {
                case 0:
                 return textMessage
                case 1:
                    // voice
                    return voiceMessage
                case 2:
                    // video
                    break
                case 3:
                    // location
                    break
                case 4:
                    return imageMessage
                case 5:
                    return fileMessage
                case 6:
                    return stickerMessage
                default:
                    break
            }
        }
    }

    // Message view component
    Component {
        id: stickerMessage
        StickerView {
            maxBubbleWidth: Math.max(0, Math.min(root.maxBubbleWidth, messageLoader.width - root.padding))
            isOutgoing: root.isOutgoing
            isSeen: root.isSeen
            date: root.date
            selected: root.selected
            fileLocation: root.fileLocation
        }
    }

    Component {
        id: fileMessage

        FileMessageView {
            viewmodel: root.viewmodel
            messagingViewModel: root.messagingViewModel
            maxBubbleWidth: Math.max(0, Math.min(root.maxBubbleWidth, messageLoader.width - root.padding))
            selected: root.selected
            selectionMode: root.selectionMode
        }
    }

    Component {
        id: imageMessage
        ImageMessageView {
            maxBubbleWidth: Math.max(0, Math.min(root.maxBubbleWidth, messageLoader.width - root.padding))
            viewmodel: root.viewmodel
            mediaControl: root.messagingViewModel
            selected: root.selected
            selectionMode: root.selectionMode


        }
    }

    Component {
        id: textMessage
        TextMessageView {
            maxBubbleWidth: Math.max(0, Math.min(root.maxBubbleWidth, messageLoader.width - root.padding))
            isOutgoing: root.isOutgoing
            isSeen: root.isSeen
            msg: root.msg
            date: root.date
            selected: root.selected
        }
    }
    Component {
        id: voiceMessage
        VoiceMessageView {
            readonly property var modelData: root.viewmodel && root.viewmodel.contents
                                             ? root.viewmodel.contents[0] : root.viewmodel
            messageId: root.viewmodel ? root.viewmodel.id : ""
            uploadProgress: modelData ? modelData.uploadProgress : 0
            isUploading: modelData ? modelData.isUploading : false
            messagingViewModel: root.messagingViewModel
            maxBubbleWidth: Math.max(0, Math.min(root.maxBubbleWidth, messageLoader.width - root.padding))
            isOutgoing: root.isOutgoing
            isSeen: root.isSeen
            msg: root.msg
            date: root.date
            selected: root.selected
            fileLocation: root.fileLocation
            duration: root.viewmodel ? root.viewmodel.duration : 0
        }
    }



}
