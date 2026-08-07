pragma ComponentBehavior: Bound
import QtQuick 2.15
import Qt5Compat.GraphicalEffects


Item {
    id: root
    width: 120
    height: 120

    property alias source: avatarImage.source
    property alias fillMode: avatarImage.fillMode

    property color borderColor: "black"
    property int borderWidth: 2


    Rectangle {
        id: mask
        anchors.fill: parent
        radius: width / 2
        visible: false
    }

    Image {
        id: avatarImage
        anchors.fill: parent
        fillMode: Image.PreserveAspectCrop
        layer.enabled: true
        layer.effect: OpacityMask {
            maskSource: mask
        }
    }
    Rectangle {
        anchors.fill: parent
        radius: width / 2
        color: "transparent"
        border.width: root.borderWidth
        border.color: root.borderColor
    }
}
