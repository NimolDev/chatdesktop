import QtQuick 2.15
import QtQuick.Controls

import Theme

Slider {
    id: root
    property bool showHandler: true


    property int progressHeight: 8
    property real handleSize: pressed ? progressHeight * 1.8 : progressHeight * 1.5
    property color backgroundColor: Colors.suface1
    property color progressColor: Colors.primary
    // property int height: 8

    background: Rectangle {
        id: track

        x: root.leftPadding
        y: root.topPadding
           + (root.availableHeight - height) / 2

        width: root.availableWidth
        height: root.progressHeight
        radius: height / 2

        color: root.backgroundColor

        Rectangle {
            width: track.width * root.visualPosition
            height: track.height

            radius: track.radius
            color: root.progressColor
        }
    }

    handle: Rectangle {
        visible: root.showHandler
        width: root.handleSize
        height: width
        radius: width / 2

        // Center handle exactly at progress endpoint
        x: root.leftPadding
           + root.visualPosition * root.availableWidth
           - width / 2

        y: root.topPadding
           + (root.availableHeight - height) / 2

        color: root.progressColor

        Behavior on width {
            NumberAnimation {
                duration: 100
                easing.type: Easing.OutCubic
            }
        }
    }
}
