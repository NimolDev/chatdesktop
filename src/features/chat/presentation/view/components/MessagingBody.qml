pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Controls

Item {
    id: body

    property var model: null
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
        if (listView.count > 0)
            scrollToLatestTimer.restart()
    }

    Image {
        anchors.fill: parent
        source: "qrc:/background/background.png"
        fillMode: Image.Tile
        sourceSize: Qt.size(512, 512)
        smooth: false
        mipmap: true
        asynchronous: false
    }

    ListView {
        id: listView
        anchors.fill: parent
        clip: true

        anchors.topMargin: 8
        bottomMargin: 20

        spacing: 2
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

        property bool dragSelecting: false
        property bool dragSelectValue: true
        readonly property real minimumContentY: originY
        readonly property real maximumContentY: Math.max(
            minimumContentY,
            originY + contentHeight + listView.bottomMargin - height
        )

        function clampContentY(value) {
            return Math.max(
                minimumContentY,
                Math.min(value, maximumContentY)
            )
        }

        // -------------------------
        // Sections
        // -------------------------

        section.property: "section"
        section.criteria: ViewSection.FullString

        section.delegate: Rectangle {
            required property string section

            width: listView.width
            height: 35
            color: "transparent"

            Text {
                anchors.centerIn: parent
                text: parent.section
                color: "white"
                font.bold: true
            }
        }

        // -------------------------
        // Message delegate
        // -------------------------

        delegate: Loader {
            id: messageDelegate

            required property var model
            required property int index

            width: ListView.view.width

            property bool selected: body.isMessageSelected(index)
            property int messageIndex: index

            sourceComponent: textMessaging
            property Item bubbleItem: item ? item.bubbleItem : null

            onLoaded: {
                item.messageData = model
                item.selected = Qt.binding(
                    () => messageDelegate.selected
                )
                item.selectionMode = Qt.binding(
                    () => body.selectionMode
                )
            }
        }

        Component {
            id: textMessaging

            MessageBubble {
                property var messageData
                width: listView.width
                isSeen: true
                msg: messageData.msg
                date: messageData.date
                isOutgoing: messageData.sender_id === "1"
            }
        }

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
                           body.contextMenuRequested(delegate.messageIndex,
                                                     selectOnly,
                                                     mouse.x, mouse.y)

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

        interval: 0
        onTriggered: {
            listView.forceLayout()
            listView.positionViewAtEnd()

            // positionViewAtEnd() runs before every pending delegate and
            // composer resize is polished. Apply the final bound once
            // more after that layout pass.
            Qt.callLater(() => {
                listView.forceLayout()
                listView.contentY = listView.maximumContentY
            })
        }
    }

}
