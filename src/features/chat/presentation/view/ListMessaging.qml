pragma ComponentBehavior: Bound

import QtQuick

import Theme
import "components" as Components

Item {
    id: root

    implicitWidth: AppLayouts.minWidth
    implicitHeight: AppLayouts.minHeight
    anchors.fill: parent

    property var model
    property bool isVisible
    property string userName: "User"
    property string userStatus: "Last Seen Recently"
    property bool isOnline: false
    property bool isShowEmojiSide: false

    property var selectedMessages: ({})
    property int selectionRevision: 0
    property int selectedCount: 0
    readonly property bool selectionMode: selectedCount > 0 || messageBody.dragSelecting

    signal messageSubmitted(string text)
    signal filesDropped(var urls, bool compress)
    signal callRequested
    signal videoRequested
    signal moreRequested
    signal forwardSelectedRequested
    signal deleteSelectedRequested

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

        if (isShowEmojiSide) {
            isShowEmojiSide = false;
            if (Window.window)
                Window.window.width -= rigthSideContainer.width;
        }
    }

    Connections {
        target: root.Window.window
        function onWidthChanged() {
            console.log("Width changed", root.Window.window.width);
            console.log("Min: ", AppLayouts.minWidth + rigthSideContainer.width);
            if (root.Window.window.width <= AppLayouts.minWidth + rigthSideContainer.width && root.isShowEmojiSide) {
                root.isShowEmojiSide = false;
                // Window.window.width -= emojiSide.width
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        color: Colors.background
    }

    Components.MessagingToolbar {
        id: toolbar

        anchors {
            top: parent.top
            left: parent.left
            right: root.isShowEmojiSide ? rigthSideContainer.left : parent.right
            leftMargin: AppLayouts.x_padding
            rightMargin: AppLayouts.x_padding
        }

        userName: root.userName
        userStatus: root.userStatus
        isOnline: root.isOnline
        selectionMode: root.selectionMode
        selectedCount: root.selectedCount

        onAudioClicked: root.callRequested()
        onVideoClicked: root.videoRequested()
        onMoreClicked: root.moreRequested()
        onForwardClicked: root.forwardSelectedRequested()
        onDeleteClicked: root.deleteSelectedRequested()
        onCancelClicked: root.clearSelection()
    }

    Components.MessagingBody {
        id: messageBody

        anchors {
            top: toolbar.bottom
            left: parent.left
            right: root.isShowEmojiSide ? rigthSideContainer.left : parent.right
            bottom: composer.top
        }

        // model: MessagingViewModel
        model: root.model
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

        anchors {
            left: parent.left
            right: root.isShowEmojiSide ? rigthSideContainer.left : parent.right
            bottom: parent.bottom
        }

        emojiSideVisible: root.isShowEmojiSide

        onSubmitted: text => {
            root.messageSubmitted(text);
            messageBody.scrollToLatest();
        }
        onEmojiSideToggleRequested: {
            if (root.Window.window.width <= AppLayouts.minWidth) {
                return;
            }

            root.isShowEmojiSide = !root.isShowEmojiSide;

            if (root.isShowEmojiSide) {
                Window.window.width += rigthSideContainer.width;
            } else {
                Window.window.width -= rigthSideContainer.width;
            }
        }
        onHeightDelta: delta => messageBody.adjustForComposerHeight(delta)
    }

    Components.RigthSideContainer {
        id: rigthSideContainer

        anchors {
            top: parent.top
            right: parent.right
            bottom: parent.bottom
        }

        isShow: root.isShowEmojiSide
    }

    Components.MessageContextMenu {
        id: messageMenu
        onSelectRequested: index => root.setMessageSelected(index, true)
    }

    Components.FileDropOverlay {
        anchors.fill: parent
        onFilesDropped: (urls, compress) => root.filesDropped(urls, compress)
    }
}
