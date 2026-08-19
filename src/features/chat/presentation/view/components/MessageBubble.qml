import QtQuick 2.15
import QtQuick.Layouts
import QtQuick.Effects
import QtQuick.Controls

import Features.Chat
import Theme

Item {

    id: root
    implicitWidth: 600
    implicitHeight: contentItem.height + 8

     property alias bubbleItem: bubble

    property int maxBubbleWidth: 400
    property bool isOutgoing: true
    property bool isSeen: false
    property string msg: ""
    property string date: ""
    property bool selectionMode: false

    readonly property int padding: 8
    readonly property bool isMaxWidth: txtMessage.implicitWidth > (maxBubbleWidth - (msgStatus.implicitWidth + 8*5))

    // --- selection state ---
    property bool selected: false

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: root.selectionMode ? checkbox.width  : 0

        Behavior on anchors.leftMargin {
            NumberAnimation { duration: 120 }
        }
        Item {
            visible: root.isOutgoing
            Layout.fillWidth: true
        }

        Bubble {
            id: bubble
            Layout.preferredWidth: Math.min(400, Math.max(50, contentItem.width + 8))
            Layout.preferredHeight: /*Math.max(45, contentItem.height)*/ contentItem.height + 8

            outgoing: root.isOutgoing
            showTail: true

            color: root.selected ? Colors.primary700 : root.isOutgoing ? Colors.primary800.alpha(0.7) : Colors.black800.alpha(0.7)
            GridLayout {
                id: contentItem
                columns: root.isMaxWidth ? 1 : 2
                rowSpacing: 0
                columnSpacing: 0

                Text {
                    id: txtMessage
                    Layout.topMargin: root.padding
                    Layout.bottomMargin: root.padding
                    Layout.leftMargin: root.isOutgoing ? root.padding : root.padding * 2.5
                    Layout.rightMargin:  root.padding

                    Layout.preferredWidth: width <= 400 - 24 ? width  : (400 - 24)
                    Layout.fillHeight: true
                    text: root.msg
                    wrapMode: Text.ElideNone

                    maximumLineCount: 0
                    color: Colors.textPrimary
                    font.family: Typography.family
                    font.pixelSize: Typography.body
                    font.weight: Typography.regular
                }

                RowLayout {
                    id: msgStatus
                    Layout.alignment: root.isMaxWidth ? Qt.AlignRight : Qt.AlignBottom
                    Layout.rightMargin: root.isOutgoing ? root.padding : root.isMaxWidth ? root.padding : 0

                    Text {
                        id: txtDate

                        text: root.date
                        color: Colors.primary600
                        font.family: Typography.family
                        font.pixelSize: Typography.caption
                        font.weight: Typography.regular
                    }

                    Item {

                        visible: root.isOutgoing
                        Layout.preferredHeight: root.isSeen ? 20 : 15
                        Layout.preferredWidth: height

                        Image {
                            id: icon
                            anchors.fill: parent
                            source: root.isSeen ? AppAssets.icDuobleCheck :  AppAssets.icCheck
                            visible: false
                            fillMode: Image.PreserveAspectFit
                        }

                        MultiEffect {
                            anchors.fill: icon
                            source: icon
                            brightness: 1.0

                            colorization: 1.0
                            colorizationColor: Colors.primary
                        }
                    }
                }
            }

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    console.log("Hello")
                }
            }
        }

    }

    // --- Check Box ---
    CheckBox {
        id: checkbox
        visible: root.selectionMode
        checked: root.selected
        anchors.left: parent.left
        anchors.leftMargin: root.padding
        anchors.verticalCenter: parent.verticalCenter
        opacity: visible ? 1 : 0
        enabled: false

        indicator: Rectangle {
            implicitWidth: 22
            implicitHeight: 22
            // anchors.fill: parent
            radius: width / 2
            color: checkbox.checked ? Colors.primary : "transparent"
            border.width: 2
            border.color: checkbox.checked
                          ? Colors.primary
                          : Colors.primary300

            Text {
                anchors.centerIn: parent
                text: "\u2713"
                visible: checkbox.checked
                color: Colors.textPrimary
                font.pixelSize: 14
                font.bold: true
            }

            Behavior on color {
                ColorAnimation { duration: 100 }
            }
        }

        Behavior on opacity {
            NumberAnimation { duration: 120 }
        }
    }




}
