import QtQuick
import QtTest
import "subject" as Subject

TestCase {
    id: testCase
    name: "VoiceMessagePlayback"
    when: windowShown
    width: 600
    height: 160
    visible: true
    property var message
    QtObject {
        id: playbackViewModel
        readonly property var voiceController: testMessaging
    }



    Component {
        id: factory
        Subject.VoiceMessageView {
            messagingViewModel: playbackViewModel
            width: 600
            height: implicitHeight
            fileLocation: Qt.resolvedUrl("audio.wav")
        }
    }
    function test_playback_data() {
        return [{tag: "incoming", outgoing: false}, {tag: "outgoing", outgoing: true}]
    }
    function test_playback(data) {
        message = createTemporaryObject(factory, testCase, {isOutgoing: data.outgoing})
        verify(message)
        const player = testMessaging
        const button = findChild(message, data.outgoing ? "outgoingAction" : "action")
        const slider = findChild(message, data.outgoing ? "outgoingSlider" : "slider")
        mouseClick(button, button.width / 2, button.height / 2)
        tryCompare(player, "voicePlaybackPlaying", true)
        tryVerify(() => slider.value > 200, 5000)
        compare(slider.to, player.voicePlaybackDuration)
        mouseClick(button, button.width / 2, button.height / 2)
        tryCompare(player, "voicePlaybackPlaying", false)
        fuzzyCompare(slider.value, player.voicePlaybackPosition, 1)
        mouseClick(slider, slider.width * 0.6, slider.height / 2)
        verify(player.voicePlaybackPosition > 1000)
        fuzzyCompare(slider.value, player.voicePlaybackPosition, 1)
        mouseClick(button, button.width / 2, button.height / 2)
        const seekPosition = player.voicePlaybackPosition
        tryVerify(() => slider.value > seekPosition + 100, 5000)
        tryCompare(player, "voicePlaybackPlaying", false, 6000)
        tryCompare(player, "voicePlaybackPosition", 0)
        tryCompare(slider, "value", 0)
        mouseClick(button, button.width / 2, button.height / 2)
        tryCompare(player, "voicePlaybackPlaying", true)
        verify(player.voicePlaybackPosition < player.voicePlaybackDuration / 2)
        player.stopVoicePlayback()
    }
}
