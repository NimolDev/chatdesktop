import QtQuick
import QtTest
import "subject" as Subject

TestCase {
    id: testCase
    name: "VoiceProgressBindings"
    when: windowShown
    width: 600
    height: 160
    visible: true

    QtObject {
        id: player
        property string voicePlaybackMessageId: ""
        property url voicePlaybackSource: ""
        property bool voicePlaybackPlaying: false
        property bool voicePlaybackSeekable: true
        property real voicePlaybackPosition: 0
        property real voicePlaybackDuration: 10000
        property string voicePlaybackDurationText: "00:00"
        property bool voiceDownloading: false
        property real voiceDownloadProgress: 0
        function playVoice(source, messageId) {
            voicePlaybackSource = source
            voicePlaybackMessageId = messageId
            voicePlaybackPlaying = !voicePlaybackPlaying
        }
        function seekVoice(value) { voicePlaybackPosition = value }
    }
    QtObject {
        id: vm
        readonly property var voiceController: player
    }
    Component {
        id: factory
        Subject.VoiceMessageView {
            width: 600
            messagingViewModel: vm
            messageId: "voice-1"
            fileLocation: "file:///tmp/recording%20one.m4a"
            duration: "00:10"
        }
    }
    function test_progressSurvivesSourceChange_data() {
        return [{tag: "incoming", outgoing: false}, {tag: "outgoing", outgoing: true}]
    }
    function test_progressSurvivesSourceChange(data) {
        player.voicePlaybackMessageId = ""
        player.voicePlaybackPlaying = false
        player.voicePlaybackPosition = 0
        const message = createTemporaryObject(factory, testCase, {isOutgoing: data.outgoing})
        const button = findChild(message, data.outgoing ? "outgoingAction" : "action")
        const slider = findChild(message, data.outgoing ? "outgoingSlider" : "slider")
        mouseClick(button, button.width / 2, button.height / 2)
        verify(message.playing)
        player.voicePlaybackPosition = 2500
        compare(slider.value, 2500)
        message.fileLocation = "file:///tmp/encrypted-cache"
        player.voicePlaybackPosition = 5000
        verify(message.activePlayback)
        compare(slider.value, 5000)
        compare(slider.to, 10000)
        player.voicePlaybackPlaying = false
        compare(slider.value, 5000)
        player.voicePlaybackMessageId = "different-message"
        verify(!message.activePlayback)
        compare(slider.value, 0)
    }
}
