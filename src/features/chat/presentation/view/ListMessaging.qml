import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Theme
import Shared.UI

Item {
    implicitWidth: AppLayouts.minWidth
    implicitHeight: AppLayouts.minHeight
    anchors.fill: parent
    Rectangle {
        anchors.fill: parent
        color: Colors.background
    }
    ToolBar {
        id: toolBar

        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
        }
        height: 50

        background: Rectangle {
            color: "red"
        }
        padding: 17
        contentItem: ColumnLayout {
            anchors.fill: parent
            Avatar {
                Layout.fillHeight: true
                Layout.preferredWidth: height
                source: AppAssets.profile
                borderWidth: 0
            }
        }
    }

    ListView {
        anchors {
            top: toolBar.bottom
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }

        Image {
            anchors.fill: parent
            source: "qrc:/background/background.png"
            fillMode: Image.Tile
            sourceSize: Qt.size(512, 512)
            smooth: false
            mipmap: true
            asynchronous: false
        }

        model: 100
        delegate: Rectangle {
        }



    }


}
