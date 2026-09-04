pragma ComponentBehavior: Bound

import QtQuick
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
    title: AppController.userName


    Component.onCompleted: {
        AppController.checkAuthentication()
    }

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
            // pageLoader.sourceComponent = mainWindow
            window.title = LoginVM.userName
        }
    }

    PetWindow {
        id: desktopPet

        visible: false

        x: 0
        y: 0
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
