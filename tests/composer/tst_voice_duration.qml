import QtQuick
import QtTest
import "subject" as Subject

TestCase {
    id: testCase
    name: "VoiceSendDuration"
    when: windowShown
    width: 640
    height: 180
    visible: true

    QtObject {
        id: vm
        property bool voicePreviewPlaying: false
        property bool voicePreviewReady: false
        property bool voicePreviewSeekable: false
        property int voicePreviewPosition: 0
        property int voicePreviewDuration: 0
        property string voicePreviewError: ""
        function stopVoicePreview() {}
        property bool isVoiceRecordingBusy: false
        property bool isSendingVoice: false
        property string voiceRecordingError: ""
        property url voiceRecordingFile: ""
        property int voiceRecordingDuration: 2345
        property string voiceRecordingDurationText: "00:02:3"
        property int stops: 0
        signal voiceRecordingCanceled()
        signal voiceMessageSent()
        signal voiceSendFailed(string message)
        property string voiceSendError: ""
        signal voiceRecordingFailed(string message)
        function stopVoiceRecording() { ++stops }
    }
    Subject.MessageComposer {
        id: composer
        width: 600
        height: implicitHeight
        viewmodel: vm
        messagingViewModel: vm
    }
    SignalSpy { id: sends; target: composer; signalName: "sendVoice" }
    SignalSpy { id: heightChanges; target: composer; signalName: "heightDelta" }

    function init() {
        sends.clear()
        vm.stops = 0
        vm.isSendingVoice = false
        vm.voiceRecordingError = ""
        vm.voiceRecordingFile = ""
        composer.resetRecoding()
    }
    function test_uploadStatusDoesNotResizeComposer() {
        composer._isRecoding = true
        composer._isPreview = true
        const initialHeight = composer.height
        const status = findChild(composer, "voicePreviewStatus")
        heightChanges.clear()
        vm.isSendingVoice = true
        compare(status.text, "Uploading audio…")
        compare(composer.height, initialHeight)
        vm.isSendingVoice = false
        vm.voiceRecordingError = "Upload failed. Please retry."
        compare(status.text, vm.voiceRecordingError)
        compare(composer.height, initialHeight)
        vm.voiceRecordingError = ""
        vm.isSendingVoice = true
        compare(composer.height, initialHeight)
        vm.isSendingVoice = false
        vm.voiceMessageSent()
        compare(composer.height, initialHeight)
        compare(heightChanges.count, 0)
    }
    function test_uploadPreventsDuplicateAndAllowsRetry() {
        composer._isRecoding = true
        composer._isPreview = true
        composer.sendRecording()
        verify(!composer._isPreview)
        vm.isSendingVoice = true
        composer.sendRecording()
        compare(sends.count, 1)
        vm.isSendingVoice = false
        vm.voiceRecordingFile = Qt.resolvedUrl("audio.wav")
        vm.voiceRecordingFailed("Upload failed")
        verify(composer._isPreview)
        composer.sendRecording()
        compare(sends.count, 2)
        vm.voiceMessageSent()
        verify(!composer._isRecoding)
        verify(!composer._isPreview)
    }
    function test_sendBeforePreviewMetadataLoads() {
        composer._isRecoding = true
        composer._isPreview = true
        compare(vm.voicePreviewDuration, 0)
        const send = findChild(composer, "voiceCloseButton")
        verify(send.visible)
        mouseClick(send)
        compare(vm.stops, 1)
        compare(sends.count, 1)
        compare(sends.signalArguments[0][0], 2345)
    }
}
