import QtQuick 2.15
import QtQuick.Controls

Button {

    id: root
    property color borderColor: "gray"
    property color color: "transparent"
    property int borderWidth: 2
    property url iconSource: ""
    property color tintColor: "white"
    property var hoverColor: null

    implicitWidth: 50
    implicitHeight: 50

    background: Rectangle {
       anchors.fill: parent
       radius: parent.width/2
       color: root.color
       border.color: root.borderColor
       border.width: root.borderWidth
    }

    padding: 8
    icon.source: root.iconSource
    icon.width: parent.width
    icon.height: parent.width
    icon.color: root.tintColor



    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
        onHoveredChanged: {
            root.icon.color = root.hoverColor
                    ? (mouse.containsMouse ? root.hoverColor : root.tintColor)
                    : (mouse.containsMouse ? Qt.lighter(root.tintColor, 1.2) : root.tintColor)
        }
    }
}
