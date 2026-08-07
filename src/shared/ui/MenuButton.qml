import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects
import Theme

ItemDelegate {
    id: menu

    implicitWidth: 100
    implicitHeight: 40
    padding: 8
    checkable: true

    property string title: "Setting"
    property url menuIcon: AppAssets.icSetting
    property bool isSelected: true
    property bool isCompactMode: false

    signal menuClicked()

    background: Rectangle {
        radius: 8
        opacity: 0.7

        border.width: 1
        border.color: {

            if (menu.checked)
                return Colors.primary700

            return "transparent"
        }

        gradient: Gradient {
            orientation: Gradient.Horizontal

            GradientStop {
                position: 0.0
                color: menu.checked
                       ? Colors.primary800
                       : "transparent"

                Behavior on color {
                    ColorAnimation {
                        duration: 200
                        easing.type: Easing.OutCubic
                    }
                }
            }

            GradientStop {
                position: 1.0
                color: menu.checked
                       ? Colors.black500
                       : "transparent"

                Behavior on color {
                    ColorAnimation {
                        duration: 200
                        easing.type: Easing.OutCubic
                    }
                }
            }
        }

        Behavior on border.color {
            ColorAnimation {
                duration: 200
                easing.type: Easing.OutCubic
            }
        }
    }

    contentItem: RowLayout {
        spacing: 8

        Item {
            Layout.preferredWidth: 22
            Layout.preferredHeight: 22
            Layout.alignment: Qt.AlignVCenter

            Image {
                id: icon
                anchors.fill: parent
                source: menu.menuIcon
                fillMode: Image.PreserveAspectFit
                visible: false
            }

            MultiEffect {
                anchors.fill: parent
                source: icon

                colorization: 1.0
                colorizationColor: Colors.primary
            }
        }

        Label {
            visible: !menu.isCompactMode
            text: menu.title

            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter

            font.family: Typography.family
            font.pixelSize: Typography.body
            font.weight: Typography.medium
        }
    }


    onClicked: {
        menuClicked()
        // console.log("Click")
    }
    HoverHandler {
        cursorShape: Qt.PointingHandCursor
    }
}