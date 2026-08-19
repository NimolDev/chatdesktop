import QtQuick
import QtQuick.Effects

Item {
    id: root
    width: 400
    height: 100

    // Content behind the glass
    Image {
        id: background
        anchors.fill: parent
        source: "background.jpg"
        fillMode: Image.PreserveAspectCrop
    }

    Item {
        id: glass
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
            margins: 16
        }

        height: 64

        // Capture what is behind the glass
        ShaderEffectSource {
            id: backdrop
            anchors.fill: parent

            sourceItem: background
            sourceRect: Qt.rect(
                glass.x,
                glass.y,
                glass.width,
                glass.height
            )

            hideSource: false
        }

        MultiEffect {
            anchors.fill: parent
            source: backdrop

            blurEnabled: true
            blur: 0.7
            blurMax: 64
        }

        Rectangle {
            anchors.fill: parent
            radius: 22

            // translucent glass tint
            color: "#30FFFFFF"

            border.width: 1
            border.color: "#40FFFFFF"
        }

        Text {
            anchors.centerIn: parent
            text: "Liquid Glass"
            color: "white"
            font.pixelSize: 17
            font.weight: Font.DemiBold
        }
    }
}