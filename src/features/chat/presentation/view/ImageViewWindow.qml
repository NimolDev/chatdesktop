import QtQuick
import QtQuick.Controls

import Features.Chat
import Theme

Window {
    id: root

    property url imageSource
    property real zoom: 1
    property real panX: 0
    property real panY: 0

    readonly property real maximumPanX: Math.max(0, (image.paintedWidth * zoom - width) / 2)
    readonly property real maximumPanY: Math.max(0, (image.paintedHeight * zoom - height) / 2)
    readonly property bool canPan: maximumPanX > 0 || maximumPanY > 0
    readonly property real minimumZoom: 0.25
    readonly property real maximumZoom: 8

    function setZoom(value, anchorX = width / 2, anchorY = height / 2) {
        const nextZoom = Math.max(minimumZoom, Math.min(maximumZoom, value))
        const ratio = nextZoom / zoom
        const nextX = anchorX - width / 2 - (anchorX - width / 2 - panX) * ratio
        const nextY = anchorY - height / 2 - (anchorY - height / 2 - panY) * ratio
        zoom = nextZoom
        panTo(nextX, nextY)
    }

    function resetZoom() {
        zoom = 1
        panTo(0, 0)
    }

    function panTo(x, y) {
        panX = Math.max(-maximumPanX, Math.min(maximumPanX, x))
        panY = Math.max(-maximumPanY, Math.min(maximumPanY, y))
    }

    onMaximumPanXChanged: panTo(panX, panY)
    onMaximumPanYChanged: panTo(panX, panY)
    onImageSourceChanged: {
        updateIndex()
        zoom = 1
        panX = 0
        panY = 0
    }

    width: AppLayouts.width
    height: AppLayouts.height
    minimumWidth: 320
    minimumHeight: 240
    visible: false
    color: Colors.background

    property int index: -1
    readonly property int imageCount: imageList.length
    property var imageList: []

    function updateIndex() {
        root.index = root.imageList.indexOf(root.imageSource.toString())
    }

    Connections {
        target: MessagingViewModel
        function onImageUrlsChanged() {
            root.imageList = MessagingViewModel.imageUrls
            root.updateIndex()
        }
    }

    function getImage() {
        MessagingViewModel.getImageList()
    }

    function reset() {
        root.index = -1
        root.imageList = []
    }

    Item {
        anchors.fill: parent
        clip: true

        Image {
            id: image
            objectName: "viewerImage"
            width: Math.max(0, parent.width - 48)
            height: Math.max(0, parent.height - 48)
            x: (parent.width - width) / 2 + root.panX
            y: (parent.height - height) / 2 + root.panY
            source: root.imageSource
            asynchronous: true
            autoTransform: true
            fillMode: Image.PreserveAspectFit
            scale: root.zoom
        }

        MouseArea {
            objectName: "imageViewerDragArea"
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton | Qt.MiddleButton
            cursorShape: pressed ? Qt.SizeAllCursor : Qt.ArrowCursor
            property real wheelRemainder: 0
            property bool dragging: false
            property bool panning: false
            property real previousX: 0
            property real previousY: 0
            onPressed: mouse => {
                if (mouse.button === Qt.MiddleButton) {
                    root.resetZoom()
                    return
                }
                dragging = false
                panning = root.canPan
                previousX = mouse.x
                previousY = mouse.y
            }
            onPositionChanged: mouse => {
                                   if (mouse.y < 20) {
                                       root.startSystemMove()
                                       return;
                                   }

                                   if (!(pressedButtons & Qt.LeftButton)) {
                                       return
                                   }
                                   if (!dragging && Math.hypot(mouse.x - previousX, mouse.y - previousY) < 4) {
                                       return
                                   }
                                   dragging = true
                                   if (panning) {
                                       root.panTo(root.panX + mouse.x - previousX,
                                                  root.panY + mouse.y - previousY)
                                       previousX = mouse.x
                                       previousY = mouse.y
                                   } else {
                                       root.startSystemMove()
                                   }
                               }
            onReleased: { panning = false; dragging = false }
            onCanceled: { panning = false; dragging = false }
            onWheel: wheel => {
                         // Qt maps the macOS Command key to ControlModifier.
                         if (wheel.modifiers & Qt.ControlModifier) {
                             if (Math.abs(wheel.angleDelta.x) > Math.abs(wheel.angleDelta.y)) {
                                 wheelRemainder = 0
                             } else {
                                 wheelRemainder += wheel.angleDelta.y
                                 const steps = Math.trunc(wheelRemainder / 120)
                                 wheelRemainder -= steps * 120
                                 if (steps !== 0) {
                                     root.setZoom(root.zoom * Math.pow(1.2, steps), wheel.x, wheel.y)
                                 }
                             }
                         } else {
                             wheelRemainder = 0
                         }
                         wheel.accepted = true
                     }
        }
    }

    BusyIndicator {
        anchors.centerIn: parent
        running: image.status === Image.Loading
        visible: running
    }

    Label {
        anchors.centerIn: parent
        visible: image.status === Image.Error
        text: qsTr("Image unavailable")
        color: Colors.textPrimary
    }
    Label {
        anchors {
            horizontalCenter: parent.horizontalCenter
            bottom: parent.bottom
        }
        text: root.index
    }

    Button {
        id: btnBackward
        width: 40
        height: 40
        visible: root.index > 0
        anchors {
            verticalCenter: parent.verticalCenter
            left: parent.left
            leftMargin: AppLayouts.x_padding
        }
        padding: 10
        hoverEnabled: true
        background: Rectangle {
            color: btnBackward.hovered ? Colors.suface1 : "transparent"
            radius: btnBackward.width / 2
        }
        contentItem: Image {
            source: AppAssets.icChevronUp
            fillMode: Image.PreserveAspectFit
            rotation: -90
        }
        onClicked: {
            if (root.index > 0) {
                root.imageSource = root.imageList[root.index - 1]
            }
        }
    }
    Button {
        id: btnForward
        width: 40
        height: 40
        visible: root.index >= 0 && root.index < root.imageCount - 1
        anchors {
            verticalCenter: parent.verticalCenter
            right: parent.right
            rightMargin: AppLayouts.x_padding
        }
        padding: 10
        hoverEnabled: true
        background: Rectangle {
            color: btnForward.hovered ? Colors.suface1 : "transparent"
            radius: btnForward.width / 2
        }

        contentItem: Image {
            source: AppAssets.icChevronUp
            fillMode: Image.PreserveAspectFit
            rotation: 90
        }
        onClicked: {
            if (root.index >= 0 && root.index < root.imageCount - 1) {
                root.imageSource = root.imageList[root.index + 1]
            }
        }

    }


    Shortcut {
        sequences: ["Ctrl++", "Ctrl+="]
        onActivated: root.setZoom(root.zoom * 1.2)
    }
    Shortcut {
        sequence: "Ctrl+-"
        onActivated: root.setZoom(root.zoom / 1.2)
    }
    Shortcut {
        sequence: "Ctrl+0"
        onActivated: root.resetZoom()
    }
    Shortcut {
        sequence: "Escape"
        onActivated: root.close()
    }
    onClosing: imageSource = ""
}
