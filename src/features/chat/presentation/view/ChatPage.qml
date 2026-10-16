pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

import Theme
import Features.Chat
import "components" as Components

Page {
    id: chatPage

    visible: false
    implicitHeight: AppLayouts.minWidth
    implicitWidth: AppLayouts.minWidth
    // anchors.fill: parent

    property bool isShowEmojiSide: false
    property bool isCollapsed: false
    property string receiverId
    property string selectUserName

    signal resetMessagingStateRequested
    signal needScrollToLast


    function closeEmojiSide() {
        if (!isShowEmojiSide) {
            return
        }
        isShowEmojiSide = false
        if (Window.window) {
            Window.window.width -= rigthSideContainer.width
        }
    }

    function toggleEmojiSide() {
        if (isShowEmojiSide) {
            closeEmojiSide()
            return
        }
        const window = Window.window
        if (!window || window.width <= AppLayouts.minWidth) {
            return
        }
        window.width += rigthSideContainer.width
        isShowEmojiSide = true
    }

    Connections {
        target: chatPage.Window.window
        function onWidthChanged() {
            if (chatPage.isShowEmojiSide
                    && chatPage.Window.window.width <= AppLayouts.minWidth + rigthSideContainer.width) {
                chatPage.isShowEmojiSide = false
            }
        }
    }

    function showConversation(index, userName, userId) {
        const switchId = ++d.switchId
        incomingReveal.stop()

        function activateConversation() {
            if (switchId !== d.switchId)
                return

            chatPage.resetMessagingStateRequested()
            d.itemSelectedIndex = index
            chatPage.receiverId = userId
            chatPage.selectUserName = userName
            loaderMessage.opacity = 0
            loaderMessage.x = 0
            MessagingViewModel.fetchMessage(userId)

        }

        // There is nothing useful to freeze before the first conversation.
        if (d.itemSelectedIndex === -1 || loaderMessage.width <= 0
                || loaderMessage.height <= 0) {
            transitionSnapshot.visible = false
            activateConversation()
            return
        }

        // Freeze the current conversation into one GPU-backed image. The
        // expensive delegate tree can then change underneath without flashing.
        const grabStarted = loaderMessage.grabToImage(function(result) {
            if (switchId !== d.switchId)
                return

            transitionSnapshot.source = result.url
            transitionSnapshot.opacity = 1
            transitionSnapshot.x = 0
            transitionSnapshot.visible = true
            activateConversation()
        }, Qt.size(Math.ceil(loaderMessage.width),
                   Math.ceil(loaderMessage.height)))

        // A grab can fail while the window is being resized or hidden. Do not
        // let that prevent the actual conversation switch.
        if (!grabStarted) {
            transitionSnapshot.visible = false
            activateConversation()
        }
    }

    function collapseUserList() {
        if (d.dragging)
            return;
        isCollapsed = true;
        d.sidebarWidth = d.collapsedWidth;
    }

    function expandUserList() {
        if (d.dragging)
            return;
        isCollapsed = false;
        d.sidebarWidth = Math.min(d.expandedWidth, d.maximumSidebarWidth);
        if (d.sidebarWidth < 200) {
            d.sidebarWidth = d.minimumExpandedWidth;
        }
    }

    onVisibleChanged: {
        if (visible) {
            ConversationsVM.fetchConversations();
        }
    }

    Connections {
        target: MessagingViewModel
        function onMessageChanged() {
            chatPage.needScrollToLast()
        }
    }

    QtObject {
        id: d

        readonly property int userItemHeight: 60
        readonly property int collapsedWidth: userItemHeight + (AppLayouts.s_padding * 2)
        readonly property int minimumExpandedWidth: 250
        readonly property int collapseThreshold: minimumExpandedWidth
        readonly property int expandPullDistance: 30
        readonly property int messageMinimumWidth: 300
        readonly property int maximumSidebarWidth: Math.max(minimumExpandedWidth, chatPage.width - messageMinimumWidth
                                                            - (chatPage.isShowEmojiSide ? rigthSideContainer.width : 0))

        property real sidebarWidth: minimumExpandedWidth
        property real expandedWidth: minimumExpandedWidth
        property real pressSceneX: 0
        property real pressWidth: 0
        property bool dragging: false
        property bool dragStartedCollapsed: false

        property int itemSelectedIndex: -1
        property int switchId: 0

        Behavior on sidebarWidth {
            enabled: !d.dragging
            NumberAnimation {
                duration: 220
                easing.type: Easing.OutCubic
            }
        }
    }

    // >>>>>> conversation
    Loader {
        id: conversationLoader
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        width: d.sidebarWidth
        sourceComponent: { ConversationsVM.isLoading
                           ? conversationLoading
                           : conversationList
        }

    }
    Component {
        id: conversationList

        ListUser {
            id: userList
            property int selectedIndex: -1
            property string userName : ""
            property string userId: ""

            previousIndex: d.itemSelectedIndex
            itemHeight: d.userItemHeight
            padding: AppLayouts.s_padding
            isCompactMode: width < d.minimumExpandedWidth
            model: ConversationsVM
            onExpandRequested: chatPage.expandUserList()
            onItemClicked: function (index, userName, userId) {
                if (index === d.itemSelectedIndex) {
                    return
                }
                userList.selectedIndex = index
                userList.userName = userName
                userList.userId = userId
                if (MessagingViewModel.voiceController.isVoiceRecording || MessagingViewModel.voiceController.isVoiceRecordingPaused) {
                    alertPopup.open()
                    return;
                }
                d.itemSelectedIndex = index
                chatPage.showConversation(index, userName, userId)
            }
            Connections {
                target: alertPopup

                function onDiscardRequested() {
                    d.itemSelectedIndex = userList.selectedIndex
                    chatPage.showConversation(userList.selectedIndex, userList.userName, userList.userId)
                }
            }
        }

    }

    Popup {
        id: alertPopup

        anchors.centerIn: parent
        width: 320

        padding: 16
        modal: true
        focus: true

        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        signal discardRequested()

        background: Rectangle {
            radius: 12
            color: Colors.suface1
            border.width: 1
            border.color: Colors.suface3
        }

        contentItem: ColumnLayout {
            spacing: 20

            Text {
                Layout.fillWidth: true

                text: "Please stop or discard your voice recording before switching conversations."
                wrapMode: Text.WordWrap

                color: Colors.textPrimary
                font.family: Typography.family
                font.pixelSize: Typography.title4
                font.weight: Typography.regular
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 36
                spacing: 4

                Item {
                    Layout.fillWidth: true
                }

                Button {
                    id: cancelButton
                    Layout.preferredHeight: 36

                    contentItem: Text {
                        text: "Cancel"
                        color: Colors.textPrimary
                        font.family: Typography.family

                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    background: Rectangle {
                        radius: 6
                        color: cancelButton.pressed
                               ? Colors.suface3
                               : cancelButton.hovered
                                 ? Colors.suface2
                                 : "transparent"
                    }

                    onClicked: alertPopup.close()
                }

                Button {
                    id: discardButton
                    Layout.preferredHeight: 36

                    contentItem: Text {
                        text: "Discard"
                        color: Colors.error
                        font.family: Typography.family

                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    background: Rectangle {
                        radius: 6
                        color: discardButton.pressed
                               ? Colors.suface3
                               : discardButton.hovered
                                 ? Colors.suface2
                                 : "transparent"
                    }

                    onClicked: {
                        alertPopup.close()
                        alertPopup.discardRequested()
                    }
                }
            }
        }
    }

    Component {
        id: conversationLoading
        Rectangle {
            color: Colors.background

            Text {
                text: "Loading"
                font.family: Typography.family
                font.pixelSize: Typography.title4
                font.weight: Typography.medium
                color: Colors.textPrimary
                anchors.centerIn: parent
            }
        }
    }

    // -- Split Handle ---
    Rectangle {
        id: splitHandle

        anchors.top: parent.top
        anchors.bottom: parent.bottom
        x: d.sidebarWidth
        width: 1
        color: handleMouseArea.containsMouse || handleMouseArea.pressed ? Colors.primary : Colors.primary600

        Behavior on color {
            ColorAnimation {
                duration: 150
            }
        }

        MouseArea {
            id: handleMouseArea

            anchors.fill: parent
            anchors.leftMargin: -6
            anchors.rightMargin: -6
            hoverEnabled: true
            cursorShape: Qt.SizeHorCursor

            onPressed: mouse => {
                d.dragging = true;
                d.dragStartedCollapsed = chatPage.isCollapsed;
                d.pressSceneX = splitHandle.mapToItem(chatPage, mouse.x, mouse.y).x;
                d.pressWidth = d.sidebarWidth;
            }

            onPositionChanged: mouse => {
                if (!pressed)
                    return;
                const sceneX = splitHandle.mapToItem(chatPage, mouse.x, mouse.y).x;
                const requestedWidth = d.pressWidth + sceneX - d.pressSceneX;
                d.sidebarWidth = Math.max(d.collapsedWidth, Math.min(requestedWidth, d.maximumSidebarWidth));
            }

            onReleased: {
                d.dragging = false;

                const expandPullReached = d.dragStartedCollapsed && d.sidebarWidth - d.pressWidth >= d.expandPullDistance;

                if (expandPullReached) {
                    chatPage.isCollapsed = false;
                    d.sidebarWidth = Math.max(d.minimumExpandedWidth, d.sidebarWidth);
                    d.expandedWidth = d.sidebarWidth;
                } else if (d.sidebarWidth < d.collapseThreshold) {
                    chatPage.isCollapsed = true;
                    d.sidebarWidth = d.collapsedWidth;
                } else {
                    chatPage.isCollapsed = false;
                    d.expandedWidth = d.sidebarWidth;
                }
            }

            onCanceled: {
                d.dragging = false;
                d.sidebarWidth = chatPage.isCollapsed ? d.collapsedWidth : d.expandedWidth;
            }
        }
    }
    onWidthChanged: {
        if (!d.dragging && d.sidebarWidth > d.maximumSidebarWidth)
            d.sidebarWidth = d.maximumSidebarWidth;
    }


    // -- Messaging Loader
    StackLayout {
        id: messageLoader

        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: splitHandle.right
        anchors.right: chatPage.isShowEmojiSide ? rigthSideContainer.left : parent.right

        currentIndex: d.itemSelectedIndex != -1 ? 1 : 0

        // sourceComponent: d.itemSelectedIndex != -1 ? messagingList : messagingEmpty
        MessagingEmpty {
            // anchors.fill: parent
        }

        Loader {
            id: loaderMessage

            opacity: 0
            active: d.itemSelectedIndex !== -1
            sourceComponent: Component {
                ListMessaging {
                    id: messagingView

                    // anchors.fill: parent
                    isShowEmojiSide: chatPage.isShowEmojiSide
                    onEmojiSideToggleRequested: chatPage.toggleEmojiSide()
                    viewmodel: MessagingViewModel
                    userName: chatPage.selectUserName
                    onConversationReady: incomingReveal.restart()
                    onCallRequested: {
                        if (chatPage.receiverId) {
                            MessagingViewModel.requestCall(chatPage.receiverId, chatPage.selectUserName)
                        }
                    }
                    isVisible: !MessagingViewModel.isLoading
                    onMessageSubmitted: msg => {
                        MessagingViewModel.sendMessage(chatPage.receiverId, msg);
                    }
                    onDeleteSelectedRequested:(indexes) => {
                                                  console.log("Delete Message",indexes);
                                                  MessagingViewModel.deleteMessage(indexes)
                                              }

                    Connections {
                        target: chatPage
                        function onResetMessagingStateRequested() {
                            messagingView.resetState();
                        }
                        function onNeedScrollToLast() {
                            console.log("Message changed")
                            messagingView.scrollMessageToLast();
                        }
                    }
                }
            }
        }


    }

    Components.RigthSideContainer {
        id: rigthSideContainer
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        isShow: chatPage.isShowEmojiSide
    }

    Image {
        id: transitionSnapshot

        anchors.fill: messageLoader
        visible: false
        fillMode: Image.Stretch
        cache: false
        z: 10
    }

    ParallelAnimation {
        id: incomingReveal

        NumberAnimation {
            target: loaderMessage
            property: "opacity"
            to: 1
            duration: 180
            easing.type: Easing.OutCubic
        }

        NumberAnimation {
            target: transitionSnapshot
            property: "opacity"
            to: 0
            duration: 170
            easing.type: Easing.OutCubic
        }
        onFinished: {
            transitionSnapshot.visible = false
            transitionSnapshot.source = ""
            transitionSnapshot.x = 0
        }
    }

}
