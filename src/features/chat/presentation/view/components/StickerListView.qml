pragma ComponentBehavior: Bound
import QtQuick 2.15
import QtQuick.Controls

import Shared.assets
import Theme

GridView {
    id: stickerGrid

    signal clicked(string url)
    anchors.fill: parent
    clip: true

    cellWidth: stickerGrid.width/4
    cellHeight: stickerGrid.width/4


    ScrollBar.vertical: ScrollBar {
        policy: ScrollBar.AlwaysOn
        width: 6
        background: Rectangle {
            color: Colors.suface1

        }
        padding: 0
        contentItem: Rectangle {
            radius: 4
            color: Colors.suface3
            opacity: 0.7
        }
    }

    model: StickerMananger.getStickers()

    delegate: Item {
        id: delegate
        required property string modelData

        width:  stickerGrid.cellWidth
        height:  stickerGrid.cellHeight

        Image {
            anchors.centerIn: parent

            width: parent.height - 4
            height: parent.height - 4

            source: delegate.modelData
            asynchronous: true
            fillMode: Image.PreserveAspectFit
            smooth: true
        }

        TapHandler {
            onTapped: {
                console.log("Selected sticker:", delegate.modelData)
                stickerGrid.clicked(delegate.modelData)
            }
        }
        HoverHandler {
            cursorShape: Qt.PointingHandCursor
        }
    }
}