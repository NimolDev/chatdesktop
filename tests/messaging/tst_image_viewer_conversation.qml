import QtQuick
import QtTest
import Features.Chat
import "subject" as Subject

TestCase {
    name: "ImageViewerConversation"
    when: windowShown
    MessagingViewModel { id: testModel }
    Component { id: modelFactory; MessagingViewModel {} }
    Component { id: factory; Subject.ImageViewWindow { messagingVM: testModel } }

    // Match the coordinator's sequence for both new and reused windows.
    function openImage(viewer, source) {
        viewer.reset()
        viewer.imageSource = source
        viewer.getImage()
        viewer.show()
    }

    function test_pageModelDestroyed() {
        const model = createTemporaryObject(modelFactory, null)
        const viewer = createTemporaryObject(factory, null, {messagingVM: model})
        openImage(viewer, Qt.resolvedUrl("first.svg"))
        verify(viewer.visible)
        model.destroy()
        tryCompare(viewer, "messagingVM", null)
        tryCompare(viewer, "visible", false)
        compare(viewer.imageCount, 0)
    }

    function test_reuseWithAnotherPageModel() {
        const first = createTemporaryObject(modelFactory, null)
        const second = createTemporaryObject(modelFactory, null)
        const source = Qt.resolvedUrl("first.svg").toString()
        first.conversationImages = [source]
        second.conversationImages = [source, Qt.resolvedUrl("removed.svg").toString()]
        const viewer = createTemporaryObject(factory, null, {messagingVM: first})
        openImage(viewer, source)
        compare(viewer.imageCount, 1)
        viewer.messagingVM = second
        openImage(viewer, source)
        compare(viewer.imageCount, 2)
        first.imageUrls = []
        compare(viewer.imageCount, 2)
    }

    function test_switchConversationWhileVisible() {
        const viewer = createTemporaryObject(factory, null)
        const first = Qt.resolvedUrl("first.svg").toString()
        const second = Qt.resolvedUrl("removed.svg").toString()
        testModel.conversationImages = [first, second]
        openImage(viewer, second)
        compare(viewer.index, 1)
        compare(viewer.imageCount, 2)

        testModel.conversationImages = [second]
        openImage(viewer, second)
        compare(viewer.visible, true)
        compare(viewer.imageCount, 1)
        compare(viewer.index, 0)
        compare(viewer.imageList[0], second)

        viewer.close()
        testModel.conversationImages = [first, second]
        openImage(viewer, first)
        compare(viewer.index, 0)
        viewer.imageSource = second
        compare(viewer.index, 1)
        viewer.imageSource = ""
        compare(viewer.index, -1)
        testModel.imageUrls = []
        compare(viewer.imageCount, 0)
        compare(viewer.index, -1)
        viewer.close()
    }
}
