import QtQuick
import QtTest
import "subject" as Subject

TestCase {
    name: "FileViewModel"
    when: windowShown
    width: 600
    height: 400
    visible: true

    QtObject {
        id: controller
        property string requested: ""
        property int requestedIndex: -1
        function downloadFile(id, index) { requested = id; requestedIndex = index }
        function cancelFileUpload(id, index) { requested = id; requestedIndex = index }
    }
    Component {
        id: factory
        Subject.MessageBubble { width: 400; messagingViewModel: controller }
    }

    function test_directBindings() {
        const model = {id: "document", msgType: 5, isMine: false, seen: true,
            body: "Caption", sentAt: "09:30", filePath: "", contents: [
                {fileName: "report.pdf", filePath: "https://example.com/report.pdf",
                 fileSize: 2048, contentIndex: 0}
            ]}
        const view = createTemporaryObject(factory, this, {viewmodel: model})
        const button = findChild(view, "fileTransferButton")
        verify(button)
        button.clicked()
        compare(controller.requested, "document")
        compare(controller.requestedIndex, 0)
        const bubble = view.bubbleItem.parent
        compare(bubble.viewmodel, view.viewmodel)
        compare(bubble.fileName, "report.pdf")
        compare(bubble.fileSize, 2048)
        compare(bubble.msg, "Caption")
        compare(bubble.date, "09:30")
        compare(bubble.isOutgoing, false)
        compare(bubble.isSeen, true)
        view.viewmodel = Object.assign({}, model, {contents: [
            Object.assign({}, model.contents[0], {isUploading: true, uploadProgress: 0.65})
        ]})
        compare(findChild(view, "fileTransferButton"), button)
        compare(button.progress, 0.65)
        compare(button.downloading, true)
        view.selectionMode = true
        view.selected = true
        compare(bubble.selectionMode, true)
        compare(bubble.selected, true)
    }

    function test_emptyContentsFallback() {
        const view = createTemporaryObject(factory, this, {viewmodel: {
            id: "legacy", msgType: 5, isMine: true, seen: false, body: "", sentAt: "",
            contents: [], filePath: "file:///legacy.pdf", fileSize: 1024,
            isUploading: true, uploadProgress: 0.4
        }})
        const bubble = view.bubbleItem.parent
        compare(bubble.fileName, "legacy.pdf")
        compare(bubble.fileSize, 1024)
        compare(bubble.uploadProgress, 0.4)
        findChild(view, "fileTransferButton").clicked()
        compare(controller.requested, "legacy")
        compare(controller.requestedIndex, 0)
    }
}
