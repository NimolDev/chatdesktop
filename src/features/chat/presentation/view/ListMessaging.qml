pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

import Theme
import Shared.UI
import "components" as Components

Item {
    id: root
    // anchors.fill: parent
    implicitWidth: AppLayouts.minWidth
    implicitHeight: AppLayouts.minHeight

    property var viewmodel
    property bool isVisible
    readonly property bool readyToReveal: messageBody.initialPositionReady
                                          && viewmodel && !viewmodel.isLoading
                                          && (chatBackground.status === Image.Ready
                                              || chatBackground.status === Image.Error)
    signal conversationReady()
    onReadyToRevealChanged: {
        if (readyToReveal) {
            conversationReady()
        }
    }
    property string userName: "User"
    property string userStatus: "Last Seen Recently"
    property bool isOnline: false
    property bool isShowEmojiSide: false

    property var selectedMessages: ({})
    property int selectionRevision: 0
    property int selectedCount: 0
    readonly property bool selectionMode: selectedCount > 0 || messageBody.dragSelecting

    signal emojiSideToggleRequested()
    signal messageSubmitted(string text)
    signal filesDropped(var files)
    signal callRequested
    signal videoRequested
    signal moreRequested
    signal forwardSelectedRequested
    signal deleteSelectedRequested(var rows)

    function isMessageSelected(messageIndex) {
        return selectedMessages[messageIndex] === true;
    }

    function setMessageSelected(messageIndex, selected) {
        const wasSelected = selectedMessages[messageIndex] === true;
        if (wasSelected === selected)
            return;
        if (selected)
            selectedMessages[messageIndex] = true;
        else
            delete selectedMessages[messageIndex];

        selectedCount += selected ? 1 : -1;
        selectionRevision++;
    }

    function clearSelection() {
        messageBody.clearDragSelection();
        selectedMessages = ({});
        selectedCount = 0;
        selectionRevision++;
    }

    function resetState() {
        clearSelection();
        messageMenu.close();
        composer.resetState();
        filePopup.close();

    }
    function scrollMessageToLast() {
        messageBody.scrollToLatest()
    }


    Image {
        id: chatBackground
        anchors {
            top:parent.top
            bottom: parent.bottom
            left: parent.left
            right: parent.right
        }

        source: root.visible ? "qrc:/background/background.png" : ""
        fillMode: Image.Tile
        sourceSize: Qt.size(512, 512)
        smooth: false
        mipmap: true
        asynchronous: true
        cache: true
    }

    // ------ Voice Playback status  -------
    Rectangle {
        id: voicePlayStatus
        property bool opened: root.viewmodel
                              ? root.viewmodel.voiceController.isInPlay
                              : false

        // visible: root.viewmodel.voiceController.isInPlay
        height: 30
        color: Colors.suface1
        anchors {
            top:parent.top
            left: parent.left
            right: parent.right
        }
        z:1
        transform: Translate {
            id: voicePlayTrnaslate
            y: voicePlayStatus.opened ? 0 : -voicePlayStatus.height
            Behavior on y {
               NumberAnimation {
                   duration: 200
                   // easing.type: Easing.OutCubic
               }
            }
        }
        opacity: 1
        // visible: opacity > 0

        Behavior on opacity {
            NumberAnimation {
                duration: 200
                // easing.type: Easing.OutCubic
            }
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: AppLayouts.x_padding
            anchors.rightMargin: AppLayouts.x_padding
            spacing: 12
            Button {
                id: btnPlay
                Layout.preferredHeight: 20
                // Layout.preferredWidth: 20
                padding: 2
                icon.source:root.viewmodel
                            ? (root.viewmodel.voiceController.voicePlaybackPlaying
                               ? AppAssets.icPause : AppAssets.icPlay)
                            : AppAssets.icPause
                hoverEnabled: true
                icon.color:  Colors.primary
                background: Rectangle {
                    color: "transparent"
                }
                onClicked: {
                    if (root.viewmodel) {
                        if (root.viewmodel.voiceController.voicePlaybackPlaying) {
                        root.viewmodel.voiceController.pauseVoice()
                        } else {
                            root.viewmodel.voiceController.resumeVoice()
                        }
                    }
                }
                HoverHandler {
                    cursorShape: Qt.PointingHandCursor
                }
            }
            Item {
                Layout.fillWidth: true
            }

            Text {
                id: txtDuration
                text: root.viewmodel
                      ? root.viewmodel.voiceController.voicePlaybackDurationText
                      : "00:00"
                color: Colors.textSecond
                font.family: Typography.family
                font.pixelSize: Typography.body
                font.weight: Typography.regular
            }
            Button {
                id: btnPlaybackSpeed

                property real selectedSpeed: 1.0

                function closeWhenNotHovered() {
                    if (!btnPlaybackSpeed.hovered && !playbackPopupHover.hovered) {
                        playbackSpeedPopup.close()
                    }
                }

                text: selectedSpeed.toFixed(1) + "X"
                hoverEnabled: true
                background: Rectangle {
                    color: "transparent"
                }
                onClicked: playbackSpeedPopup.open()
                onHoveredChanged: {
                    if (hovered) {
                        playbackSpeedPopup.open()
                    } else {
                        Qt.callLater(btnPlaybackSpeed.closeWhenNotHovered)
                    }
                }

                Popup {
                    id: playbackSpeedPopup

                    width: 100
                    height: speedOptions.implicitHeight + padding * 2
                    x: btnPlaybackSpeed.width - width
                    y: btnPlaybackSpeed.height
                    padding: 4
                    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
                    background: Rectangle {
                        color: Colors.suface1
                        radius: AppLayouts.x_radius
                        border.color: Colors.suface3
                    }

                    contentItem: Column {
                        id: speedOptions
                        spacing: 2

                        HoverHandler {
                            id: playbackPopupHover
                            margin: playbackSpeedPopup.padding
                            onHoveredChanged: {
                                if (!hovered) {
                                    Qt.callLater(btnPlaybackSpeed.closeWhenNotHovered)
                                }
                            }
                        }
                        Repeater {
                            model: [0.5, 1.0, 1.2, 1.5, 1.7, 2.0]

                            delegate: Button {
                                id: speedOption
                                required property real modelData

                                width: speedOptions.width
                                height: 32
                                text: modelData.toFixed(1) + "X"
                                hoverEnabled: true
                                highlighted: btnPlaybackSpeed.selectedSpeed === modelData

                                contentItem: Text {
                                    text: speedOption.text
                                    color: speedOption.highlighted ? Colors.primary : Colors.textPrimary
                                    font.family: Typography.family
                                    font.pixelSize: Typography.body
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }
                                background: Rectangle {
                                    radius: AppLayouts.x_radius
                                    color: speedOption.hovered || speedOption.highlighted
                                           ? Colors.suface3 : "transparent"
                                }
                                onClicked: {
                                    btnPlaybackSpeed.selectedSpeed = modelData
                                    playbackSpeedPopup.close()
                                    if (root.viewmodel) {
                                        root.viewmodel.voiceController.setPlaybackRate(modelData)
                                    }
                                }
                                HoverHandler {
                                    cursorShape: Qt.PointingHandCursor
                                }
                            }
                        }
                    }
                }
            }


            Button {
                id: btnClosePlayback
                Layout.preferredHeight: 20
                Layout.preferredWidth: 20
                padding: 2
                icon.source: AppAssets.icClose
                icon.color: Colors.primary
                hoverEnabled: true
                background: Rectangle {
                    color: "transparent"
                }
                onClicked: {
                    if (root.viewmodel) {
                        root.viewmodel.voiceController.cancelVoicePlayback()
                    }
                }
                HoverHandler {
                    cursorShape: Qt.PointingHandCursor
                }
            }
        }

    }

    Components.MessagingToolbar {
        id: toolbar
        // visible: false
        anchors {
            // top: voicePlayStatus.visible ? voicePlayStatus.bottom : parent.top
            left: parent.left
            right: parent.right
        }
        z: 1
        states: [
            State {
                name: "expanded"
                when: voicePlayStatus.opened
                AnchorChanges {
                    target: toolbar
                    anchors.top: voicePlayStatus.bottom
                }
            },
            State {
                name: "collapsed"
                when: !voicePlayStatus.opened
                AnchorChanges {
                    target: toolbar
                    anchors.top: parent.top
                }
            }
        ]
        transitions: [
            Transition {
                from: "collapsed"
                to: "expanded"
                AnchorAnimation {
                    duration: 200
                    // easing.type: Easing.OutCubic
                }

            },
            Transition {
                from: "expanded"

                to: "collapsed"
                AnchorAnimation {
                    duration: 200
                    // easing.type: Easing.InCubic
                }

            }
        ]

        userName: root.userName
        userStatus: root.userStatus
        isOnline: root.isOnline
        selectionMode: root.selectionMode
        selectedCount: root.selectedCount

        onAudioClicked: root.callRequested()
        onVideoClicked: root.videoRequested()
        onMoreClicked: root.moreRequested()
        onForwardClicked: root.forwardSelectedRequested()
        onDeleteClicked: {
            const indexes = Object.keys(root.selectedMessages)
            .filter(key => root.selectedMessages[key])
            .map(Number)
            .sort((a, b) => b - a);
            root.deleteSelectedRequested(indexes);
            root.clearSelection()
        }
        onCancelClicked: root.clearSelection()
    }

    Components.MessagingBody {
        id: messageBody

        anchors {
            top: toolbar.bottom
            left: parent.left
            right: parent.right
            bottom: composer.top
        }

        model: root.viewmodel
        isVisible: root.isVisible
        selectionMode: root.selectionMode
        selectionRevision: root.selectionRevision
        selectionLookup: index => root.isMessageSelected(index)
        contextMenuOpen: messageMenu.opened

        onSelectionRequested: (index, selected) => {
            root.setMessageSelected(index, selected);
        }
        onContextMenuRequested: (index, selectOnly, mouseX, mouseY) => {
            const position = messageBody.mapToItem(messageMenu.parent, mouseX, mouseY);
            messageMenu.showForMessage(index, selectOnly, position);
        }
    }

    Components.MessageComposer {
        id: composer
        viewmodel: root.viewmodel ? root.viewmodel.voiceController : null
        messagingViewModel: root.viewmodel

        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }

        emojiSideVisible: root.isShowEmojiSide

        onSubmitted: text => {
            root.messageSubmitted(text);
            messageBody.scrollToLatest();
        }
        onSendVoice: (duration) => {
                         root.viewmodel.sendVoice(duration)
                     }

        onEmojiSideToggleRequested: root.emojiSideToggleRequested()

        // onFilePicker: urls => {

        //                   var files = []
        //                   for (var i = 0; i < urls.length; i++) {
        //                       files.push({url: String(urls[i]), compress: false})
        //                   }
        //                   console.log("File Picker" , files[0]["compress"])
        //                   if (root.viewmodel) {
        //                       root.viewmodel.addAttachments(urls)
        //                   }
        //                   filePopup.files = filePopup.files.concat(files)
        //                   filePopup.compress = true
        //                   filePopup.open()
        //                   root.filesDropped(files)
        //               }




        onImageAndVideoFileSelected: urls => {
                                         var files = []
                                         for (var i = 0; i < urls.length; i++) {
                                             files.push({url: String(urls[i]), compress: true})
                                         }
                                         console.log("File Picker" , files[0]["compress"])
                                         if (root.viewmodel) {
                                             root.viewmodel.addAttachments(urls)
                                         }
                                         filePopup.files = filePopup.files.concat(files)
                                         filePopup.compress = true
                                         filePopup.open()
                                         root.filesDropped(files)
                                     }
        onFilesSelected: urls => {
                            var files = []
                            for (var i = 0; i < urls.length; i++) {
                                files.push({url: String(urls[i]), compress: false})
                            }
                            console.log("File Picker" , files[0]["compress"])
                            if (root.viewmodel) {
                                root.viewmodel.addAttachments(urls)
                            }
                            filePopup.files = filePopup.files.concat(files)
                            filePopup.compress = false
                            filePopup.open()
                            root.filesDropped(files)
                        }
    }

    Components.MessageContextMenu {
        id: messageMenu
        onSelectRequested: index => root.setMessageSelected(index, true)
    }

    Components.FileDropOverlay {
        // Render above the modal popup, but only accept drops inside this conversation.
        parent: Overlay.overlay
        readonly property rect conversationFrame: root.mapToItem(parent, 0, 0, root.width, root.height)
        x: conversationFrame.x
        y: conversationFrame.y
        width: conversationFrame.width
        height: conversationFrame.height
        visible: root.visible
        enabled: root.enabled
        z: 2147483647
        onFilesDropped: (files, compress) => {
                            // if (filePopup.uploading) {
                            //     return
                            // }
                            const window = root.Window.window
                            if (window) {
                                window.raise()
                                window.requestActivate()
                            }
                            const fileCount = filePopup.files.length + files.length
                            if (files.length === 0 || fileCount > 5) {
                                return
                            }

                            const urls = files.map(file => file.url)
                            .filter(url => url !== undefined && url !== null && url !== "")
                            if (root.viewmodel) {
                                root.viewmodel.addAttachments(urls)
                            }

                            filePopup.files = filePopup.files.concat(files)
                            filePopup.compress = compress
                            root.filesDropped(files)
                            filePopup.open()
                        }

    }

    Components.AttachmentUploadDialog {
        id: filePopup
        viewmodel: root.viewmodel
        onSend: (urls, caption, compress) => {
                    if (root.viewmodel) {
                        if (compress) {
                            root.viewmodel.sendImage(caption)
                        } else {
                            root.viewmodel.sendFile(caption)
                        }
                        root.viewmodel.resetAttachments()
                    }
                }
    }
}
