import QtQuick 2.15
import QtQuick.Layouts
import QtQuick.Controls.Basic

import Theme
import Shared.UI
// import App.Components

Item {
    id: root
    implicitWidth: 300
    implicitHeight: 50
    // anchors.fill: parent
    property bool isActive: true
    property bool isCompactMode: false
    property bool isSeen: true
    property int padding: 0
    property string username: "Username"
    property string message: "Message"


    property bool isSelected: false
    signal itemClicked()

    Pane {
        anchors.fill: parent
        padding: root.padding
        background: Rectangle {
            color: root.isSelected ? Colors.primary900 : Colors.background
        }

        RowLayout {
            anchors.fill: parent

            // --- Profile ----
            Item {
                id: profile
                Layout.fillHeight: true
                Layout.preferredWidth: height
                CircularImage {
                    id: imgProfile
                    anchors.fill: profile
                    source: AppAssets.profile
                    // borderWidth: 0
                }
                Rectangle {
                    visible: true
                    width: 12
                    height: 12
                    radius: width / 2
                    anchors.right: imgProfile.right
                    anchors.bottom: imgProfile.bottom
                    // anchors.rightMargin: 0
                    color: root.isActive ?  Colors.success : Colors.error
                }
            }

            // --- Message ---
            Column {
                id: msg
                visible: !root.isCompactMode
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 4
                Label {
                    id: txtUsername
                    text: root.username
                    font.family: Typography.family
                    font.pixelSize: Typography.body
                    font.weight: Typography.medium
                    color: Colors.primary
                    maximumLineCount: 1
                    wrapMode: Text.ElideRight
                    width: msg.width
                }
                Label {
                    text: "Hello, Welcome back."
                    width: msg.width
                    font.family: Typography.family
                    font.pixelSize: Typography.caption
                    font.weight: Typography.regular
                    color: Colors.textSecond
                    maximumLineCount: 1
                    wrapMode: Text.ElideRight

                }
            }

            // --- Status ---
            ColumnLayout {
                visible: !root.isCompactMode
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 4

                Label {
                    text: "Mon 22"
                    color: Colors.primary
                    font.family: Typography.family
                    font.pixelSize: Typography.caption
                    font.weight: Typography.regular
                    Layout.alignment: Qt.AlignRight
                }

                Rectangle {
                    // width: 16
                    // height: 16
                    Layout.preferredHeight: 16
                    Layout.preferredWidth:16
                    Layout.alignment: Qt.AlignRight
                    radius: width / 2
                    clip: true
                    color: Colors.primary
                    Label {
                        text: "1"
                        anchors.fill: parent
                        color: Colors.black500
                        font.family: Typography.family
                        font.pixelSize: Typography.caption
                        font.weight: Typography.medium
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                }
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onClicked: {
            root.itemClicked()
        }
    }

}
