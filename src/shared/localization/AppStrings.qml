pragma Singleton
import QtQuick

QtObject {
    readonly property string appName: Application.name

    readonly property string login: qsTr("Login")
    readonly property string logout: qsTr("Logout")

    readonly property string username: qsTr("Username")
    readonly property string password: qsTr("Password")

    readonly property string send: qsTr("Send")
    readonly property string cancel: qsTr("Cancel")
    readonly property string save: qsTr("Save")
    readonly property string deleteText: qsTr("Delete")

    readonly property string chat: qsTr("Chat")
    readonly property string contacts: qsTr("Contacts")
    readonly property string settings: qsTr("Settings")
    readonly property string iot: qsTr("IoT")
    readonly property string utility: qsTr("Utilities")

    readonly property string user: qsTr("User")
    readonly property string forward: qsTr("forward")
    readonly property string online: qsTr("Online")
    readonly property string viewProfile: qsTr("View Profile")
    readonly property string mediaAndFiles: qsTr("Media And Files")
    readonly property string muteNotification: qsTr("Mute Notification")
    readonly property string blockUser: qsTr("Block User")
    readonly property string profile: qsTr("Profile")
    readonly property string videoCall: qsTr("Video Call")
    readonly property string audioCall: qsTr("Audio Call")
    readonly property string message: qsTr("Message")

}
