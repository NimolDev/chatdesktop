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

    property real previousBodyHeight: 0
    property real resizeContentY: 0
    property real stableContentY: 0
    property bool resizing: false
    property bool followLatest: false
    property bool initialLoadComplete: false
    property bool initialPositionReady: false

    function prepareInitialMessages() {
        initialReveal.stop()
        initialPositionTimer.stop()
        initialPositionReady = false
        listView.opacity = 0
        listView.animateAppends = false
    }


    function settleLatestPosition() {
        if (followLatest && !resizing && !appendScroll.running
                && !scrollToLatestTimer.running && !listView.paginationPending) {
            listView.forceLayout()
            listView.positionViewAtEnd()
        }
    }

    Component.onCompleted: previousBodyHeight = height
    onHeightChanged: {
        const previousHeight = previousBodyHeight
        previousBodyHeight = height
        if (previousHeight <= 0 || height <= 0) {
            return
        }

        if (!resizing) {
            resizeContentY = stableContentY
            resizing = true
            appendScroll.stop()
        }
        resizeContentY += previousHeight - height
        Qt.callLater(restoreResizePosition)
    }

    function restoreResizePosition() {
        listView.forceLayout()
        listView.contentY = listView.clampContentY(resizeContentY)
        stableContentY = listView.contentY
        resizing = false
    }

    function scrollToLatest(animated = true) {
        followLatest = true
        // The model can notify before ListView has processed the new rows.
        scrollToLatestTimer.animated = animated && listView.animateAppends
        scrollToLatestTimer.restart()
    }



    ListView {
        id: listView
        objectName: "messageList"

        property bool animateAppends: false
        anchors.fill: parent
        clip: true
        visible: true
        opacity: 0
        enabled: body.initialPositionReady

        property bool dragSelecting: false
        property bool dragSelectValue: true
        readonly property real minimumContentY: originY
        readonly property real maximumContentY: Math.max(
            minimumContentY,
            originY + contentHeight + listView.bottomMargin - height
        )
        readonly property real distanceFromBottom: Math.max(0, maximumContentY - contentY)

        readonly property bool isNearBottom: distanceFromBottom <= 10

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
        Behavior on anchors.topMargin {
            enabled: listView.animateAppends && !listView.paginationPending
                     && !body.resizing && body.model && !body.model.isLoading
            NumberAnimation { duration: 240; easing.type: Easing.OutCubic }
        }

        add: Transition {
            enabled: listView.animateAppends && !listView.paginationPending
                     && body.model && !body.model.isLoading
            ParallelAnimation {
                NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 180 }
                NumberAnimation {
                    property: "scale"
                    from: 0.97
                    to: 1
                    duration: 240
                    easing.type: Easing.OutCubic
                }
            }
        }

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

        // Layout changes (especially delayed removals) also move contentY.
        // Only an explicit scroll should fetch history and release bottom anchoring.
        function requestOlderMessages() {
            const distanceToTop = contentY - minimumContentY
            if (distanceToTop < loadMoreThreshold
                    && body.model
                    && !body.model.isLoading
                    && !paginationPending
                    && !body.resizing
                    && body.height === body.previousBodyHeight
                    && !appendScroll.running
                    && !scrollToLatestTimer.running) {
                scrollToLatestTimer.stop()
                body.followLatest = false
                capturePaginationAnchor()
                paginationPending = true
                body.model.fetchNextMessage()
            }
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
            transformOrigin: isOutgoing ? Item.Right : Item.Left

            width: ListView.view.width
            property real removalHeight: -1
            property real collapseProgress: 0
            height: removalHeight >= 0
                    ? removalHeight * (1 - collapseProgress) : implicitHeight

            viewmodel: model
            messagingViewModel: body.model
            selected: body.isMessageSelected(index)
            selectionMode: body.selectionMode

            ListView.onRemove: {
                if (body.isVisible && listView.animateAppends
                        && body.model && !body.model.isLoading) {
                    // Reserve this row until its pixels have disappeared.
                    ListView.delayRemove = true
                    removalHeight = height
                    removalAnimation.start()
                }
            }

            SequentialAnimation {
                id: removalAnimation
                NumberAnimation {
                    target: messageDelegate
                    property: "removalProgress"
                    from: 0
                    to: 1
                    duration: 400
                    easing.type: Easing.Linear
                }
                // Let ListView lay out the shrinking row on every frame. A
                // removeDisplaced y animation competes with bottom anchoring
                // and asynchronous media sizing, leaving neighboring rows overlapping.
                NumberAnimation {
                    target: messageDelegate
                    property: "collapseProgress"
                    from: 0
                    to: 1
                    duration: 220
                    easing.type: Easing.InOutCubic
                }
                onStopped: messageDelegate.ListView.delayRemove = false
            }

            Connections {
                target: listView
                function onAnimateAppendsChanged() {
                    if (!listView.animateAppends) {
                        removalAnimation.stop()
                    }
                }
            }
        }

        verticalLayoutDirection: ListView.TopToBottom
        onContentHeightChanged: Qt.callLater(body.settleLatestPosition)
        onMovementStarted: body.followLatest = false
        Component.onCompleted: {
            body.initialLoadComplete = body.model && !body.model.isLoading
            body.scrollToLatest(false)
        }
        onVisibleChanged: {
            if (visible)
                body.scrollToLatest(false)
        }
        onContentYChanged: {
            // Geometry changes can clamp contentY before onHeightChanged runs.
            if (!body.resizing && body.height === body.previousBodyHeight) {
                body.stableContentY = contentY
            }
            if (verticalScrollBar.pressed) {
                requestOlderMessages()
            }
            const distantFromBottom = (contentY - maximumContentY) <= -200
            fabContent.visible = distantFromBottom
        }

        Connections {
            target: body.model

            function onRowsInserted(parent, first, last) {
                if (!body.model.isLoading && !listView.paginationPending) {
                    body.scrollToLatest()
                }
            }

            function onModelAboutToBeReset() {
                body.initialLoadComplete = false
                body.prepareInitialMessages()
                body.followLatest = false
                appendScroll.stop()
                scrollToLatestTimer.stop()
                listView.animateAppends = false
            }

            function onInitialMessagesLoaded() {
                body.initialLoadComplete = true
                listView.paginationPending = false
                listView.paginationAnchorIndex = -1
                body.scrollToLatest(false)
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

        // =====================================================
        // ONE MouseArea controls selection for entire ListView
        // =====================================================

        MouseArea {
            id: selectionArea

            anchors.fill: parent

            acceptedButtons: Qt.LeftButton | Qt.RightButton
            // Observe movement only during selection presses so delegate hover
            // handlers can receive hover events and provide their own cursors.
            hoverEnabled: false
            // MouseArea sets an arrow cursor even when hover is disabled.
            // Unset it so controls underneath can supply their own cursor.
            cursorShape: undefined

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

                           // Non-text bubble controls handle their own pointer gestures.
                           // Text messages and selection mode keep row selection behavior.
                           if (!body.selectionMode && delegate
                                   && delegate.msgType !== 0 && delegate.bubbleItem) {
                               const bubblePosition = selectionArea.mapToItem(
                                   delegate.bubbleItem, mouse.x, mouse.y)
                               if (delegate.bubbleItem.contains(bubblePosition)) {
                                   mouse.accepted = false
                                   return
                               }
                           }

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
                         body.followLatest = false
                         appendScroll.stop()
                         scrollToLatestTimer.stop()
                         listView.contentY = listView.clampContentY(
                             listView.contentY - wheel.angleDelta.y
                             )
                         listView.requestOlderMessages()
                         verticalScrollBar.active = true
                         scrollIndicatorTimer.restart()
                         wheel.accepted = true
                    }
        }
    }
    Rectangle {
        id: fabContent
        visible: false
        implicitWidth: 50
        implicitHeight: 50

        x: parent.width - width - AppLayouts.x_padding
        y: visible ? parent.height - (30 + implicitHeight) : parent.height
        z: 1
        color: "transparent"


        Rectangle {
            id: notSeenContent
            property int count: 0

            visible: count > 0
            height: 20
            width: Math.max(height, txtNotSeenCount.implicitWidth + 8)
            z:1
            anchors {
                top: parent.top
                horizontalCenter: parent.horizontalCenter
            }
            color: Colors.primary
            radius: height/2
            clip: true

            Text {
                id: txtNotSeenCount
                text: notSeenContent.count > 1000 ? "999+" : notSeenContent.count
                color: Colors.black500
                font.family: Typography.family
                font.pixelSize: Typography.body
                font.weight: Typography.semiBold
                horizontalAlignment: Text.AlignHCenter

                anchors.fill: parent
                anchors.leftMargin: 4
                anchors.rightMargin: 4
            }
        }

        Button {
            id: btnScrollToLast
            width: 45
            height: 45
            padding: AppLayouts.x_padding
            anchors {
                top: notSeenContent.bottom
                topMargin: -8
                horizontalCenter: parent.horizontalCenter
            }

            background: Rectangle {
                color: Colors.suface1
                radius: width/2
                clip: true
                border.width: 1
                border.color: Colors.suface2
            }
            contentItem: Image {
                source: AppAssets.icChevronUp
                fillMode: Image.PreserveAspectFit
                rotation: 180
            }
            hoverEnabled: true

            onClicked: {
                body.scrollToLatest(true)

            }
            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: parent.clicked()
            }
        }

        Behavior on y {
            NumberAnimation {
                duration: 400
                easing.type: Easing.OutCubic
            }
        }
    }
    NumberAnimation {
        id: appendScroll
        target: listView
        property: "contentY"
        duration: 240
        easing.type: Easing.OutCubic
        onFinished: Qt.callLater(body.settleLatestPosition)
    }

    Timer {
        id: scrollToLatestTimer
        property bool animated: false

        interval: 10
        repeat: false
        onTriggered: {
            listView.forceLayout()
            if (!listView.visible || !body.initialLoadComplete) {
                stop()
                return
            }

            appendScroll.stop()
            if (animated) {
                // Instantiate the last rows before choosing the target. The
                // estimated contentHeight can otherwise leave a gap or require
                // a visible correction after the animation.
                const startY = listView.contentY
                listView.positionViewAtEnd()
                listView.forceLayout()
                const endY = listView.contentY
                // Limit distant jumps to one viewport so the final rows stay
                // instantiated and their geometry remains stable while scrolling.
                const animationStartY = Math.max(startY, endY - listView.height)
                listView.contentY = animationStartY
                appendScroll.from = animationStartY
                appendScroll.to = endY
                appendScroll.start()
            } else {
                listView.positionViewAtEnd()
            }
            if (!body.initialPositionReady) {
                initialPositionTimer.restart()
            } else {
                listView.animateAppends = true
            }
        }
    }

    // Allow the final delegates and section headers a layout pass before reveal.
    Timer {
        id: initialPositionTimer
        interval: 16
        onTriggered: {
            listView.forceLayout()
            listView.positionViewAtEnd()
            body.stableContentY = listView.contentY
            body.initialPositionReady = true
            listView.animateAppends = true
            initialReveal.start()
        }
    }

    NumberAnimation {
        id: initialReveal
        target: listView
        property: "opacity"
        from: 0
        to: 1
        duration: 180
        easing.type: Easing.OutCubic
    }


}
