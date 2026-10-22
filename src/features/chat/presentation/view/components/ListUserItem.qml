import QtQuick 2.15
import QtQuick.Layouts
import QtQuick.Controls.Basic

import Theme
import Shared.UI
// import App.Components

Item {
    id: root
    implicitWidth: 300
    // implicitHeight: 30
    // anchors.fill: parent
    property bool isActive: true
    property bool isCompactMode: false
    property bool isSeen: true
    property int padding: 0
    property string username: ""
    property string message: ""
    property bool isOnline: true


    property bool isSelected: false
    signal itemClicked()

    Rectangle {
        id: pane
        anchors.fill: parent
        color:  root.isSelected ? Colors.primary800 : (mouseArea.containsMouse ? Colors.suface1 : Colors.background)
        enabled: true

        RowLayout {
            anchors.fill: parent
            anchors.margins: AppLayouts.m_padding
            // anchors.rightMargin: AppLayouts.m_padding

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
                    visible: root.isOnline
                    width: 14
                    height: 14
                    radius: width / 2
                    anchors.right: imgProfile.right
                    anchors.bottom: imgProfile.bottom
                    // anchors.rightMargin: 0
                    color: root.isActive ?  Colors.success : Colors.error
                    border.width: 2
                    border.color: root.isSelected ? Colors.primary800 : (mouseArea.containsMouse ? Colors.suface1 : Colors.background)
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
        id: mouseArea
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        hoverEnabled: true
        onClicked: {
            root.itemClicked()
        }
    }

}
