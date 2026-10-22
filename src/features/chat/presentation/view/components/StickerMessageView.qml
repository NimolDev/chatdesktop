import QtQuick 2.15

import Theme

Item {
    id: root
    implicitWidth: 600
    implicitHeight: container.height + padding

    property var viewmodel

    readonly property Item bubbleItem: container
    readonly property var isOutgoing: viewmodel ? (viewmodel.isMine ? true : false )
                                                : true
    readonly property var imageSource: viewmodel ? viewmodel.filePath : "qrc:/stickers/g2/1f603.png"
    readonly property var date: viewmodel ? viewmodel.sentAt : "None"

    readonly property real padding: AppLayouts.x_padding
    readonly property bool isCollapsedMessage: width > AppLayouts.width


    // Rectangle {
    //     anchors.fill: parent
    //     color: "black"
    // }

    Rectangle {
        id: container
        width: 170
        height: 170
        opacity: imgSticker.status == Image.Ready || Image.Error

        color: "transparent"

       states: [
            State {
                name: "outgoing"
                when: root.isOutgoing && !root.isCollapsedMessage
                AnchorChanges {
                    target: container
                    anchors.left: undefined
                    anchors.right: container.parent.right
                }

            },
             State {
                name: "incoming"
                 when: !root.isOutgoing && root.isCollapsedMessage
                 AnchorChanges {
                     target:container
                     anchors.left: container.parent.left
                     anchors.right: undefined

                 }
            }
        ]
        Image {
            id: imgSticker
            anchors.fill: parent
            source: root.imageSource
            fillMode: Image.PreserveAspectFit
            smooth: true
            asynchronous: true
            cache: true
            mirror: false
        }

        Rectangle {
            color: Qt.alpha(Colors.background, 0.9)

            anchors {
                bottom: parent.bottom
                right: parent.right
            }
            width: Math.min(parent.width, txtDate.implicitWidth + root.padding)
            height: txtDate.implicitHeight + root.padding
            radius: height/2
            clip: true

            Text {
                id: txtDate
                anchors.centerIn: parent
                text: root.date
                color: Colors.textPrimary
                font.family: Typography.family
                font.weight: Typography.caption
                font.pixelSize: Typography.caption
            }
        }
    }


}
