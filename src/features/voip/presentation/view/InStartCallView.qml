pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtMultimedia
import QtQuick.Effects
import QtQuick.Controls

import Features.Voip
import Shared.UI
import Theme

Item {
    id: root

    implicitWidth: 500
    implicitHeight: 500

    property CallVM viewmodel: null
    property bool isVideoCall

    property bool _isMute: false
    property bool _isVideo: false
    property bool _isEnableSpeaker: true
    property bool _isEnableCamera: root.isVideoCall ? true : false
    readonly property bool _isRemoteArrive: root.viewmodel !== null && root.viewmodel.hasRemoteVideo

    signal endCall()
    signal muteChanged(bool mute)
    signal speakerChanged(bool mute)
    signal cameraChanged(bool enable)

    // Keep source controls above both video surfaces.
    Rectangle {
        z: 2
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: AppLayouts.x_padding
        anchors.topMargin: 40
        height: sourceLayout.implicitHeight + 16
        radius: 12
        color: Colors.background
        opacity: 0.95

        GridLayout {
            id: sourceLayout
            anchors.fill: parent
            anchors.margins: 8
            columns: 2
            columnSpacing: 12

            Label { text: qsTr("Audio input"); color: Colors.textPrimary }
            ComboBox {
                id: audioInputSelector
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                model: root.viewmodel ? root.viewmodel.audioInputs : null
                textRole: "name"
                currentIndex: root.viewmodel ? root.viewmodel.selectedAudioInputIndex : -1
                enabled: root.viewmodel !== null && count > 0
                displayText: count > 0 ? currentText : qsTr("No microphone available")
                Accessible.name: qsTr("Audio input")
                onActivated: index => {
                    if (root.viewmodel) {
                        root.viewmodel.selectAudioInput(index)
                    }
                }
            }
            Label { text: qsTr("Audio output"); color: Colors.textPrimary }
            ComboBox {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                model: root.viewmodel ? root.viewmodel.audioOutputs : null
                textRole: "name"
                currentIndex: root.viewmodel ? root.viewmodel.selectedAudioOutputIndex : -1
                enabled: root.viewmodel !== null && count > 0
                displayText: count > 0 ? currentText : qsTr("No speaker available")
                Accessible.name: qsTr("Audio output")
                onActivated: index => {
                    if (root.viewmodel) {
                        root.viewmodel.selectAudioOutput(index)
                    }
                }
            }
            Label {
                visible: root.isVideoCall
                text: qsTr("Video input")
                color: Colors.textPrimary
            }
            ComboBox {
                visible: root.isVideoCall
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                model: root.viewmodel ? root.viewmodel.cameras : null
                textRole: "name"
                currentIndex: root.viewmodel ? root.viewmodel.selectedCameraIndex : -1
                enabled: root.viewmodel !== null && count > 0
                displayText: count > 0 ? currentText : qsTr("No camera available")
                Accessible.name: qsTr("Video input")
                onActivated: index => {
                    if (root.viewmodel) {
                        root.viewmodel.selectCamera(index)
                    }
                }
            }
        }
    }
    VideoOutput {
        id: remoteVideo
        anchors.fill: parent
        visible: root.isVideoCall && root.viewmodel !== null
        fillMode: VideoOutput.PreserveAspectFit
        mirrored: false

        Binding {
            target: root.viewmodel
            property: "remoteVideoSink"
            value: remoteVideo.videoSink
            when: root.viewmodel !== null && root.isVideoCall
            restoreMode: Binding.RestoreBindingOrValue
        }
    }
    // Keep the local camera full size until a valid remote frame arrives.
    Item {
        id: localPreview

        readonly property bool isPreview: root._isRemoteArrive
        readonly property real previewWidth: Math.max(0, Math.min(
            180, root.width * 0.32,
            root.width - 2 * AppLayouts.x_padding,
            (callControls.y - 2 * AppLayouts.x_padding) * 16 / 9))
        readonly property real previewHeight: previewWidth * 9 / 16
        property real previewProgress: isPreview ? 1 : 0

        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.rightMargin: AppLayouts.x_padding * previewProgress
        anchors.bottomMargin: (root.height - callControls.y ) * previewProgress
        width: root.width + (previewWidth - root.width) * previewProgress
        height: root.height + (previewHeight - root.height) * previewProgress
        visible: root.isVideoCall && root.viewmodel !== null
        clip: true

        // Animate only the change to picture-in-picture. Window resizing must
        // update the geometry immediately, without restarting four animations.
        Behavior on previewProgress {
            NumberAnimation { duration: 500; easing.type: Easing.InOutCubic }
        }

        VideoOutput {
            id: localPreviewVideo
            anchors.fill: parent
            fillMode: VideoOutput.PreserveAspectFit
            mirrored: true

            Binding {
                target: root.viewmodel
                property: "localVideoSink"
                value: localPreviewVideo.videoSink
                when: root.viewmodel !== null && root.isVideoCall
                restoreMode: Binding.RestoreBindingOrValue
            }
            layer.enabled: true
            layer.effect: MultiEffect {
                maskEnabled: localPreview.isPreview
                maskSource: ShaderEffectSource {
                    sourceItem: Rectangle {
                        width: localPreviewVideo.width
                        height: localPreviewVideo.height
                        radius: localPreview.isPreview ? 16 : 0
                    }

                }

            }
        }
    }


    RowLayout {
        id: callControls
        anchors {
            bottom: parent.bottom
            bottomMargin: 12
            horizontalCenter: parent.horizontalCenter
        }
        spacing: 12

        Column {
            spacing: AppLayouts.s_padding
            CircleButton {
                id: btnSpeaker
                Layout.preferredHeight: 40
                Layout.preferredWidth: 40
                anchors.horizontalCenter: parent.horizontalCenter
                // padding: AppLayouts.x_padding
                iconSource: root._isEnableSpeaker ?  AppAssets.icSpeaker : AppAssets.icSpeakderSlash
                color: Colors.primary700
                borderWidth: 0
                onClicked: {
                    root._isEnableSpeaker = !root._isEnableSpeaker
                    root.speakerChanged(root._isEnableSpeaker)
                }
            }
            Text {
                text: "Speaker"
                font.family: Typography.family
                font.pixelSize: Typography.body
                font.weight: Typography.medium
                color: Colors.textPrimary
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }
        Column {
            spacing: AppLayouts.s_padding
            CircleButton {
                id: btnEnd
                Layout.preferredHeight: 40
                Layout.preferredWidth: 40
                anchors.horizontalCenter: parent.horizontalCenter
                // padding: AppLayouts.x_padding
                iconSource: AppAssets.icPhoneDown
                color: Colors.error
                borderWidth: 0
                onClicked: root.endCall()

            }
            Text {
                text: "End Call"
                font.family: Typography.family
                font.pixelSize: Typography.body
                font.weight: Typography.medium
                color: Colors.textPrimary
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }
        Column {
            spacing: AppLayouts.s_padding
            CircleButton {
                id: btnMute
                Layout.preferredHeight: 40
                Layout.preferredWidth: 40
                anchors.horizontalCenter: parent.horizontalCenter
                padding: AppLayouts.x_padding
                iconSource: root._isMute ? AppAssets.icMicrophoneSlash : AppAssets.icMicrophone
                color: Colors.primary700
                borderWidth: 0
                onClicked: {
                    root._isMute = !root._isMute
                    root.muteChanged(root._isMute)
                }

            }
            Text {
                text: "Mute"
                font.family: Typography.family
                font.pixelSize: Typography.body
                font.weight: Typography.medium
                color: Colors.textPrimary
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }
        Column {
            visible: root.isVideoCall
            spacing: AppLayouts.s_padding
            CircleButton {
                id: btnCamera
                Layout.preferredHeight: 40
                Layout.preferredWidth: 40
                anchors.horizontalCenter: parent.horizontalCenter
                iconSource: root._isEnableCamera ? AppAssets.icVideo : AppAssets.icVideoSlash
                color: Colors.primary700
                borderWidth: 0
                onClicked: {
                    root._isEnableCamera = !root._isEnableCamera
                    root.cameraChanged(root._isEnableCamera)
                }

            }
            Text {
                text: "Camera"
                font.family: Typography.family
                font.pixelSize: Typography.body
                font.weight: Typography.medium
                color: Colors.textPrimary
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }
    }

}
