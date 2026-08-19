pragma ComponentBehavior: Bound
import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts

import Theme
import Shared.UI
import Localization

ToolBar {
    id: root

    property bool selectionMode: false
    property string userName: ""
    property bool isOnline: false
    property string userStatus: ""
    property int selectedCount: 0

    signal audioClicked()
    signal videoClicked()
    signal moreClicked()
    signal deleteClicked()
    signal forwardClicked()
    signal cancelClicked()

    height: 60
    implicitWidth: 100

    background: Rectangle {
        color: Colors.background
    }

    onWidthChanged: {
        if (moreMenu.visible) {
            moreMenu.close()
        }
    }


    contentItem: Item {
        id: container
        clip: true

        anchors.fill: parent

        Loader {
            id: normalContent

            width: parent.width
            height: parent.height
            y: root.selectionMode ? height : 0
            sourceComponent: normalToolbar

            Behavior on y {
                NumberAnimation {
                    duration: 220
                    easing.type: Easing.InOutCubic
                }
            }
        }

        Loader {
            id: selectionContent

            width: parent.width
            height: parent.height
            y: root.selectionMode ? 0 : -height
            sourceComponent: selectedToolbar

            Behavior on y {
                NumberAnimation {
                    duration: 220
                    easing.type: Easing.InOutCubic
                }
            }
        }
    }

    // --- Normal toolbar ---
    Component {
        id: normalToolbar
        RowLayout {
            anchors.fill: parent
            spacing: 12
            CircularImage {
                Layout.fillHeight: true
                Layout.preferredWidth: height
                Layout.topMargin: 4
                Layout.bottomMargin: 4
                source: AppAssets.profile
                borderWidth: 0
            }
            Column {
                Label {
                    text: root.userName
                    font.family: Typography.family
                    font.pixelSize: Typography.title4
                    font.weight: Typography.medium
                }
                Label {
                    text: root.isOnline ? AppStrings.online : root.userStatus
                    font.family: Typography.family
                    font.pixelSize: Typography.caption
                    font.weight: Typography.regular
                }
            }

            Item {
                Layout.fillWidth: true
            }

            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredHeight: 30
                spacing: 4
                CircleButton {
                    id: btnCall
                    Layout.preferredHeight: 40
                    Layout.preferredWidth: height
                    padding: 10
                    iconSource: AppAssets.icPhone
                    borderColor: Colors.suface3
                    tintColor: Colors.primary
                    hoverColor: Colors.primaryHover
                    onClicked: root.audioClicked()
                }
                CircleButton {
                    id: btnVideo
                    Layout.preferredHeight: 40
                    Layout.preferredWidth: height
                    padding: 8
                    iconSource: AppAssets.icVideo
                    borderColor: Colors.suface3
                    tintColor: Colors.primary
                    hoverColor: Colors.primaryHover
                    onClicked: root.videoClicked()
                }
                CircleButton {
                    id: btnMore
                    Layout.preferredHeight: 40
                    Layout.preferredWidth: height
                    padding: 12
                    iconSource: AppAssets.icMore
                    borderColor: Colors.suface3
                    tintColor: Colors.primary
                    hoverColor: Colors.primaryHover
                    onClicked: {
                        root.moreClicked()

                        if (moreMenu.opened) {
                            moreMenu.close()
                            return
                        }

                        const position = btnMore.mapToItem(
                            moreMenu.parent,
                            btnMore.width,
                            btnMore.height
                        )
                        moreMenu.x = position.x - moreMenu.width
                        moreMenu.y = position.y + 4
                        moreMenu.open()

                    }
                }

            }
        }

    }


    // --- Selection Mode toolbar ---
    Component {
        id: selectedToolbar
        RowLayout {
            anchors.fill: parent
            anchors {
                topMargin: AppLayouts.x_padding
                bottomMargin: AppLayouts.x_padding
            }
            spacing: 4

            ToolbarButton {
                id: btnForward
                Layout.fillHeight: true
                text: AppStrings.forward.toUpperCase()
                number: root.selectedCount
                background: Rectangle {
                    color: Colors.primary600
                    topLeftRadius: AppLayouts.l_radius
                    bottomLeftRadius: AppLayouts.l_radius
                    topRightRadius: AppLayouts.s_radius
                    bottomRightRadius: AppLayouts.s_radius
                }
                onClicked: root.forwardClicked()
            }

            ToolbarButton {
                id: btnDelete
                Layout.fillHeight: true
                text: AppStrings.deleteText.toUpperCase()
                number: root.selectedCount
                background: Rectangle {
                    color: Colors.primary600
                    topLeftRadius: AppLayouts.s_radius
                    bottomLeftRadius: AppLayouts.s_radius
                    topRightRadius: AppLayouts.l_radius
                    bottomRightRadius: AppLayouts.l_radius
                }
                onClicked: root.deleteClicked()
            }

            Item {
                Layout.fillWidth: true
            }

            ToolbarButton {
                id: btnCancel
                Layout.fillHeight: true
                text: AppStrings.cancel.toUpperCase()
                onClicked: root.cancelClicked()
            }
        }

    }


    Menu {
        id: moreMenu

        padding: AppLayouts.m_padding
        transformOrigin: Item.TopRight

        implicitWidth: Math.max(
            viewProfileItem.implicitWidth,
            mediaFilesItem.implicitWidth,
            muteItem.implicitWidth,
            blockItem.implicitWidth
        ) + leftPadding + rightPadding


        background: Rectangle {
            color: Colors.suface1
            radius: AppLayouts.x_radius
            border.color: Colors.suface3
        }

        ContextMenuItem {
            id: viewProfileItem
            text: AppStrings.viewProfile
            icon.source: AppAssets.icProfile
        }

        ContextMenuItem {
            id: mediaFilesItem
            text: AppStrings.mediaAndFiles
            icon.source: AppAssets.icPhoto
        }

        ContextMenuItem {
            id: muteItem
            text: AppStrings.muteNotification
            icon.source: AppAssets.icBellSlash
        }

        ContextMenuItem {
            id: blockItem
            text: AppStrings.blockUser
            icon.source: AppAssets.icNoSign
            tintColor: Colors.red500

        }
    }
}
