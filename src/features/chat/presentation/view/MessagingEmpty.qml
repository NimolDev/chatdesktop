import QtQuick
import QtQuick.Controls

import Theme

Item {

    implicitWidth: AppLayouts.minWidth
    implicitHeight: AppLayouts.minHeight

    Rectangle {
        anchors.fill: parent
        color: Colors.background
    }

    Label {
        anchors.centerIn: parent
        height: contentHeight + 16 // padding top: 12, bottom: 12
        width: contentWidth + 16 // padding left: 12, right: 12
        text: "Select a chat to start messaging."
        font.family: Typography.family
        font.pixelSize: Typography.title4
        font.weight: Typography.bold
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter

        background: Rectangle {
           radius: AppLayouts.x_radius
            color: Colors.suface1
            border.color: Colors.suface3
        }
    }

}
