pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Controls

import Theme

Item {
    id: body

    property var model

    property bool isVisible

    property bool selectionMode: false
    property int selectionRevision: 0
    property var selectionLookup: null
    property bool contextMenuOpen: false

    readonly property alias dragSelecting: listView.dragSelecting

    signal selectionRequested(int messageIndex, bool selected)
    signal contextMenuRequested(int messageIndex, bool selectOnly,
                                real mouseX, real mouseY)

    function isMessageSelected(messageIndex) {
        // Keep delegate bindings reactive when the selection map is mutated.
        const revision = selectionRevision
        return selectionLookup ? selectionLookup(messageIndex) : false
    }

    function clearDragSelection() {
        listView.dragSelecting = false
        listView.dragSelectValue = false
    }

    function adjustForComposerHeight(delta) {
        Qt.callLater(() => {
            listView.contentY = listView.clampContentY(listView.contentY + delta)
        })
    }

    function scrollToLatest() {
        if (listView.count > 0) {
            scrollToLatestTimer.restart()
        }
    }

    Image {
        anchors.fill: parent
        source: body.visible ? "qrc:/background/background.png" : ""
        fillMode: Image.Tile
        sourceSize: Qt.size(512, 512)
        smooth: false
        mipmap: true
        asynchronous: true
        cache: true
    }

    ListView {
        id: listView
        anchors.fill: parent
        clip: true
        visible: true

        property bool dragSelecting: false
        property bool dragSelectValue: true
        readonly property real minimumContentY: originY
        readonly property real maximumContentY: Math.max(
            minimumContentY,
            originY + contentHeight + listView.bottomMargin - height
        )

        property int loadMoreThreshold: 200
        property bool paginationPending: false
        property int paginationAnchorIndex: -1
        property real paginationAnchorOffset: 0

        // Keep short conversations at the bottom of the chat area. Unlike a
        // BottomToTop ListView, this preserves the normal order of section
        // headers (header first, then its messages).
        anchors.topMargin: Math.max(
            0,
            body.height - contentHeight - bottomMargin
        )
        // bottomMargin: 20
        footer: Item {
                width: listView.width
                height: 16
            }


        spacing: 2
        // Keep the outgoing conversation rendered while the next one loads.
        // ChatPage freezes it into a transition layer before this model resets.
        model: body.model
        interactive: false

        ScrollBar.vertical: ScrollBar {
            id: verticalScrollBar

            policy: ScrollBar.AsNeeded
        }

        Timer {
            id: scrollIndicatorTimer

            interval: 600
            onTriggered: verticalScrollBar.active = false
        }


        function clampContentY(value) {
            return Math.max(
                minimumContentY,
                Math.min(value, maximumContentY)
            )
        }

        function capturePaginationAnchor() {
            paginationAnchorIndex = -1
            paginationAnchorOffset = 0

            for (let index = 0; index < count; ++index) {
                const item = itemAtIndex(index)
                if (item && item.y + item.height >= contentY) {
                    paginationAnchorIndex = index
                    paginationAnchorOffset = item.y - contentY
                    return
                }
            }
        }

        // -------------------------
        // Sections
        // -------------------------

        section.property: "section"
        section.criteria: ViewSection.FullString
        section.labelPositioning: ViewSection.InlineLabels
                                  | ViewSection.CurrentLabelAtStart

        section.delegate: Rectangle {
            id: header
            required property string section

            width: listView.width
            height: 35
            color: "transparent"

            Rectangle {
                color: Colors.suface1
                implicitWidth: txtText.implicitWidth + AppLayouts.x_padding*2
                implicitHeight: txtText.implicitHeight + AppLayouts.m_padding*2
                anchors.centerIn: parent
                radius: width/2
                border.color: Colors.suface3
                Text {
                    id: txtText
                    anchors.centerIn: parent
                    text: header.section
                    color: "white"
                    font.bold: true
                    // padding: AppLayouts.x_padding

                }
            }
        }

        // -------------------------
        // Message delegate
        // -------------------------

         delegate: MessageBubble {
            id: messageDelegate
            required property var model
            required property int index
            readonly property int messageIndex: index

            width: ListView.view.width

            selected: body.isMessageSelected(index)
            selectionMode: body.selectionMode
            isSeen: true
             // A Loader creates this item before onLoaded assigns the
             // delegate's model object. Keep the initial binding
             // evaluation (and model resets) safe while it is null.
            msg: model ? model.body : ""
            date: model ? model.sentAt : ""
            isOutgoing: model ? model.isMine : false
         }


        verticalLayoutDirection: ListView.TopToBottom
        Component.onCompleted: {
            body.scrollToLatest()
        }
        onVisibleChanged: {
            if (visible)
                body.scrollToLatest()
        }
        onContentYChanged: {
            const distanceToTop = contentY - minimumContentY
            if (distanceToTop < loadMoreThreshold
                    && body.model
                    && !body.model.isLoading
                    && !paginationPending) {
                scrollToLatestTimer.stop()
                capturePaginationAnchor()
                paginationPending = true
                body.model.fetchNextMessage()
            }
        }

        Connections {
            target: body.model

            function onInitialMessagesLoaded() {
                listView.paginationPending = false
                listView.paginationAnchorIndex = -1
                body.scrollToLatest()
            }

            function onOlderMessagesLoaded(insertedCount) {
                Qt.callLater(() => {
                    listView.forceLayout()
                    if (insertedCount > 0
                            && listView.paginationAnchorIndex >= 0) {
                        const targetIndex = listView.paginationAnchorIndex
                                + insertedCount
                        listView.positionViewAtIndex(targetIndex,
                                                     ListView.Beginning)
                        listView.forceLayout()
                        const anchorItem = listView.itemAtIndex(targetIndex)
                        if (anchorItem) {
                            listView.contentY = listView.clampContentY(
                                        anchorItem.y
                                        - listView.paginationAnchorOffset)
                        }
                    }
                    listView.paginationAnchorIndex = -1
                    listView.paginationPending = false
                })
            }
        }

        // remove: Transition {
        //     // Conversation switches set isVisible=false before clearing the
        //     // model. Do not keep removed delegates alive for their exit
        //     // animation, or they can overlap a fast-loading conversation.
        //     enabled: body.isVisible
        //     ParallelAnimation {
        //         NumberAnimation {
        //             property: "opacity"
        //             from: 1
        //             to: 0
        //             duration: 180
        //         }
        //         NumberAnimation {
        //             property: "x"
        //             to: 100
        //             duration: 180
        //             easing.type: Easing.InCubic
        //         }
        //     }
        // }


        // =====================================================
        // ONE MouseArea controls selection for entire ListView
        // =====================================================

        MouseArea {
            id: selectionArea

            anchors.fill: parent

            acceptedButtons: Qt.LeftButton | Qt.RightButton
            hoverEnabled: true

            property var lastItem: null
            property var pressedItem: null
            property real pressY: 0
            property real lastMouseY: 0
            property int dragDirection: 0
            readonly property real verticalDragThreshold: 4
            readonly property real directionChangeThreshold: 2

            function itemUnderMouse(mouseX, mouseY) {

                // Convert viewport coordinate -> ListView content coordinate
                let p = selectionArea.mapToItem(
                    listView.contentItem,
                    mouseX,
                    mouseY
                )

                return listView.itemAt(p.x, p.y)
            }

            onClicked: (mouse) => {
                           if (mouse.button === Qt.LeftButton) {
                               return
                           }
                           let delegate = itemUnderMouse(mouse.x, mouse.y)
                           if (!delegate || !delegate.bubbleItem) {
                               return
                           }

                           var p = selectionArea.mapToItem(delegate.bubbleItem, mouse.x, mouse.y)
                           const selectOnly = !delegate.bubbleItem.contains(p)
                           const bodyPosition = selectionArea.mapToItem(
                               body, mouse.x, mouse.y)
                           body.contextMenuRequested(delegate.messageIndex,
                                                     selectOnly,
                                                     bodyPosition.x,
                                                     bodyPosition.y)

                       }
            onPressed: mouse => {
                           if (mouse.button === Qt.RightButton) {
                               return
                           }
                           if (body.contextMenuOpen) {
                               mouse.accepted = false
                               return
                           }

                           let delegate = itemUnderMouse(mouse.x, mouse.y)

                           pressedItem = delegate
                           pressY = mouse.y
                           lastMouseY = mouse.y
                           dragDirection = 0
                           lastItem = null
                           listView.dragSelecting = false

                           if (!delegate)
                           return

                           const startWasSelected = body.isMessageSelected(
                               delegate.messageIndex)

                           // The state at mouse press controls the complete gesture.
                           // Checked start   -> uncheck every crossed message.
                           // Unchecked start -> check every crossed message.
                           listView.dragSelectValue = !startWasSelected
                       }
            onPositionChanged: mouse => {
                                   if (body.contextMenuOpen) {
                                       mouse.accepted = false

                                       return
                                   }
                                   if (!pressedItem) {
                                       return
                                   }

                                   // Ignore clicks and horizontal movement. Selection starts
                                   // only after an upward or downward drag.
                                   if (!listView.dragSelecting) {
                                       if (Math.abs(mouse.y - pressY) < verticalDragThreshold)
                                       return

                                       listView.dragSelecting = true
                                       dragDirection = mouse.y > pressY ? 1 : -1
                                       body.selectionRequested(
                                           pressedItem.messageIndex,
                                           listView.dragSelectValue
                                           )
                                       lastItem = pressedItem
                                   }

                                   const movement = mouse.y - lastMouseY
                                   if (Math.abs(movement) >= directionChangeThreshold) {
                                       const newDirection = movement > 0 ? 1 : -1

                                       if (dragDirection !== 0 && newDirection !== dragDirection) {
                                           // Reversing the drag reverses its action. Update the
                                           // turning-point message immediately, then use the
                                           // new value for messages crossed on the way back.
                                           listView.dragSelectValue = !listView.dragSelectValue
                                           if (lastItem) {
                                               body.selectionRequested(
                                                   lastItem.messageIndex,
                                                   listView.dragSelectValue
                                                   )
                                           }
                                       }

                                       dragDirection = newDirection
                                       lastMouseY = mouse.y
                                   }

                                   let delegate = itemUnderMouse(mouse.x, mouse.y)
                                   if (!delegate)
                                   return

                                   // Don't repeatedly process same delegate
                                   if (delegate === lastItem)
                                   return

                                   body.selectionRequested(
                                       delegate.messageIndex,
                                       listView.dragSelectValue
                                       )

                                   lastItem = delegate
                               }
            onReleased: (mouse) => {
                // Once selection mode is active, clicking toggles any
                // message. Outside selection mode, a click does nothing.
                if (mouse.button == Qt.LeftButton
                        &&!listView.dragSelecting
                        && pressedItem
                        && body.selectionMode) {
                    body.selectionRequested(
                                pressedItem.messageIndex,
                                !body.isMessageSelected(pressedItem.messageIndex)
                                )
                }

                listView.dragSelecting = false
                pressedItem = null
                lastItem = null
                dragDirection = 0
            }
            onCanceled: {
                listView.dragSelecting = false
                pressedItem = null
                lastItem = null
                dragDirection = 0
            }
            // wheel scrolling
            onWheel: wheel => {
                         listView.contentY = listView.clampContentY(
                             listView.contentY - wheel.angleDelta.y
                             )
                         verticalScrollBar.active = true
                         scrollIndicatorTimer.restart()
                         wheel.accepted = true
                    }
        }
    }

    Timer {
        id: scrollToLatestTimer

        interval: 10
        repeat: false
        onTriggered: {
            if (!listView.visible || listView.count === 0) {
                stop()
                return
            }

            listView.forceLayout()
            listView.currentIndex = listView.count - 1
            listView.positionViewAtEnd()
        }
    }

}
