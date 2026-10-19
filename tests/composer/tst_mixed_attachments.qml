import QtQuick
import QtTest
import "subject" as Subject

TestCase {
    id: testCase
    name: "MixedAttachments"
    when: windowShown
    width: 600
    height: 700
    visible: true

    QtObject {
        id: vm
        property bool isUploadingAttachments: false
        property bool isCancelingAttachmentUpload: false
        property var entries: []
        function resetAttachments() { entries = [] }
        function addAttachments(urls, compress) {
            entries = entries.concat([{url: urls[0], compress: compress}])
        }
    }
    Subject.AttachmentUploadDialog { id: dialog; viewmodel: vm }
    SignalSpy { id: sent; target: dialog; signalName: "send" }

    function cleanup() {
        dialog.close()
        vm.resetAttachments()
        sent.clear()
    }

    function test_mixedPreviewAndRemoval() {
        dialog.files = [
            {url: "file:///document.txt", compress: false},
            {url: "file:///photo.png", compress: true},
            {url: "file:///original.png", compress: false}
        ]
        dialog.open()
        const repeater = findChild(dialog, "attachmentRepeater")
        tryCompare(repeater, "count", 3)
        const remove = findChild(repeater.itemAt(1), "removeImageAttachment")
        verify(remove)
        remove.clicked()
        compare(dialog.files.length, 2)
        compare(dialog.files[0].url, "file:///document.txt")
        compare(dialog.files[1].url, "file:///original.png")
        compare(vm.entries.length, 2)
        compare(vm.entries[0].compress, false)
        compare(vm.entries[1].compress, false)
    }

    function test_selectionOrder() {
        dialog.files = [{url: "file:///first.png", compress: true}]
        dialog.open()
        dialog.files = dialog.files.concat([{url: "file:///middle.txt", compress: false}])
        dialog.files = dialog.files.concat([{url: "file:///last.png", compress: true}])
        const repeater = findChild(dialog, "attachmentRepeater")
        tryCompare(repeater, "count", 3)
        tryVerify(function() { return repeater.itemAt(2).y > repeater.itemAt(1).y })
        verify(repeater.itemAt(1).y > repeater.itemAt(0).y)
        compare(repeater.itemAt(0).fileName, "first.png")
        compare(repeater.itemAt(1).fileName, "middle.txt")
        compare(repeater.itemAt(2).fileName, "last.png")
        compare(repeater.itemAt(0).compressed, true)
        compare(repeater.itemAt(1).compressed, false)
        compare(repeater.itemAt(2).compressed, true)
        findChild(repeater.itemAt(1), "removeFileAttachment").clicked()
        compare(dialog.files.length, 2)
        compare(dialog.files[0].url, "file:///first.png")
        compare(dialog.files[1].url, "file:///last.png")
        compare(vm.entries[0].compress, true)
        compare(vm.entries[1].compress, true)
    }

    function test_removeFilePreservesCompression() {
        dialog.files = [
            {url: "file:///document.txt", compress: false},
            {url: "file:///photo.png", compress: true}
        ]
        dialog.open()
        dialog.removeFile(0)
        compare(vm.entries.length, 1)
        compare(vm.entries[0].compress, true)
        findChild(dialog, "attachmentCaption").text = "Caption"
        findChild(dialog, "attachmentUploadAction").clicked()
        compare(sent.count, 1)
        compare(sent.signalArguments[0][0], "Caption")
    }
}
