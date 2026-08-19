import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts

import Theme
import Localization
import Shared.UI

Item {

    id: root
    implicitWidth:  300
    implicitHeight: 500

    signal closeClicked()


    Rectangle {
        anchors.fill: parent
        color: Colors.suface1
    }
    RowLayout {
        id: header
        anchors {
            top: root.top
            left: root.left
            right: root.right
        }

        Label {
            text: AppStrings.profile
            font.family: Typography.family
            font.pixelSize: Typography.title4
            font.weight: Typography.medium
            Layout.alignment: Qt.AlignVCenter
        }
        Item {
            Layout.fillWidth: true
        }

        Button {
            id: btnClose
            Layout.alignment: Qt.AlignVCenter
            Layout.preferredHeight: 30
            Layout.preferredWidth: 30
            icon.source: AppAssets.icClose
            background: Rectangle {
                color: btnClose.hovered ? Colors.suface2 : "transparent"
                radius: height/2
            }
            HoverHandler {
                cursorShape: Qt.PointingHandCursor
            }
            onClicked: root.closeClicked()
        }
    }


    ColumnLayout {
        anchors {
            top: header.bottom
            left: root.left
            right: root.right
            bottom: root.bottom
        }

        CircularImage {
            id: imgProfile
            Layout.preferredHeight: 70
            Layout.preferredWidth: height
            Layout.alignment: Qt.AlignHCenter
            source: AppAssets.profile

        }
    }


}
