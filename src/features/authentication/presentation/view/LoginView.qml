import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

import Theme
import Localization
import Shared.UI
import Features.Auth

Page {
    id: loginPage
    implicitWidth: AppLayouts.minWidth
    implicitHeight: AppLayouts.minHeight

    signal loginSucceeded(string userName)

    Pane {
        anchors.fill: parent
        padding: 12

        background: Rectangle {
            color: Colors.background

        }
        ColumnLayout {
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.right: parent.right

            Column {
                Layout.fillWidth: true
                spacing: 4
                Label {
                    text: AppStrings.username
                    font.family: Typography.family
                    font.pixelSize: Typography.body
                    color: Colors.white600
                }

                MyTextField {
                    id: txtUserName
                    width: parent.width
                    height: 45
                    placeholderText: AppStrings.username
                    font.family: Typography.family
                    font.pixelSize: Typography.body
                    font.weight: Typography.medium
                    color: Colors.textPrimary
                }
            }
            Column {
                Layout.fillWidth: true
                spacing: 4
                Label {
                    text: AppStrings.password
                    font.family: Typography.family
                    font.pixelSize: Typography.body
                    color: Colors.white600
                }

                MyTextField {
                    id: txtPassword
                    width: parent.width
                    height: 45
                    placeholderText: AppStrings.password
                    font.family: Typography.family
                    font.pixelSize: Typography.body
                    font.weight: Typography.medium
                    color: Colors.textPrimary
                }
            }


            Item {
                Layout.preferredHeight: 25
            }

            MyButton {
                Layout.fillWidth: true
                Layout.preferredHeight: 45
                text: AppStrings.login
                onClicked: {
                    LoginVM.login(txtUserName.text, txtPassword.text)
                }
            }
        }
    }

    Connections {
        target: LoginVM

        function onLoginSucceeded() {
            console.log("user name", LoginVM.userName)
            console.log("user name", LoginVM.userJid)
            loginPage.loginSucceeded(LoginVM.userName)
        }
        function onConnecting() {
            console.log("Connecting to server...")
        }
    }


}
