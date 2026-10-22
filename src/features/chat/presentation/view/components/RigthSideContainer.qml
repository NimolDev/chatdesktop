pragma ComponentBehavior: Bound
import QtQuick 2.15

import Theme


Item {
    id: root
    property bool isShow: false
    property bool isEmoji: true

    signal stickerClicked(string url)
    // signal

    visible: isShow
    implicitWidth: 300

    Rectangle {
        id: lineBreak
        height: parent.height
        width: 1
        color: Colors.suface1
        anchors {
            top: parent.top
            bottom: parent.bottom
            left: parent.left
        }
    }

    Rectangle {
        id: container
        anchors {
            top: parent.top
            bottom: parent.bottom
            left: lineBreak.right
            right: parent.right
        }

        color: "transparent"
        clip: true
        Loader {
            anchors.fill: parent
            anchors.topMargin: AppLayouts.x_padding
            anchors.bottomMargin: AppLayouts.x_padding
            sourceComponent: root.isEmoji ? stickerComponent : useProfileComponent
        }
    }
    Component {
        id: stickerComponent
        StickerListView {

            onClicked: url => {
                root.stickerClicked(url)
            }
        }
    }
    Component {
        id: useProfileComponent
        UserProfileView {

        }
    }

}
