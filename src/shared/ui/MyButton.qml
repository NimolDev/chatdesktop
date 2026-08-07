import QtQuick 2.15
import QtQuick.Controls.Basic
import Theme

Button {
    id: control
    implicitHeight: 50
    implicitWidth: 200
    font.family: Typography.family
    font.pixelSize: Typography.title4
    font.weight: Typography.medium
    text: qsTr("")

    background: Rectangle {
        radius: 12
        color: control.down ? Colors.primary600 : Colors.primary


    }
    onClicked: {
        background: Colors.primary900Changed
    }
}