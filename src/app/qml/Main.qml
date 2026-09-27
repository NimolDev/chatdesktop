pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Window

import Localization
import Theme
import Features.Auth
import Features.Chat
import ChatApp

import "component" as AppComponent


ApplicationWindow {
    id: window
    width: AppLayouts.width
    height: AppLayouts.height
    minimumWidth: AppLayouts.minWidth
    minimumHeight: AppLayouts.minHeight
    visible: true
    title: AppController.userName
    color: Colors.error

    // property bool lightMode: Application.styleHints.colorScheme === Qt.Light
    // property color reallyDark: "#1f1f1f"
    // property color dark: "#262626"
    // property color reallyLight: "#e7e7e7"
    property color light: Colors.primary
    // color: "black"

    readonly property Window aboutDialog: aboutDialogLoader.item as Window
    menuBar:  Qt.platform.os === "osx" ? menuBar : null

    Component.onCompleted: AppController.checkAuthentication()

    onClosing: function(close) {
        close.accepted = false
        window.hide()
    }

    Connections {
        target: AppController
        function onStateChanged() {
            var state = AppController.state
            switch(AppController.state ) {
            case AppController.Unauthenticated:
                 pageLoader.sourceComponent = loginPage
                break
            case AppController.Logout:
                 pageLoader.sourceComponent = loginPage
                break
            default:
                pageLoader.sourceComponent = mainWindow
                break
            }
        }
    }

    Connections {
        target: LoginVM
        function onLoginSucceeded() {
            pageLoader.sourceComponent = mainWindow
            window.title = LoginVM.userName
        }
    }


    AboutDialog {
        id: aboutWindow
        objectName: "aboutWindow"
        onClosing: Qt.callLater(function() {
            // aboutDialogLoader.active = false
            aboutWindow.hide()
        })
    }
    Loader {
        id: pageLoader
        anchors.fill: parent
        sourceComponent: loadingPage
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

    Loader {
        id: pageLoader
        anchors.fill: parent
        // color: Colors.background
        sourceComponent: background
    }
    Component {
        id: background
        Rectangle {
            color: Colors.background
        }
    }
    Component {
        id: loadingPage

        Page {
            background: Rectangle {
                color: Colors.background
            }

            BusyIndicator {
                anchors.centerIn: parent
                running: true
            }
        }
    }
    Component {
        id: loginPage
        LoginView {
            implicitWidth: 200
            implicitHeight: 200
            onLoginSucceeded: function(userName) {
                window.title = userName
            }
        }
    }
    Component {
        id: mainWindow

        AppComponent.Menu {
            visible: pageLoader.sourceComponent == mainWindow ? true : false
            onLogoutClicked: {
                AppController.logout()
                pageLoader.sourceComponent = loginPage
                window.title = AppStrings.login
            }
        }
    }

}