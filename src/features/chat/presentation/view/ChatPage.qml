pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

import Theme
import Features.Chat

Page {
    id: chatPage

    visible: false
    implicitHeight: AppLayouts.minWidth
    implicitWidth: AppLayouts.minWidth
    // anchors.fill: parent

    property bool isCollapsed: false
    property string receiverId
    property string selectUserName

    signal resetMessagingStateRequested
    signal needScrollToLast

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
            messagingView.opacity = 1
            messagingView.x = 0
            MessagingViewModel.fetchMessage(userId)
        }

        // There is nothing useful to freeze before the first conversation.
        if (d.itemSelectedIndex === -1 || messagingView.width <= 0
                || messagingView.height <= 0) {
            transitionSnapshot.visible = false
            activateConversation()
            return
        }

        // Freeze the current conversation into one GPU-backed image. The
        // expensive delegate tree can then change underneath without flashing.
        const grabStarted = messagingView.grabToImage(function(result) {
            if (switchId !== d.switchId)
                return

            transitionSnapshot.source = result.url
            transitionSnapshot.opacity = 1
            transitionSnapshot.x = 0
            transitionSnapshot.visible = true
            activateConversation()
        }, Qt.size(Math.ceil(messagingView.width),
                   Math.ceil(messagingView.height)))

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
            console.log("ChatPage Visible");
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
        readonly property int maximumSidebarWidth: Math.max(minimumExpandedWidth, chatPage.width - messageMinimumWidth)

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

    ListUser {
        id: userList

        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        width: d.sidebarWidth
        itemHeight: d.userItemHeight
        padding: AppLayouts.s_padding
        isCompactMode: width < d.minimumExpandedWidth
        model: ConversationsVM
        onExpandRequested: chatPage.expandUserList()
        onItemClicked: function (index, userName, userId) {
            if (index === d.itemSelectedIndex) {
                return
            }
            chatPage.showConversation(index, userName, userId)
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
        anchors.right: parent.right

        currentIndex: d.itemSelectedIndex != -1 ? 1 : 0

        // sourceComponent: d.itemSelectedIndex != -1 ? messagingList : messagingEmpty
        MessagingEmpty {
            // anchors.fill: parent
        }

        ListMessaging {
            id: messagingView

            // anchors.fill: parent
            model: MessagingViewModel
            userName: chatPage.selectUserName
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

    Image {
        id: transitionSnapshot

        anchors.fill: messageLoader
        visible: false
        fillMode: Image.Stretch
        cache: false
        z: 10
    }

    Connections {
        target: MessagingViewModel

        function onIsLoadingChanged() {
            if (MessagingViewModel.isLoading || !transitionSnapshot.visible)
                return

            // Let ListView instantiate and position the incoming delegates
            // before removing the frozen outgoing conversation.
            Qt.callLater(function() {
                messagingView.scrollMessageToLast()
                incomingReveal.restart()
            })
        }
    }

    ParallelAnimation {
        id: incomingReveal

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
