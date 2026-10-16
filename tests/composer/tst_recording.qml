import QtQuick
import QtTest
import "subject" as Subject

TestCase {
    id: testCase
    name: "MessageComposerRecording"
    when: windowShown
    width: 640
    height: 180
    visible: true
    property var composer
    property var mock

    Component {
        id: mockFactory
        QtObject {
            property bool isVoiceRecordingBusy: false
            property bool isVoiceRecording: false
            property bool isVoiceRecordingPaused: false
            property real voiceRecordingDuration: 0
            property url voiceRecordingFile: ""
            property string voiceRecordingError: ""
            property int starts: 0
            property int stops: 0
            property int cancels: 0
            property bool failStart: false
            function startVoiceRecording() {
                ++starts
                if (failStart) {
                    voiceRecordingError = "Microphone denied"
                    return
                }
                isVoiceRecordingBusy = true
                isVoiceRecording = true
            }
            function stopVoiceRecording() {
                ++stops
                isVoiceRecordingBusy = false
                isVoiceRecording = false
            }
            function cancelVoiceRecording() {
                ++cancels
                isVoiceRecordingBusy = false
                isVoiceRecording = false
                isVoiceRecordingPaused = false
                voiceRecordingFile = ""
            }
            function pauseVoiceRecording() {
                isVoiceRecording = false
                isVoiceRecordingPaused = true
            }
            function resumeVoiceRecording() {
                isVoiceRecording = true
                isVoiceRecordingPaused = false
            }
        }
    }
    Component {
        id: composerFactory
        Subject.MessageComposer { width: 600; height: implicitHeight }
    }
    SignalSpy { id: submitted; signalName: "submitted" }
    function init() {
        mock = createTemporaryObject(mockFactory, testCase)
        composer = createTemporaryObject(composerFactory, testCase, {viewmodel: mock})
        submitted.target = composer
        submitted.clear()
        wait(20)
    }
    function test_clickPauseResumeStop() {
        mouseClick(findChild(composer, "messageSendButton"))
        compare(mock.starts, 1)
        verify(composer.recordingBusy)
        mouseClick(findChild(composer, "voicePauseButton"))
        verify(mock.isVoiceRecordingPaused)
        mouseClick(findChild(composer, "voicePauseButton"))
        verify(mock.isVoiceRecording)
        mouseClick(findChild(composer, "voiceStopButton"))
        compare(mock.stops, 1)
        verify(!composer.recordingBusy)
    }
    function test_holdRelease() {
        const button = findChild(composer, "messageSendButton")
        mousePress(button)
        wait(650)
        compare(mock.starts, 1)
        mouseRelease(button)
        compare(mock.stops, 1)
        compare(mock.cancels, 0)
    }
    function test_slideToCancel() {
        const button = findChild(composer, "messageSendButton")
        mousePress(button)
        wait(650)
        mouseMove(button, -100, 20)
        mouseRelease(button, -100, 20)
        compare(mock.cancels, 1)
        compare(mock.stops, 0)
    }
    function test_longPressWithTextDoesNotRecord() {
        findChild(composer, "messageInput").text = "hello"
        const button = findChild(composer, "messageSendButton")
        mousePress(button)
        wait(650)
        mouseRelease(button)
        compare(mock.starts, 0)
        compare(submitted.count, 1)
        compare(submitted.signalArguments[0][0], "hello")
    }
    function test_permissionFailure() {
        mock.failStart = true
        mouseClick(findChild(composer, "messageSendButton"))
        compare(composer.recordingError, "Microphone denied")
        verify(!composer.recordingBusy)
    }
    function test_resetCancels() {
        mouseClick(findChild(composer, "messageSendButton"))
        composer.resetState()
        compare(mock.cancels, 1)
        verify(!composer.recordingBusy)
    }
}
