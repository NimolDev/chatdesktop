pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import QtQuick.Window

import Localization
import Theme
import Features.Auth
import ChatApp

import "component" as AppComponent


ApplicationWindow {
    id: window
    width: AppLayouts.width
    height: AppLayouts.height
    minimumWidth: AppLayouts.minWidth
    minimumHeight: AppLayouts.minHeight
    visible: true
    title:  AppStrings.appName

    property bool lightMode: Application.styleHints.colorScheme === Qt.Light
    property color reallyDark: "#1f1f1f"
    property color dark: "#262626"
    property color reallyLight: "#e7e7e7"
    property color light: Colors.primary
    readonly property Window aboutDialog: aboutDialogLoader.item as Window
    menuBar:  Qt.platform.os === "osx" ? menuBar : null

    Component.onCompleted: AppController.checkAuthentication()
    Connections {
        target: AppController
        function onStateChanged() {
            var state = AppController.state
            switch(AppController.state ) {
            case AppController.Unauthenticated:
                stackLayout.currentIndex = 0
                break
            case AppController.Logout:
                stackLayout.currentIndex = 0
                break
            default:
                stackLayout.currentIndex = 1
                break
            }
        }
    }
    Connections {
        target: LoginVM
        function onLoginSucceeded() {
            stackLayout.currentIndex = 1
        }
    }


    MenuBar {
        id: menuBar
          visible: Qt.platform.os === "osx"

        Menu {
            title: qsTr("Help")
            Action {
                text: qsTr("About %1").arg(Application.name)
                onTriggered: {
                    aboutDialogLoader.active = true
                    window.aboutDialog.show()
                    window.aboutDialog.raise()
                    window.aboutDialog.requestActivate()
                }
            }
            Action {
                text: qsTr("Preference...")
            }

        }

        Menu {
            title: qsTr("Edit")
            Action {
                text: qsTr("Undo")
                shortcut: StandardKey.Undo
            }
            Action {
                text: qsTr("Redo")
                shortcut: StandardKey.Redo
            }
            MenuSeparator {}
            Action {
                text: qsTr("Cut")
                shortcut: StandardKey.Cut
            }
            Action {
                text: qsTr("Copy")
                shortcut: StandardKey.Copy
            }
            Action {
                text: qsTr("Paste")
                shortcut: StandardKey.Paste
            }
            Action {
                text: qsTr("Delete")
                shortcut: StandardKey.Delete
                icon.name: "edit-delete"
                // enabled: window.activeFocusItem
                //         && window.activeFocusItem.selectedText !== undefined
                //         && window.activeFocusItem.selectedText.length > 0
            }
        }
    }


    Loader {
        id: aboutDialogLoader
        active: false
        sourceComponent: Component {
            AboutDialog {
                onClosing: Qt.callLater(function() {
                    aboutDialogLoader.active = false
                })
            }
        }
    }


    StackLayout {
        id: stackLayout
        anchors.fill: parent
        currentIndex: 0

        LoginView {
            implicitWidth: 200
            implicitHeight: 200
            onLoginSucceeded: function(userName) {
                window.title = userName
            }
        }

        AppComponent.Menu {

            onLogoutClicked: {

                AppController.logout()
                stackLayout.currentIndex = 0
            }
        }

   }



}
