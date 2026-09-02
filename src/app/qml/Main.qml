pragma ComponentBehavior: Bound

import QtQuick
// import QtQuick.Layouts
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

    property bool lightMode: Application.styleHints.colorScheme === Qt.Light
    property color reallyDark: "#1f1f1f"
    property color dark: "#262626"
    property color reallyLight: "#e7e7e7"
    property color light: Colors.primary
    // readonly property Window aboutDialog: aboutDialogLoader.item as Window

    Component.onCompleted: {

        // desktopPet.show()
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
                pageLoader.sourceComponent = chatPage

                break
            }
        }
    }

    Connections {
        target: LoginVM
        function onLoginSucceeded() {
            pageLoader.sourceComponent = chatPage
            window.title = LoginVM.userName
        }
    }

    PetWindow {
        id: desktopPet

        visible: false

        x: 0
        y: 0
    }

    // Loader {
    //     id: aboutDialogLoader
    //     active: false
    //     sourceComponent: Component {
    //         AboutDialog {
    //             onClosing: Qt.callLater(function() {
    //                 aboutDialogLoader.active = false
    //             })
    //         }
    //     }
    // }

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
        id: chatPage

        AppComponent.Menu {
            visible: pageLoader.sourceComponent == chatPage ? true : false
            onLogoutClicked: {
                AppController.logout()
                pageLoader.sourceComponent = loginPage
                window.title = AppStrings.login
            }
        }
    }


}
