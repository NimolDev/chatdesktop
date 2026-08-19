import QtQuick
import QtQuick.Controls.Basic

import Theme
import Shared.UI

Menu {
    id: root

    property bool selectOnly: false
    property int messageIndex: -1

    signal replyRequested(int messageIndex)
    signal copyRequested(int messageIndex)
    signal forwardRequested(int messageIndex)
    signal deleteRequested(int messageIndex)
    signal selectRequested(int messageIndex)

    function showForMessage(index, onlySelect, position) {
        messageIndex = index
        selectOnly = onlySelect
        x = position.x + 4
        y = position.y + 4
        open()
    }

    padding: 6
    implicitWidth: 150
    transformOrigin: Item.TopLeft

    background: Rectangle {
        radius: AppLayouts.x_radius
        color: Colors.suface1
        border.color: Colors.suface3
    }

    enter: Transition {
        ParallelAnimation {
            NumberAnimation {
                property: "opacity"
                from: 0
                to: 1
                duration: 150
                easing.type: Easing.OutCubic
            }
            NumberAnimation {
                property: "scale"
                from: 0.5
                to: 1
                duration: 150
                easing.type: Easing.OutQuad
            }
        }
    }

    exit: Transition {
        ParallelAnimation {
            NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 150 }
            NumberAnimation { property: "scale"; from: 1; to: 0.5; duration: 150 }
        }
    }

    ContextMenuItem {
        visible: !root.selectOnly
        height: visible ? implicitHeight : 0
        text: qsTr("Reply")
        icon.name: "arrowshape.turn.up.left"
        onTriggered: root.replyRequested(root.messageIndex)
    }
    ContextMenuItem {
        visible: !root.selectOnly
        height: visible ? implicitHeight : 0
        text: qsTr("Copy Text")
        icon.name: "document.on.document"
        onTriggered: root.copyRequested(root.messageIndex)
    }
    ContextMenuItem {
        visible: !root.selectOnly
        height: visible ? implicitHeight : 0
        text: qsTr("Forward")
        icon.name: "arrowshape.turn.up.forward"
        onTriggered: root.forwardRequested(root.messageIndex)
    }
    ContextMenuItem {
        visible: !root.selectOnly
        height: visible ? implicitHeight : 0
        text: qsTr("Delete")
        icon.name: "trash"
        onTriggered: root.deleteRequested(root.messageIndex)
    }
    ContextMenuItem {
        visible: true
        height: implicitHeight
        text: qsTr("Select")
        icon.name: "checkmark.circle"
        onTriggered: root.selectRequested(root.messageIndex)
    }

}
