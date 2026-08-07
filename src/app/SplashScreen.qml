// shared/ui/SplashScreen.qml
import QtQuick
import QtQuick.Controls
import Theme
// import Localization

Item {
    id: root

    property string appName: "App Name"
    property real progress: 0.0        // bind to your real loading progress if you have one
    signal finished()

    anchors.fill: parent

    Rectangle {
        anchors.fill: parent
        color: Colors.background
    }

    Column {
        anchors.centerIn: parent
        spacing: 24

        Image {
            id: logo
            source: "qrc:/assets/logo.png"   // swap for your actual logo path
            width: 96
            height: 96
            anchors.horizontalCenter: parent.horizontalCenter
            fillMode: Image.PreserveAspectFit

            opacity: 0
            scale: 0.8

            // NumberAnimation on opacity {
            //     to: 1
            //     duration: 400
            //     easing.type: Easing.OutCubic
            // }
            // NumberAnimation on scale {
            //     to: 1
            //     duration: 400
            //     easing.type: Easing.OutBack
            // }
        }

        Text {
            text: root.appName
            font.family: Typography.family
            color: Colors.textPrimary
            anchors.horizontalCenter: parent.horizontalCenter
            opacity: 0

            // NumberAnimation on opacity {
            //     to: 1
            //     duration: 400
            //     delay: 150
            //     easing.type: Easing.OutCubic
            // }
        }

        // Simple indeterminate loading indicator.
        // Swap for a ProgressBar bound to root.progress if you have real loading steps.
        BusyIndicator {
            running: true
            anchors.horizontalCenter: parent.horizontalCenter

            palette.dark: Colors.suface3
        }
    }

    // Call root.finished() when your app's actual init work completes,
    // or fake a minimum display time like this:
    Timer {
        interval: 1500
        running: true
        onTriggered: root.finished()
    }
}