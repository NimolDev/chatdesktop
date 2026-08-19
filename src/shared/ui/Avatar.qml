import QtQuick
import QtQuick.Effects

Item {
    id: root
    width: 120
    height: 120

    property alias source: avatarImage.source
    property alias fillMode: avatarImage.fillMode
    property color borderColor: "black"
    property int borderWidth: 2

    // Image {
    //     id: avatarImage
    //     anchors.fill: parent
    //     source: "qrc:/images/profile.jpeg"
    //     fillMode: Image.PreserveAspectCrop
    //     opacity: 0          // was visible: false — this hid it from the scene graph entirely
    // }

    // Rectangle {
    //     id: mask
    //     anchors.fill: parent
    //     radius: width / 2
    //     opacity: 0           // same fix here
    // }

    // MultiEffect {
    //     anchors.fill: parent
    //     source: avatarImage
    //     maskEnabled: true
    //     maskSource: mask
    // }

    // Rectangle {
    //     anchors.fill: parent
    //     radius: width / 2
    //     color: "transparent"
    //     border.color: root.borderColor
    //     border.width: root.borderWidth
    // }
    // Avatar.qml

    Image {
        id: avatarImage
        anchors.fill: parent

        source: "qrc:/images/profile.jpeg"
        fillMode: Image.PreserveAspectCrop

        smooth: true
        mipmap: true

        layer.enabled: true
        layer.smooth: true

        visible: false
    }

    Rectangle {
        id: mask

        anchors.fill: parent
        radius: width / 2

        antialiasing: true

        layer.enabled: true
        layer.smooth: true

        visible: false
    }

    MultiEffect {
        anchors.fill: parent

        source: avatarImage

        maskEnabled: true
        maskSource: mask

        maskThresholdMin: 0.0
        maskSpreadAtMin: 0.05

        antialiasing: true
    }
}