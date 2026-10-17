import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts


import Theme
import Shared.UI

Rectangle {
    id: root

    property var viewmodel
    property bool emojiSideVisible: false
    property real previousHeight: 0

    signal submitted(string text)
    signal emojiSideToggleRequested()
    signal heightDelta(real delta)
    property var messagingViewModel: null
    readonly property bool isSendingVoice: messagingViewModel ? messagingViewModel.isSendingVoice : false
    readonly property string voiceError: messagingViewModel && messagingViewModel.voiceSendError
                                        ? messagingViewModel.voiceSendError
                                        : (viewmodel ? viewmodel.voiceRecordingError : "")
    signal sendVoice(int duration)
    signal filePicker(var urls)
    signal imageAndVideoFileSelected(var urls)
    signal filesSelected(var urls)
    signal locationSelected(var coordinate)

    property bool _isRecoding: viewmodel ? viewmodel.isVoiceRecordingBusy : false
    readonly property bool hasVoicePreview: root.viewmodel && String(root.viewmodel.voiceRecordingFile).length > 0

    property var _nearToLock: 0
    property bool _isLockRecoding: false
    property bool _isPause: false
    property bool _isPreview: false
    property bool _suppressAttachmentHover: false


    Connections {
        target: root.viewmodel
        function onVoiceRecordingCanceled() {
            root.resetRecoding()
        }
        function onVoiceRecordingFailed(message) {
            if (root.hasVoicePreview) {
                root._isRecoding = true
                root._isPreview = true
            }
        }
    }

    Connections {
        target: root.messagingViewModel
        function onVoiceMessageQueued() { root.resetRecoding() }
        function onVoiceSendFailed(message) {
            if (root.hasVoicePreview) {
                root._isRecoding = true
                root._isPreview = true
            }
        }
    }

    onVisibleChanged: {
        if (!visible) {
            if (root.viewmodel) { root.viewmodel.stopVoicePreview() }
        }
    }

    function resetState() {
        if (root.viewmodel) { root.viewmodel.stopVoicePreview() }
        longPressTimer.stop()
        sendButton.longPressed = false
        sendButton.pointerDown = false
        if (viewmodel) {
            viewmodel.cancelVoiceRecording()
        }
        messageInput.clear()
        attachmentPopup.close()
        emojiPopup.close()
        emojiCloseTimer.stop()
        resetRecoding()
    }

    function submit() {
        if (audioRecoder.visible) {
            return
        }
        const text = messageInput.text.trim()
        if (text.length === 0)
            return

        submitted(text)
        messageInput.clear()
        messageInput.forceActiveFocus()
    }
    function sendRecording() {
        if (!root.viewmodel || root.isSendingVoice) {
            return
        }
        if (root.viewmodel) { root.viewmodel.stopVoicePreview() }
        root.viewmodel.stopVoiceRecording()
        root._isRecoding = true
        root._isPreview = true
        root._isLockRecoding = false
        root.sendVoice(root.viewmodel.voiceRecordingDuration)
        resetRecoding()
    }

    function resetRecoding() {
        root._isRecoding = false
        root._isLockRecoding = false
        root._nearToLock = 0
        root._isPause = false
        root._isPreview = false
    }
    implicitHeight: Math.max(56, inputScroll.height + 16) + (voiceSendStatus.visible ? 28 : 0)

    Text {
        id: voiceSendStatus
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 8
        z: 2
        // The preview row displays upload status without resizing the composer.
        visible: !root._isPreview && root.viewmodel && root.voiceError.length > 0
        text: root.viewmodel ? (root.isSendingVoice ? qsTr("Uploading audio…") : root.voiceError) : ""
        color: root.viewmodel && root.isSendingVoice ? Colors.textPrimary : Colors.error
        elide: Text.ElideRight
        font.pixelSize: 12
    }
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
        objectName: "attachmentButton"
        enabled: !audioRecoder.visible
        hoverEnabled: true
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
            if (!hovered && !audioRecoder.visible) {
                root._suppressAttachmentHover = false
            }
            if (hovered && !root._suppressAttachmentHover && !attachmentPopup.visible) {
                attachmentPopup.open()
            }
        }

        Attachment {
            id: attachmentPopup
            objectName: "attachmentPopup"
            x: 0
            y: -(height + 16)
            onFilePicker: urls => {
                              root.filePicker(urls)
                          }
            onImageAndVideoFileSelected: urls => root.imageAndVideoFileSelected(urls)
            onFilesSelected: urls => root.filesSelected(urls)
            onLocationSelected: coordinate => root.locationSelected(coordinate)
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

        Flickable {
            id: inputScroll
            height: Math.max(40, Math.min(messageInput.contentHeight,
                                         messageInput.textLineHeight * 8)
                            + messageInput.topPadding + messageInput.bottomPadding)
            boundsBehavior: Flickable.StopAtBounds
            flickableDirection: Flickable.VerticalFlick
            onHeightChanged: Qt.callLater(ensureCursorVisible)
            onContentHeightChanged: Qt.callLater(ensureCursorVisible)

            function ensureCursorVisible() {
                const maxOffset = Math.max(0, contentHeight - height)
                const cursor = messageInput.cursorRectangle
                let offset = Math.max(0, Math.min(contentY, maxOffset))
                const cursorTop = cursor.y - messageInput.topPadding
                const cursorBottom = cursor.y + cursor.height + messageInput.bottomPadding
                if (cursorTop < offset) {
                    offset = cursorTop
                } else if (cursorBottom > offset + height) {
                    offset = cursorBottom - height
                }
                contentY = Math.max(0, Math.min(offset, maxOffset))
            }
            ScrollBar.vertical: ScrollBar {
                policy: inputScroll.contentHeight > inputScroll.height
                        ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff
            }

            anchors {
                left: parent.left
                right: emojiButton.left
                bottom: parent.bottom
                leftMargin: 4
            }
            clip: true

            TextArea.flickable: TextArea {
                id: messageInput
                readonly property real textLineHeight: contentHeight / Math.max(1, lineCount)
                readOnly: audioRecoder.visible
                textFormat: TextEdit.PlainText
                wrapMode: TextEdit.Wrap
                verticalAlignment: Text.AlignTop
                selectByMouse: true
                topPadding: Math.max(8, (40 - contentHeight) / 2)
                bottomPadding: topPadding
                rightPadding: 14
                font.pixelSize: 15
                placeholderText: qsTr("Write a message")
                background: Rectangle { color: "transparent" }
                onCursorRectangleChanged: Qt.callLater(inputScroll.ensureCursorVisible)

                Keys.onPressed: event => {
                    if (event.key !== Qt.Key_Return && event.key !== Qt.Key_Enter) {
                        return
                    }

                    if (event.modifiers & Qt.ShiftModifier) {
                        // Let TextArea insert the newline and keep its cursor visible.
                        event.accepted = false
                    } else {
                        root.submit()
                        event.accepted = true
                    }
                }
            }
        }



        Button {
            id: emojiButton
            enabled: !audioRecoder.visible
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

                // if (hovered)
                //     showPopup()
                // else
                //     emojiCloseTimer.restart()
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
            enabled: !root.viewmodel || !root.isSendingVoice
            objectName: "messageSendButton"

            width: 40
            height: 40
            padding: 8

            anchors {
                right: parent.right
                bottom: parent.bottom
                rightMargin: 8
            }

            property bool longPressed: false
            property bool pointerDown: false

            icon.source: messageInput.text.trim().length > 0
                         ? AppAssets.icSend
                         : AppAssets.icMicrophone

            icon.color: hovered
                        ? Colors.primaryHover
                        : Colors.primary

            hoverEnabled: true

            background: Rectangle {
                color: "transparent"
            }
            MouseArea {
                id: mouseArea

                anchors.fill: parent

                acceptedButtons: Qt.LeftButton
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor

                // Important: keep receiving mouse events
                // after leaving the button.
                preventStealing: true

                onPressed: function(mouse) {
                    sendButton.pointerDown = true
                    sendButton.longPressed = false

                    if (messageInput.text.trim().length === 0) {
                        longPressTimer.restart()
                    }
                    console.log("PRESS")

                }

                onReleased: function(mouse) {
                    longPressTimer.stop()

                    if (sendButton.longPressed) {
                        if (root._isLockRecoding) {return}
                        sendButton.longPressed = false
                        root.sendRecording()

                    } else {
                        console.log("NORMAL CLICK")
                        root.submit()
                    }

                    sendButton.pointerDown = false
                }

                onCanceled: {
                    longPressTimer.stop()

                    if (sendButton.longPressed) {
                        root.viewmodel.cancelVoiceRecording()
                    }
                    sendButton.longPressed = false
                    sendButton.pointerDown = false
                }
                onPositionChanged: function(mouse) {
                    if (sendButton.longPressed) {
                        // console.log(
                        //     "Recording, position:",
                        //     mouse.x,
                        //     mouse.y
                        // )

                        if (mouse.y <= -50 && mouse.x >= -50) {
                            if (!root._isLockRecoding) {
                                root._isLockRecoding = true
                            }
                        } else if (mouse.y <= 0 && mouse.x >= -50) {
                            root._nearToLock = Math.abs(mouse.y)
                        }
                    }
                }
            }

            Timer {
                id: longPressTimer

                interval: 100
                repeat: false

                onTriggered: {
                    if (sendButton.pointerDown) {
                        sendButton.longPressed = true
                        console.log("START RECORDING")
                        root._isRecoding = true
                        if (root.viewmodel) { root.viewmodel.stopVoicePreview() }
                        root.viewmodel.startVoiceRecording();
                    }
                }
            }
        }
    }

    Rectangle {
        id: audioRecoder
        anchors.fill: parent
        anchors.topMargin: voiceSendStatus.visible ? 28 : 0
        visible: root._isRecoding
        color: Colors.suface3

        onVisibleChanged: {
            if (visible) {
                root._suppressAttachmentHover = true
                attachmentPopup.close()
                emojiPopup.close()
                emojiCloseTimer.stop()
            }
        }

        // Block the covered composer while keeping recorder controls above this area.
        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.AllButtons
            hoverEnabled: true
            onWheel: wheel => { wheel.accepted = true }
        }

        RowLayout {
            visible: root._isRecoding && !root._isPreview
            enabled: !root.viewmodel || !root.isSendingVoice
            anchors.fill: parent
            anchors.leftMargin: 8
            anchors.rightMargin: 8
            spacing: 12

            Rectangle {
                Layout.preferredHeight: 10
                Layout.preferredWidth: 10
                radius: 5
                color: Colors.error

                SequentialAnimation on opacity {
                    loops: Animation.Infinite
                    running: root._isRecoding

                    NumberAnimation {
                        to: 0
                        duration: 500
                        easing.type: Easing.InOutSine
                    }
                    NumberAnimation {
                        to: 1
                        duration: 500
                        easing.type: Easing.InOutSine
                    }
                }
            }

            Text {
                id: txtDuration
                Layout.preferredWidth: 150
                text: root.viewmodel ? root.viewmodel.voiceRecordingDurationText : "00:00:0"
                horizontalAlignment: Text.AlignLeft
                font.family: Typography.family
                font.pixelSize: Typography.body
                font.weight: Typography.regular
                color: Colors.textPrimary
            }
            Button {
                id: btnCancel
                Layout.fillWidth: true
                Layout.rightMargin: 150
                contentItem: Text {
                    text: "Cancel"
                    color: Colors.textPrimary
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter

                    scale: btnCancel.pressed ? 0.9 : 1.0

                    Behavior on scale {
                        NumberAnimation {
                            duration: 100
                            easing.type: Easing.OutCubic
                        }
                    }
                }

                background: Rectangle {
                    radius: 8
                    color: "transparent"
                }
                onClicked: {
                    root.resetRecoding()
                    root.viewmodel.cancelVoiceRecording()

                }
            }

            Rectangle {
                id: recoding
                Layout.preferredHeight: 45
                Layout.preferredWidth: 45
                Layout.alignment: Qt.Horizontal
                color: "transparent"
                // -------------------------
                // Wave 1
                // -------------------------
                Rectangle {
                    id: wave1
                    anchors.centerIn: parent
                    width: 56
                    height: width
                    radius: width / 2
                    color: Colors.primary900
                    opacity: 0

                    SequentialAnimation {
                        running: root._isRecoding
                        loops: Animation.Infinite
                        ParallelAnimation {
                            NumberAnimation {
                                target: wave1
                                property: "scale"
                                from: 1.0
                                to: 1.7
                                duration: 1200
                                easing.type: Easing.OutQuad
                            }

                            NumberAnimation {
                                target: wave1
                                property: "opacity"
                                from: 0.35
                                to: 0
                                duration: 1200
                            }
                        }
                    }
                }

                // -------------------------
                // Wave 2
                // -------------------------
                Rectangle {
                    id: wave2
                    anchors.centerIn: parent
                    width: 56
                    height: width
                    radius: width / 2
                    color: Colors.primary400
                    opacity: 0

                    SequentialAnimation {
                        running:  root._isRecoding
                        loops: Animation.Infinite
                        PauseAnimation {
                            duration: 400
                        }

                        ParallelAnimation {
                            NumberAnimation {
                                target: wave2
                                property: "scale"
                                from: 1.0
                                to: 1.7
                                duration: 1200
                                easing.type: Easing.OutQuad
                            }

                            NumberAnimation {
                                target: wave2
                                property: "opacity"
                                from: 0.35
                                to: 0
                                duration: 1200
                            }

                        }

                        PauseAnimation {
                            duration: 800
                        }
                    }
                }

                // -------------------------
                // Wave 3
                // -------------------------

                Rectangle {
                    id: wave3
                    anchors.centerIn: parent
                    width: 56
                    height: width
                    radius: width / 2
                    color: Colors.primary700
                    opacity: 0
                    SequentialAnimation {
                        running:  root._isRecoding
                        loops: Animation.Infinite
                        PauseAnimation {
                            duration: 800
                        }

                        ParallelAnimation {
                            NumberAnimation {
                                target: wave3
                                property: "scale"
                                from: 1.0
                                to: 1.7
                                duration: 1200
                                easing.type: Easing.OutQuad
                            }

                            NumberAnimation {
                                target: wave3
                                property: "opacity"
                                from: 0.35
                                to: 0
                                duration: 1200
                            }
                        }

                        PauseAnimation {
                            duration: 400
                        }
                    }
                }

                CircleButton {
                    id: btnSend
                    anchors.fill: parent
                    color: Colors.primary
                    borderWidth: 0
                    iconSource: root._isLockRecoding ?AppAssets.icSend : AppAssets.icMicrophone
                    padding: 12
                    onClicked: {
                        root.sendRecording()
                    }
                }
            }
        }

        RowLayout {
            id: previewvVoice
            enabled: !root.viewmodel || !root.isSendingVoice
            anchors.fill: parent
            anchors.margins: 8
            spacing: 12
            visible:  root._isPreview

            Button {
                id: btnDeleteVoice
                objectName: "btnDeleteVoice"
                Layout.preferredHeight: 35
                Layout.preferredWidth: 35
                icon.source: AppAssets.icTrash
                icon.color: Colors.error
                background: Rectangle{
                    color: "transparent"
                }
                onClicked: {
                    root.resetRecoding()
                    root.viewmodel.cancelVoiceRecording()
                }
            }

            Button {
                id: btnPlayVoice
                objectName: "voicePlayButton"
                Layout.preferredHeight: 35
                Layout.preferredWidth:  35
                icon.source: root.viewmodel && root.viewmodel.voicePreviewPlaying
                             ? AppAssets.icPause
                             : AppAssets.icPlay
                enabled: root.viewmodel && root.viewmodel.voicePreviewReady
                background: Rectangle {
                    color: "transparent"
                }

                onClicked: {
                    root.viewmodel.playVoicePreview()
                    voiceSlider.showHandler = true
                }
            }

            MySlider {
                id: voiceSlider

                Layout.fillWidth: true
                Layout.preferredHeight: 24

                from: 0
                to: Math.max(1, root.viewmodel ? root.viewmodel.voicePreviewDuration : 0)
                value: root.viewmodel ? root.viewmodel.voicePreviewPosition : 0

                enabled: root.viewmodel && root.viewmodel.voicePreviewSeekable
                hoverEnabled: true
                showHandler: false

                onMoved: root.viewmodel.seekVoicePreview(value)
            }

            Text {
                objectName: "voicePreviewStatus"
                Layout.maximumWidth: 150
                elide: Text.ElideRight
                text: voiceSendStatus.text.length > 0
                      ? voiceSendStatus.text
                      : (root.viewmodel && root.viewmodel.voicePreviewError.length > 0)
                      ? qsTr("Playback unavailable")
                      : (root.viewmodel ? root.viewmodel.voiceRecordingDurationText : "00:00:0")
                color: (root.viewmodel && root.voiceError.length > 0)
                       || (root.viewmodel && root.viewmodel.voicePreviewError.length > 0) ? Colors.error : Colors.textPrimary
                font.family: Typography.family
                font.pixelSize: Typography.body
                ToolTip.visible: errorHover.hovered && ToolTip.text.length > 0
                ToolTip.text: root.viewmodel && root.voiceError.length > 0
                              ? root.voiceError : (root.viewmodel ? root.viewmodel.voicePreviewError : "")
                HoverHandler { id: errorHover }
            }

            Button {
                id: btnVoiceSend
                objectName: "voiceCloseButton"
                icon.source: AppAssets.icSend
                icon.color: Colors.primary
                background: Rectangle {
                    color: "transparent"
                }

                onClicked: {
                    root.sendRecording()
                }
            }
        }
    }

    Rectangle {
        id: lockRecoder
        enabled: !root.viewmodel || !root.isSendingVoice
        visible: root._isRecoding
        width: 40
        height: 100
        x: root.width - (width + AppLayouts.x_padding)
        y: -(height + AppLayouts.x_padding)
        color: "transparent"
        Rectangle {
            id: lock
            visible: root._isRecoding
            width: 40
            height: root._isLockRecoding ? 40 : 100 - root._nearToLock
            anchors.top: parent.top
            color: Colors.suface3
            clip: true
            radius: width/2
            anchors {
                topMargin: 4
                bottomMargin: 4
            }
            ColumnLayout {
                anchors.fill: parent


                Image {
                    id: lockIcon
                    Layout.preferredHeight: 20
                    Layout.preferredWidth: 20
                    source: root._isLockRecoding ? AppAssets.icLock : AppAssets.icLockOpen
                    fillMode: Image.PreserveAspectFit
                    transformOrigin: Item.Center
                    Layout.alignment: Qt.AlignHCenter

                    SequentialAnimation {
                        id: iconChangeAnimation
                        ParallelAnimation {
                            NumberAnimation {
                                target: lockIcon
                                property: "scale"
                                to: 0.6
                                duration: 100
                                easing.type: Easing.InQuad
                            }

                            NumberAnimation {
                                target: lockIcon
                                property: "opacity"
                                to: 0
                                duration: 100
                            }

                        }

                        ScriptAction {
                            script: {
                                lockIcon.source = root._isLockRecoding
                                        ? AppAssets.icLock

                                        : AppAssets.icLockOpen
                            }

                        }

                        ParallelAnimation {
                            NumberAnimation {
                                target: lockIcon
                                property: "scale"
                                to: 1.0
                                duration: 150
                                easing.type: Easing.OutBack
                            }

                            NumberAnimation {
                                target: lockIcon
                                property: "opacity"
                                to: 1
                                duration: 100
                            }
                        }

                    }

                    Connections {
                        target: root
                        function on_IsLockRecodingChanged() {
                            iconChangeAnimation.restart()
                        }

                    }
                }
                Image {
                    visible: !root._isLockRecoding
                    Layout.preferredHeight: 18
                    Layout.preferredWidth: 18
                    source: AppAssets.icChevronUp
                    fillMode: Image.PreserveAspectFit
                    Layout.alignment: Qt.AlignHCenter
                }
            }
        }

        Button {
            id: btnPause
            width: 40
            height: 40
            visible: root._isLockRecoding
            anchors {
                top: lock.bottom
                topMargin: 8
            }
            icon.source: root._isPause ? AppAssets.icPlay : AppAssets.icPause

            clip: true
            background: Rectangle {
                color: Colors.suface3
                radius: width/2
            }

            onClicked: {
                if (root.viewmodel) {
                    if (root._isPause) {
                        if (root.viewmodel) { root.viewmodel.stopVoicePreview() }
                        root.viewmodel.resumeVoiceRecording()
                    } else {
                        root.viewmodel.pauseVoiceRecording()
                    }
                }
                root._isPause = !root._isPause
                root._isPreview = root._isPause
            }

            SequentialAnimation {
                id: pauseAnimation
                ParallelAnimation {
                    NumberAnimation {
                        target: lockIcon
                        property: "scale"
                        to: 0.6
                        duration: 100
                        easing.type: Easing.InQuad
                    }

                    NumberAnimation {
                        target: lockIcon
                        property: "opacity"
                        to: 0
                        duration: 100
                    }

                }

                ScriptAction {
                    script: {
                        lockIcon.source = root._isLockRecoding
                                ? AppAssets.icLock
                                : AppAssets.icLockOpen
                    }

                }

                ParallelAnimation {
                    NumberAnimation {
                        target: lockIcon
                        property: "scale"
                        to: 1.0
                        duration: 150
                        easing.type: Easing.OutBack
                    }

                    NumberAnimation {
                        target: lockIcon
                        property: "opacity"
                        to: 1
                        duration: 100
                    }
                }

            }
            Connections {
                target: root
                function on_IsLockRecodingChanged() {
                    pauseAnimation.restart()
                }

            }
        }
    }



































}
