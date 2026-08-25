pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic


import Shared.UI
import Theme
// import Features.Chat

import "components" as Components

Item {
    id: root

    property int itemHeight: 60
    property int padding: 0
    property bool isCompactMode: false
    property var model


    signal expandRequested()

    property bool isSelected: false

    signal searchClicked()
    signal itemClicked(int index,string userName ,string userId)


    // Component.onCompleted: {
    //     ConversationsVM.fetchConversations();
    // }

    Pane {
        padding: 4
        anchors.fill: parent
        ColumnLayout {
            anchors.fill: parent
            spacing: 4

            SearchField {
                id: search
                visible: !root.isCompactMode
                Layout.preferredHeight: 35
                Layout.fillWidth: true
                placeholderText: "Searching.."

            }

            Button {
                id: btnSearch
                visible: root.isCompactMode
                icon.source: AppAssets.icSearch
                Layout.preferredWidth: 35
                Layout.preferredHeight: 35
                icon.width: width  * 0.8
                icon.height: height * 0.8
                padding: 0
                Layout.alignment: Qt.AlignHCenter
                background: Rectangle {
                    color: "transparent"
                }
                onClicked:  {
                    root.expandRequested()
                }
            }

            ListView {
                id: listUser
                model: root.model
                Layout.fillWidth: true
                Layout.fillHeight: true

                clip: true
                spacing: 1
                delegate: Components.ListUserItem {
                    required property int index
                    required property var model
                    id: list
                    width: ListView.view.width
                    height: root.itemHeight
                    padding: root.padding
                    isCompactMode: root.isCompactMode
                    username: model.name

                    isSelected: root.isSelected ? listUser.currentIndex === index : 0

                    onItemClicked: {
                        listUser.currentIndex = index
                        root.isSelected = true
                        root.itemClicked(index,model.name, model.uuid)
                        // console.log(model.uuid)
                    }
                }

                ScrollBar.vertical: ScrollBar {
                    policy: ScrollBar.AsNeeded
                }

            }

        }
    }



}
