import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import Theme

Window {
    id: root

    width: 480
    height: 300

    visible: false
    title: qsTr("About %1").arg(Application.name)

    minimumWidth: width
    maximumWidth: width

    minimumHeight: height
    maximumHeight: height

    flags: Qt.Window
            | Qt.CustomizeWindowHint
            | Qt.WindowTitleHint
            | Qt.WindowCloseButtonHint
            | Qt.WindowMaximizeButtonHint

    modality: Qt.ApplicationModal

    Pane {
        anchors.fill: parent
        padding: 16
        ColumnLayout {
            anchors.fill: parent

            RowLayout {
                Layout.fillWidth: true
                spacing: 16

                Image {
                    id: imgAppLogo

                    source: AppAssets.image
                    fillMode: Image.PreserveAspectFit

                    Layout.preferredWidth: 100
                    Layout.preferredHeight: 100
                    Layout.alignment: Qt.AlignVCenter
                }

                Text {
                    text: Application.name

                    color: Colors.primary
                    font.family: Typography.family
                    font.pixelSize: 0
                    font.weight: Typography.bold

                    Layout.alignment: Qt.AlignVCenter
                }
            }
            Text {
                Layout.fillWidth: true
                Layout.fillHeight: true
                text:"Sending messages, files,voices and stickers are as easy as ever.Within a few click you can connect with your people any time and anywhere as long as you have internet connection."

                wrapMode: Text.WordWrap
                color: Colors.textPrimary
                font.family: Typography.family
                font.pixelSize: 0
                font.weight: Typography.regular
            }

            Button {

                Layout.preferredHeight: 40
                Layout.preferredWidth: 100
                Layout.alignment: Qt.AlignRight
                text: "Close"
                font.family: Typography.family
                font.pixelSize: Typography.body
                font.weight: Typography.medium

                background: Rectangle {
                    radius: 8
                    color: Colors.suface1
                    border.color: Colors.suface3
                }
                onClicked: root.close()

            }

        }
    }

}
