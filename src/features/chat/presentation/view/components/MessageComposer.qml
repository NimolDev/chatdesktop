import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Window

import Theme
import Shared.UI

Rectangle {
    id: root

    property bool emojiSideVisible: false
    property real previousHeight: 0

    signal submitted(string text)
    signal emojiSideToggleRequested()
    signal heightDelta(real delta)

    function resetState() {
        messageInput.clear()
        attachmentPopup.close()
        emojiPopup.close()
        emojiCloseTimer.stop()
    }

    function submit() {
        const text = messageInput.text.trim()
        if (text.length === 0)
            return

        submitted(text)
        messageInput.clear()
        messageInput.forceActiveFocus()
    }

    implicitHeight: Math.max(56, inputScroll.height + 16)
    color: Colors.background.alpha(0.9)

    Component.onCompleted: previousHeight = height
    onEmojiSideVisibleChanged: {
        if (emojiSideVisible)
            emojiPopup.close()
    }
    onHeightChanged: {
        if (previousHeight === 0) {
            previousHeight = height
            return
        }

        const delta = height - previousHeight
        previousHeight = height
        heightDelta(delta)
    }

    CircleButton {
        id: attachmentButton
        width: 40
        height: 40
        icon.source: AppAssets.icAttactment
        borderColor: Colors.suface3
        borderWidth: 1
        tintColor: Colors.primary
        hoverColor: Colors.primaryHover

        anchors {
            left: parent.left
            bottom: parent.bottom
            leftMargin: 8
            bottomMargin: 8
        }

        onClicked: attachmentPopup.open()
        onHoveredChanged: {
            if (hovered && !attachmentPopup.visible)
                attachmentPopup.open()
        }

        Attachment {
            id: attachmentPopup
            x: 0
            y: -(height + 16)
        }
    }

    Rectangle {
        anchors {
            left: attachmentButton.right
            right: parent.right
            bottom: parent.bottom
            bottomMargin: 8
            leftMargin: 4
            rightMargin: 8
        }
        height: inputScroll.height
        color: "transparent"
        border.color: Colors.suface3
        radius: 20

        ScrollView {
            id: inputScroll
            height: Math.min(Math.max(messageInput.contentHeight + 16, 40), 120)

            anchors {
                left: parent.left
                right: emojiButton.left
                bottom: parent.bottom
                leftMargin: 4
            }
            clip: true

            TextArea {
                id: messageInput
                width: inputScroll.availableWidth
                height: inputScroll.availableHeight
                wrapMode: TextEdit.WrapAnywhere
                verticalAlignment: Text.AlignVCenter
                selectByMouse: true
                font.pixelSize: 15
                background: Rectangle { color: "transparent" }

                Keys.onPressed: event => {
                    if (event.key !== Qt.Key_Return && event.key !== Qt.Key_Enter)
                        return

                    if (event.modifiers & Qt.ShiftModifier)
                        insert(cursorPosition, "\n")
                    else
                        root.submit()

                    event.accepted = true
                }
            }
        }

        Text {
            anchors {
                left: inputScroll.left
                leftMargin: messageInput.leftPadding
                verticalCenter: inputScroll.verticalCenter
            }
            visible: messageInput.text.length === 0
            text: qsTr("Write a message")
            color: messageInput.palette.placeholderText
            font: messageInput.font
        }

        Button {
            id: emojiButton
            width: 40
            height: 40
            anchors {
                right: sendButton.left
                bottom: parent.bottom
            }
            icon.source: AppAssets.icEmoji
            icon.color: hovered ? Colors.primaryHover : Colors.primary
            hoverEnabled: true
            background: Rectangle { color: "transparent" }

            function positionPopup() {
                const position = mapToItem(null, 0, 0)
                const distanceFromRight = Window.window.width - position.x
                emojiPopup.x = -Math.abs(distanceFromRight - emojiPopup.width - 8)
                emojiPopup.y = -(emojiPopup.height + 16)
            }

            function showPopup() {
                emojiCloseTimer.stop()
                positionPopup()
                emojiPopup.open()
            }

            onClicked: root.emojiSideToggleRequested()
            onHoveredChanged: {
                if (root.emojiSideVisible)
                    return

                if (hovered)
                    showPopup()
                else
                    emojiCloseTimer.restart()
            }

            Timer {
                id: emojiCloseTimer
                interval: 180
                onTriggered: {
                    if (!emojiButton.hovered && !emojiPopup.hovered)
                        emojiPopup.close()
                }
            }

            HoverHandler { cursorShape: Qt.PointingHandCursor }

            EmojiPopup {
                id: emojiPopup
                onHoveredChanged: {
                    if (hovered)
                        emojiCloseTimer.stop()
                    else if (!emojiButton.hovered)
                        emojiCloseTimer.restart()
                }
            }
        }

        Button {
            id: sendButton
            width: 40
            height: 40
            padding: 8
            anchors {
                right: parent.right
                bottom: parent.bottom
                rightMargin: 8
            }
            icon.source: messageInput.text.trim().length > 0
                         ? AppAssets.icSend : AppAssets.icMicrophone
            icon.color: hovered ? Colors.primaryHover : Colors.primary
            hoverEnabled: true
            background: Rectangle { color: "transparent" }
            onClicked: root.submit()
        }
    }
}
