pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Window

import Localization
import Theme
import Features.Auth
import ChatApp
import Shared.UI
import App.Core 1.0

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
     // flags: Qt.Window | Qt.FramelessWindowHint

    Component.onCompleted: {
        AppController.checkAuthentication()
    }

    onClosing: function(close) {
        close.accepted = false
        window.hide()
    }
    Connections {

        target: NetworkMonitor

        function onNetworkChanged(online) {

            console.log("Network online:", online)
            // if (online) {

            // }
            // networkStatus.visible = online
            // networkStatus.isOnline = online
            // if (!online) {
            // networkStatus.showStatus()
            // }

        }

        function onNetworkConnected() {

            console.log("Network connected")
            networkStatus.hideStatus()

        }

        function onNetworkDisconnected() {

            console.log("Network disconnected")


            networkStatus.showStatus()

        }

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

    Rectangle {
        id: networkStatus

        visible: false
        property bool isOnline: false

        width: content.implicitWidth + AppLayouts.x_padding
        height: content.implicitHeight + AppLayouts.x_padding

        x: (parent.width - width) / 2
        y: -height

        radius: AppLayouts.x_radius
        clip: true
        z: 10

        color: "transparent"

        border.width: 1
        border.color: "#45FFFFFF"

        readonly property real duration: 450

        gradient: Gradient {
            orientation: Gradient.Horizontal

            GradientStop {
                position: 0.0
                color: "#50FFFFFF"
            }

            GradientStop {
                position: 0.5
                color: "#25FFFFFF"
            }

            GradientStop {
                position: 1.0
                color: "#15FFFFFF"
            }
        }

        Row {
            id: content
            anchors.centerIn: parent
            spacing: AppLayouts.m_padding

            CircleLoading {
                width: 20
                height: 20
                strokeWidth: 2
            }

            Text {
                anchors.verticalCenter: parent.verticalCenter

                text: networkStatus.isOnline
                      ? "Connecting"
                      : "Waiting for network"

                color: networkStatus.isOnline
                       ? Colors.success
                       : Colors.error
            }
        }

        ParallelAnimation {
            id: showAnimation

            NumberAnimation {
                target: networkStatus
                property: "y"
                from: -networkStatus.height
                to: AppLayouts.s_padding
                duration: networkStatus.duration
                easing.type: Easing.OutCubic
            }

            ScaleAnimator {
                target: networkStatus
                from: 0.85
                to: 1.0
                duration: networkStatus.duration
                easing.type: Easing.OutBack
            }
        }
        ParallelAnimation {
            id: hideAnimation
            NumberAnimation {
                target: networkStatus
                property: "y"
                from: AppLayouts.s_padding
                to: -networkStatus.height
                duration: networkStatus.duration
                easing.type: Easing.InCubic
            }

            ScaleAnimator {
                target: networkStatus
                from: 1.0
                to: 0.85
                duration: networkStatus.duration
                easing.type: Easing.InBack
            }
            onFinished: {
                networkStatus.visible = false
            }
        }

        function showStatus() {
            hideAnimation.stop()

            visible = true
            showAnimation.restart()
        }
        function hideStatus() {
            if (!visible)
                return

            showAnimation.stop()
            hideAnimation.restart()
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
    Component {
        id: loadingPage

        Page {
            background: Rectangle {
                color: Colors.background
            }

            //  {
            //     anchors.centerIn: parent
            //    BusyIndicator running: true
            // }
            CircleLoading {
                anchors.centerIn: parent
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
