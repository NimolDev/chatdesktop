import QtQuick
import QtTest
import "subject" as Subject

TestCase {
    id: testCase
    name: "ComposerAttachmentHover"
    when: windowShown
    width: 640
    height: 200
    visible: true

    QtObject {
        id: vm
        property bool isVoiceRecordingBusy: false
        property url voiceRecordingFile: ""
        property string voiceRecordingDurationText: "00:00:0"
        function cancelVoiceRecording() { voiceRecordingFile = "" }
    }
    Subject.MessageComposer {
        id: composer
        width: 600
        height: implicitHeight
        viewmodel: vm
    }

    function test_deleteDoesNotOpenAttachment() {
        const attachment = findChild(composer, "attachmentButton")
        const popup = findChild(composer, "attachmentPopup")
        composer._isRecoding = true
        composer._isPreview = true
        const remove = findChild(composer, "btnDeleteVoice")
        mouseMove(remove)
        mouseClick(remove)
        wait(100)
        verify(attachment.hovered)
        verify(!popup.visible)
        // Moving away rearms normal hover interaction.
        mouseMove(composer, 300, 25)
        mouseMove(attachment)
        tryCompare(popup, "visible", true)
        popup.close()
        mouseClick(attachment)
        tryCompare(popup, "visible", true)
        popup.close()
    }
}
