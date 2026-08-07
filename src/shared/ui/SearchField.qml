import QtQuick 2.15
import QtQuick.Controls.Basic

import Theme


TextField {
    id: searchField
    implicitHeight: 40
    implicitWidth: 240
    placeholderText: qsTr("Search")
    selectByMouse: true
    leftPadding: 36
    rightPadding: text.length > 0 ? 36 : 12
    // font.pixelSize: 14
    color: Colors.textPrimary
    placeholderTextColor: Colors.white400
    font.family: Typography.family
    font.pixelSize: Typography.body
    font.weight: Typography.medium


    background: Rectangle {
        radius: 8
        color: Colors.background
        border.color: searchField.activeFocus ? Colors.primary500 : Colors.suface3
        border.width: 1

        Behavior on border.color { ColorAnimation { duration: 120 } }
    }

    // Search icon, left-aligned
    Image {
        id: searchIcon
        source: AppAssets.icSearch // swap for your icon path
        width: 22
        height: 22
        anchors.left: parent.left
        anchors.leftMargin: 12
        anchors.verticalCenter: parent.verticalCenter
        opacity: 0.6

    }

    // Clear ("x") button, only visible when there's text
    ToolButton {
        visible: searchField.activeFocus
        anchors.right: parent.right
        anchors.rightMargin: 4
        anchors.verticalCenter: parent.verticalCenter
        implicitWidth: 28
        implicitHeight: 28
        icon.source: AppAssets.icClear // swap for your icon path
        icon.width: 22
        icon.height: 22
        icon.color: Colors.primary
        flat: true
        background: Rectangle {
            color: "transparent"
        }

        onClicked: {
            searchField.focus = false
            searchField.clear()
        }
    }

}

