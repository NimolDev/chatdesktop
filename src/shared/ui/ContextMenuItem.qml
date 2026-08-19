import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import Theme

MenuItem {
    id: root

    implicitWidth: contentRow.implicitWidth + leftPadding + rightPadding
    implicitHeight: 40
    hoverEnabled: true

    property bool isShowIcon: true
    property color tintColor: "white"
    HoverHandler {
        cursorShape: Qt.PointingHandCursor
    }

    contentItem: Row {
        id: contentRow
        spacing: root.isShowIcon ? 10 : 0

        IconImage {
            visible: root.isShowIcon
            width: root.isShowIcon ? 18 : 0
            height: 18
            anchors.verticalCenter: parent.verticalCenter

            source: root.icon.source
            name: root.icon.name
            fillMode: Image.PreserveAspectFit
            color: root.tintColor

        }

        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: root.text
            color: root.tintColor
            font.pixelSize: 14

        }
    }

    background: Rectangle {
        radius: AppLayouts.s_radius

        color: root.hovered
            ? Colors.suface2
            : "transparent"
    }


}