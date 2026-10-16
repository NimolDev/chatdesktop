import QtQuick
import QtTest
import Theme
import "subject" as Subject

TestCase {
    name: "FileUploadProgress"
    when: windowShown
    width: 600
    height: 400
    visible: true

    QtObject {
        id: message
        property string id: "file-test"
        property var contents: []
        property real downloadProgress: 0
        property bool isDownloading: false
        property url downloadedFile: ""
        property string downloadError: ""
        property bool isMine: true
        property bool seen: false
        property string body: ""
        property string sentAt: ""
        property int msgType: 5
        property url filePath: "file:///example.MP4"
        property real fileSize: 1024
        property real uploadProgress: 0
        property bool isUploading: false
    }

    QtObject {
        id: controller
        property string requested: ""
        property string canceled: ""
        property int requestedIndex: -1
        property int canceledIndex: -1
        property string canceledUpload: ""
        property int canceledUploadIndex: -1
        function cancelFileUpload(id, index) { canceledUpload = id; canceledUploadIndex = index }
        function downloadFile(id, index) { requested = id; requestedIndex = index }
        function cancelFileDownload(id, index) { canceled = id; canceledIndex = index }
    }

    Component {
        id: factory
        Subject.MessageBubble { width: 400; viewmodel: message; messagingViewModel: controller }
    }

    function test_incomingDownload() {
        message.isMine = false
        message.filePath = "https://example.com/report.pdf"
        const view = createTemporaryObject(factory, this)
        verify(view)
        const button = findChild(view, "fileTransferButton")
        verify(button)
        compare(button.iconSource, AppAssets.icArrowDown)
        button.clicked()
        compare(controller.requested, "file-test")
        message.isDownloading = true
        compare(button.progress, 0)
        compare(button.indeterminate, false)
        compare(button.arcProgress, 0)
        message.downloadProgress = 0.4
        compare(button.downloading, true)
        compare(button.progress, 0.4)
        compare(button.arcProgress, 0.4)
        const activeAngle = button.rotationAngle
        wait(100)
        verify(button.rotationAngle !== activeAngle)
        compare(button.arcProgress, 0.4)
        compare(button.iconSource, AppAssets.icClose)
        button.clicked()
        compare(controller.canceled, "file-test")
        message.isDownloading = false
        message.downloadProgress = 0
        compare(button.downloading, false)
        const stoppedAngle = button.rotationAngle
        wait(100)
        compare(button.rotationAngle, stoppedAngle)
        message.downloadedFile = "file:///cached/report.pdf"
        compare(button.iconSource, AppAssets.icDocument)
        message.downloadedFile = ""
        message.isMine = true
        message.filePath = "file:///example.MP4"
    }

    function test_multipleFiles_data() {
        return [{tag: "incoming", outgoing: false}, {tag: "outgoing", outgoing: true}]
    }

    function test_multipleFiles(data) {
        message.isMine = data.outgoing
        message.contents = [
            {filePath: "https://example.com/1", file_name: "report.pdf", fileSize: 2048, contentIndex: 0},
            {filePath: "https://example.com/2", file_name: "budget.xlsx", fileSize: 4096, contentIndex: 1,
             isUploading: data.outgoing, uploadProgress: 0.6}
        ]
        const view = createTemporaryObject(factory, this)
        verify(view)
        const first = findChild(view, "fileContentBubble0")
        const second = findChild(view, "fileContentBubble1")
        verify(first)
        verify(second)
        compare(first.fileName, "report.pdf")
        compare(second.fileName, "budget.xlsx")
        compare(second.fileSize, 4096)
        tryVerify(function() { return second.y >= first.height && view.height >= first.height + second.height })
        compare(first.isUploading, false)
        compare(second.isUploading, data.outgoing)
        if (data.outgoing) {
            findChild(second, "fileTransferButton").clicked()
            compare(controller.canceledUpload, "file-test")
            compare(controller.canceledUploadIndex, 1)
        } else {
            findChild(second, "fileTransferButton").clicked()
            compare(controller.requested, "file-test")
            compare(controller.requestedIndex, 1)
        }
        message.contents = []
        message.isMine = true
    }

    function test_progressPreservesBubbles() {
        const files = [
            {filePath: "file:///one.pdf", isUploading: true, uploadProgress: 0},
            {filePath: "file:///two.pdf", isUploading: true, uploadProgress: 0}
        ]
        message.contents = files
        const view = createTemporaryObject(factory, this)
        verify(view)
        const first = findChild(view, "fileContentBubble0")
        const second = findChild(view, "fileContentBubble1")
        const button = findChild(second, "fileTransferButton")
        compare(button.indeterminate, true)
        verify(button.arcProgress > 0)
        const initialAngle = button.rotationAngle
        wait(100)
        verify(button.rotationAngle !== initialAngle)
        const height = view.height
        for (let progress of [0.1, 0.4, 0.75, 1]) {
            message.contents = [files[0], {filePath: "file:///two.pdf", isUploading: true, uploadProgress: progress}]
            compare(findChild(view, "fileContentBubble0"), first)
            compare(findChild(view, "fileContentBubble1"), second)
            compare(findChild(second, "fileTransferButton"), button)
            compare(button.progress, progress)
            compare(button.indeterminate, false)
            compare(button.arcProgress, progress)
            compare(view.height, height)
        }
        message.contents = [files[0], {filePath: "file:///two.pdf", isUploading: false, uploadProgress: 1}]
        compare(button.downloading, false)
        compare(button.iconSource, AppAssets.icDocument)
        compare(first.isUploading, true)
        message.contents = []
    }

    function test_liveProgress() {
        const view = createTemporaryObject(factory, this)
        verify(view)
        const button = findChild(view, "fileTransferButton")
        verify(button)
        compare(button.downloading, false)
        message.isUploading = true
        message.uploadProgress = 0.35
        compare(button.downloading, true)
        compare(button.progress, 0.35)
        compare(button.strokeWidth, 3)
        button.clicked()
        message.uploadProgress = 0.8
        compare(button.progress, 0.8)
        message.uploadProgress = 1
        message.isUploading = false
        compare(button.downloading, false)
        compare(button.progress, 0)
        compare(button.strokeWidth, 0)
    }
}
