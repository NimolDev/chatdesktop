import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts

import Shared.UI
import Theme
import Localization

import Features.Auth
import Features.Chat

Page {
    id: root
    implicitHeight: 480
    implicitWidth: 730
    // anchors.fill: parent

    property bool isCompactMode: width <= AppLayouts.minWidth
    readonly property int _padding: 8
    readonly property int _menuHeight: 40

    background: Rectangle {
        color: Colors.background
    }

    onIsCompactModeChanged: {
        txtUserName.visible = !isCompactMode
        txtUserNumber.visible = !isCompactMode
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // --- Side Menu ----
        Rectangle {
            id: sideMenu
            Layout.preferredWidth: root.isCompactMode ? (root._padding*2 + root._menuHeight) : 150
            Layout.fillHeight: true
            color: Colors.background
            // Behavior on Layout.preferredWidth {
            //     NumberAnimation {
            //         duration: 100
            //         easing.type: Easing.OutInCubic
            //     }
            // }


            // -- Seperate  Line ---
            Rectangle {
                anchors.top: sideMenu.top
                anchors.bottom: sideMenu.bottom
                anchors.right: sideMenu.right
                width: 1
                color: Colors.suface2
            }

            Pane {
                anchors.fill: parent
                padding: 8
                background: Rectangle {
                    color: "transparent"
                }

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 3

                    // --- Header ---
                    Row {
                        Avatar {
                            source: AppAssets.image
                            borderWidth: 0
                            width: 40
                            height: 40
                        }
                        Text {
                            visible: !root.isCompactMode
                            text: AppStrings.appName
                            color: Colors.primary
                            anchors.verticalCenter: parent.verticalCenter
                            font.family: Typography.family
                            font.pixelSize: Typography.title3
                            font.weight: Typography.bold
                        }
                    }

                    MenuSeparator {
                        Layout.fillWidth: true
                    }


                    ButtonGroup {
                        id: menuGroup
                    }

                    MenuButton {
                        id: contact
                        Layout.preferredHeight: root._menuHeight
                        // Layout.preferredWidth: 40
                        Layout.fillWidth: true
                        title: AppStrings.contacts
                        menuIcon: AppAssets.icContact
                        ButtonGroup.group: menuGroup
                        checked: true
                        isCompactMode: root.isCompactMode
                    }
                    MenuButton {
                        id: chat
                        Layout.preferredHeight: root._menuHeight
                        Layout.fillWidth: true
                        title: AppStrings.chat
                        menuIcon: AppAssets.icChat
                        ButtonGroup.group: menuGroup
                        isCompactMode: root.isCompactMode
                        onClicked: {
                        }
                    }
                    MenuButton {
                        Layout.preferredHeight: root._menuHeight
                        Layout.fillWidth: true
                        title: AppStrings.iot
                        menuIcon: AppAssets.icIoT
                        ButtonGroup.group: menuGroup
                        isCompactMode: root.isCompactMode
                    }
                    MenuButton {
                        Layout.preferredHeight: root._menuHeight
                        Layout.fillWidth: true
                        title: AppStrings.utility
                        menuIcon: AppAssets.icUtility
                        ButtonGroup.group: menuGroup
                        isCompactMode: root.isCompactMode
                    }
                    MenuButton {
                        Layout.preferredHeight: root._menuHeight
                        Layout.fillWidth: true
                        title: AppStrings.settings
                        menuIcon: AppAssets.icSetting
                        ButtonGroup.group: menuGroup
                        isCompactMode: root.isCompactMode
                    }






                    Item {
                        Layout.fillHeight: true
                    }

                    Pane {
                        Layout.fillWidth: true
                        padding: 4
                        background: Rectangle {
                            visible: !root.isCompactMode
                            radius: AppLayouts.x_padding
                            color: Colors.suface1
                            border.color: Colors.suface3
                        }
                        ColumnLayout {
                            anchors.horizontalCenter: parent.horizontalCenter

                            Avatar {
                                Layout.preferredHeight: 45
                                Layout.preferredWidth: 45
                                // source: AppAssets.profile
                                source: "https://photosvibes.com/wp-content/uploads/2026/05/shinchan-pic-3.jpg"
                                borderWidth: 0
                                Layout.alignment: Qt.AlignHCenter
                            }
                            Label {
                                id: txtUserName
                                Layout.fillWidth: true
                                text: "Shinnosuke"
                                horizontalAlignment: Text.AlignHCenter
                                font.family: Typography.family
                                font.pixelSize: Typography.body
                                font.weight: Typography.medium
                                maximumLineCount: 2
                            }

                            Label {
                                id: txtUserNumber
                                Layout.fillWidth: true
                                text: "+855 96xxxxxx"
                                color: Colors.white300
                                horizontalAlignment: Text.AlignHCenter
                                font.family: Typography.family
                                font.pixelSize: Typography.caption
                                maximumLineCount: 2
                            }
                        }
                    }

                    GridLayout {
                        Layout.fillWidth: true
                        columns: root.isCompactMode ? 1 : 2
                        Layout.alignment: Qt.AlignHCenter
                        columnSpacing: 0

                        Button {
                            Layout.preferredHeight: root._menuHeight
                            Layout.preferredWidth: root._menuHeight
                            icon.source: AppAssets.icQR
                            icon.color: Colors.primary
                            icon.height: 30
                            icon.width: 30
                            background: Rectangle {
                                color: "transparent"
                            }
                            HoverHandler {
                                cursorShape: Qt.PointingHandCursor
                            }
                        }

                        Button {
                            Layout.preferredHeight: root._menuHeight
                            Layout.preferredWidth: root._menuHeight
                            icon.source: AppAssets.icExit
                            icon.color: Colors.red500
                            icon.height: 30
                            icon.width: 30
                            padding: 0

                            background: Rectangle {
                                color: "transparent"
                            }
                            HoverHandler {
                                cursorShape: Qt.PointingHandCursor
                            }
                        }


                    }

                    MenuSeparator {
                        Layout.fillWidth: true
                    }
                    Label {
                        id: txtAppVersion

                        text: !root.isCompactMode ? Application.name + " Desktop " + Application.version : Application.version
                        wrapMode: Text.ElideRight
                        color: Colors.primary
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignHCenter
                        font.family: Typography.family
                        font.pixelSize: Typography.caption
                    }
                }
            }
        }

        // --- Loader ----

        Loader {
          id: loader
          Layout.fillWidth: true
          Layout.fillHeight: true
          sourceComponent: chatPage
        }
    }

    // Page Component
    Component {
        id: auth
        LoginView {
            anchors.fill: parent
        }
    }
    Component {
        id: chatPage
        ChatPage {
            anchors.fill: parent
        }
    }



}
