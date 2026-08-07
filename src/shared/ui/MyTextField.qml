import QtQuick 2.15
import QtQuick.Controls.Basic
import Theme


TextField {
    id: input
    implicitHeight: 50
    implicitWidth: 250
    color: Colors.textPrimary
    font.family: Typography.family
    font.pixelSize: Typography.title4
    font.weight: Typography.regular
    placeholderText: qsTr("placeholder..")
    background: Rectangle {
        radius: 8
        color: Colors.suface1
        border.color: Colors.suface3
    }


    ContextMenu.menu: Menu {

            width: 150

            padding: 6
            background: Rectangle {
                radius: 12
                color: Colors.suface1
                border.color: Colors.suface3
            }

            MenuItem {
                text: qsTr("Cut")
                enabled: input.selectedText.length > 0 && !input.readOnly
                onTriggered: input.cut()
            }

            MenuItem {
                text: qsTr("Copy")
                enabled: input.selectedText.length > 0
                onTriggered: input.copy()
            }

            MenuItem {
                text: qsTr("Paste")
                enabled: input.canPaste && !input.readOnly
                onTriggered: input.paste()
            }

            MenuSeparator {}

            MenuItem {
                text: qsTr("Select All")
                enabled: input.text.length > 0
                onTriggered: input.selectAll()
            }

            MenuItem {
                id: item
                text: qsTr("Delete")
                contentItem: Text {

                    text: item.text
                    color: (input.selectedText.length > 0 && !input.readOnly) ? "red" : item.palette.buttonText
                }
                enabled: input.selectedText.length > 0 && !input.readOnly
                onTriggered: input.remove(
                                 input.selectionStart,
                                 input.selectionEnd
                                 )
            }

        }


}