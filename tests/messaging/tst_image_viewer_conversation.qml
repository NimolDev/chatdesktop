import QtQuick
import QtTest
import Features.Chat
import "subject" as Subject

TestCase {
    name: "ImageViewerConversation"
    when: windowShown
    Component { id: factory; Subject.ImageViewWindow {} }

    // Match the coordinator's sequence for both new and reused windows.
    function openImage(viewer, source) {
        viewer.reset()
        viewer.imageSource = source
        viewer.getImage()
        viewer.show()
    }

    function test_switchConversationWhileVisible() {
        const viewer = createTemporaryObject(factory, null)
        const first = Qt.resolvedUrl("first.svg").toString()
        const second = Qt.resolvedUrl("removed.svg").toString()
        MessagingViewModel.conversationImages = [first, second]
        openImage(viewer, second)
        compare(viewer.index, 1)
        compare(viewer.imageCount, 2)

        MessagingViewModel.conversationImages = [second]
        openImage(viewer, second)
        compare(viewer.visible, true)
        compare(viewer.imageCount, 1)
        compare(viewer.index, 0)
        compare(viewer.imageList[0], second)

        viewer.close()
        MessagingViewModel.conversationImages = [first, second]
        openImage(viewer, first)
        compare(viewer.index, 0)
        viewer.imageSource = second
        compare(viewer.index, 1)
        viewer.imageSource = ""
        compare(viewer.index, -1)
        MessagingViewModel.imageUrls = []
        compare(viewer.imageCount, 0)
        compare(viewer.index, -1)
        viewer.close()
    }
}
