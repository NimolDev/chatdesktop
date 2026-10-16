import QtQuick
import QtTest
import "subject" as Subject

TestCase {
    id: testCase
    name: "ComposerVoiceSend"
    when: windowShown
    width: 640
    height: 200
    visible: true

    QtObject {
        id: vm
        property bool isVoiceRecordingBusy: false
        property bool isSendingVoice: false
        property url voiceRecordingFile: ""
        property string voiceRecordingDurationText: "00:03:0"
        property string voiceRecordingError: ""
        property int sends: 0
        signal voiceMessageSent()
        signal voiceRecordingFailed(string message)
        signal voiceRecordingCanceled()
        function sendVoiceRecording() { ++sends; isSendingVoice = true }
        function cancelVoiceRecording() { voiceRecordingFile = "" }
    }
    Subject.MessageComposer {
        id: composer
        width: 600
        height: implicitHeight
        viewmodel: vm
    }

    function test_sendRetainsPreviewUntilSuccess() {
        vm.voiceRecordingFile = Qt.resolvedUrl("audio.wav")
        composer._isRecoding = true
        composer._isPreview = true
        const send = findChild(composer, "voiceCloseButton")
        mouseClick(send)
        compare(vm.sends, 1)
        verify(!send.enabled)
        verify(composer._isPreview)
        verify(composer._isRecoding)
        vm.isSendingVoice = false
        vm.voiceRecordingError = "Upload failed"
        vm.voiceRecordingFailed(vm.voiceRecordingError)
        verify(send.enabled)
        verify(composer._isPreview)
        mouseClick(send)
        compare(vm.sends, 2)
        vm.voiceRecordingFile = ""
        vm.voiceRecordingError = ""
        vm.isSendingVoice = false
        vm.voiceMessageSent()
        verify(!composer._isRecoding)
        verify(!composer._isPreview)
    }
}
