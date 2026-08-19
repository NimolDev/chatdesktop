import QtQuick 2.15
import QtQuick.Controls

import Theme

Popup {
    id: emojiPopup
    readonly property alias hovered: popupHover.hovered

    padding: 8

    width: 400
    height: 400


    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    background: Rectangle {
        color: Colors.suface1
        radius: 8
        border.color: Colors.suface3
    }
    HoverHandler {
        id: popupHover
        cursorShape: Qt.PointingHandCursor
    }
    transformOrigin: Item.BottomRight
    enter: Transition {
       ParallelAnimation {
           NumberAnimation {
               property: "opacity"
               from: 0
               to: 1
               duration: 150
               easing.type: Easing.OutCubic
           }
           NumberAnimation {
               property: "scale"
               from: 0.5
               to: 1
               duration: 100
           }
       }
    }
    exit: Transition {
        ParallelAnimation {
            NumberAnimation {
                property: "opacity"
                from: 1
                to: 0
                duration: 150
            }
            NumberAnimation {
                property: "scale"
                from: 1
                to: 0.5
                duration: 100
            }
        }
    }

    contentItem: Rectangle {
        color: "transparent"
        Text {
            text: "hello"
        }
    }


}
