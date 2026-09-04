import QtQuick

import QtQuick.Window

Window {
    id: petWindow

    // Keep the pet independent from the main window. In particular, hiding the
    // chat window must not hide its desktop companion as a transient child.
    transientParent: null
    objectName: "desktopPetWindow"

    width: 140
    height: 140
    visible: true

    color: "transparent"

    flags: Qt.FramelessWindowHint
         | Qt.WindowStaysOnTopHint
         | Qt.Tool

    // Image {
    //     id: pet

    //     anchors.centerIn: parent
    //     width: 120
    //     height: 120

    //     source: "qrc:/images/vp_start_blink.gif"
    //     fillMode: Image.PreserveAspectFit
    // }
    AnimatedImage {
        id: pet
        source: "qrc:/images/vp_start_blink.gif"

        width: 120
        height: 120
        fillMode: Image.PreserveAspectFit

        playing: petWindow.visible
        speed: 2
    }

    MouseArea {
        anchors.fill: parent

        property point pressPosition

        onPressed: mouse => {
                       pressPosition = Qt.point(mouse.x, mouse.y)
                       console.log("Presessed")
                   }
        onHoveredChanged: {
            console.log("On hover")
        }

        onPositionChanged: mouse => {
                               if (!pressed) {
                                   return
                               }
                               petWindow.x += mouse.x - pressPosition.x
                               petWindow.y += mouse.y - pressPosition.y
                           }
    }
}
