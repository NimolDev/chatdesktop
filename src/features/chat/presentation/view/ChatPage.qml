pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic

import Theme

Page {
    id: chatPage

    implicitHeight: AppLayouts.minWidth
    implicitWidth: AppLayouts.minWidth
    // anchors.fill: parent

    property bool isCollapsed: false

    function collapseUserList() {
        if (d.dragging)
            return

        isCollapsed = true
        d.sidebarWidth = d.collapsedWidth
    }

    function expandUserList() {
        if (d.dragging)
            return

        isCollapsed = false
        d.sidebarWidth = Math.min(d.expandedWidth, d.maximumSidebarWidth)
        if (d.sidebarWidth < 200 ) {
            d.sidebarWidth = d.minimumExpandedWidth
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
        readonly property int maximumSidebarWidth: Math.max(
                                                       minimumExpandedWidth,
                                                       chatPage.width - messageMinimumWidth)

        property real sidebarWidth: minimumExpandedWidth
        property real expandedWidth: minimumExpandedWidth
        property real pressSceneX: 0
        property real pressWidth: 0
        property bool dragging: false
        property bool dragStartedCollapsed: false

        property int itemSelectedIndex: -1

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
        onExpandRequested: chatPage.expandUserList()
        onItemClicked: function(index) {
            console.log("Clicked: ", index)
            d.itemSelectedIndex = index
        }
    }

    // -- Split Handle ---
    Rectangle {
        id: splitHandle

        anchors.top: parent.top
        anchors.bottom: parent.bottom
        x: d.sidebarWidth
        width: 1
        color: handleMouseArea.containsMouse || handleMouseArea.pressed
               ? Colors.primary : Colors.primary600

        Behavior on color {
            ColorAnimation { duration: 150 }
        }

        MouseArea {
            id: handleMouseArea

            anchors.fill: parent
            anchors.leftMargin: -6
            anchors.rightMargin: -6
            hoverEnabled: true
            cursorShape: Qt.SizeHorCursor

            onPressed: (mouse) => {
                d.dragging = true
                d.dragStartedCollapsed = chatPage.isCollapsed
                d.pressSceneX = splitHandle.mapToItem(chatPage, mouse.x, mouse.y).x
                d.pressWidth = d.sidebarWidth
            }

            onPositionChanged: (mouse) => {
                if (!pressed)
                    return

                const sceneX = splitHandle.mapToItem(chatPage, mouse.x, mouse.y).x
                const requestedWidth = d.pressWidth + sceneX - d.pressSceneX
                d.sidebarWidth = Math.max(
                            d.collapsedWidth,
                            Math.min(requestedWidth, d.maximumSidebarWidth))
            }

            onReleased: {
                d.dragging = false

                const expandPullReached = d.dragStartedCollapsed
                        && d.sidebarWidth - d.pressWidth >= d.expandPullDistance

                if (expandPullReached) {
                    chatPage.isCollapsed = false
                    d.sidebarWidth = Math.max(d.minimumExpandedWidth,
                                              d.sidebarWidth)
                    d.expandedWidth = d.sidebarWidth
                } else if (d.sidebarWidth < d.collapseThreshold) {
                    chatPage.isCollapsed = true
                    d.sidebarWidth = d.collapsedWidth
                } else {
                    chatPage.isCollapsed = false
                    d.expandedWidth = d.sidebarWidth
                }
            }

            onCanceled: {
                d.dragging = false
                d.sidebarWidth = chatPage.isCollapsed
                        ? d.collapsedWidth : d.expandedWidth
            }
        }
    }
    onWidthChanged: {
        if (!d.dragging && d.sidebarWidth > d.maximumSidebarWidth)
            d.sidebarWidth = d.maximumSidebarWidth
    }

    // -- Messaging Loader
    Loader {
        id: messageLoader

        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: splitHandle.right
        anchors.right: parent.right

        sourceComponent: d.itemSelectedIndex != -1 ? messagingList : messagingEmpty
    }

    Component {
        id: messagingEmpty
        MessagingEmpty {
            anchors.fill: parent
        }
    }
    Component {
        id: messagingList
        ListMessaging {
            anchors.fill: parent
        }
    }
}
