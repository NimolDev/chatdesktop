import QtQuick
import QtTest
import Features.Chat
import "subject" as Subject

TestCase {
    name: "ImageViewerZoom"
    when: windowShown

    MessagingViewModel { id: testModel }
    Component { id: modelFactory; MessagingViewModel {} }
    Component { id: factory; Subject.ImageViewWindow { messagingVM: testModel } }

    function test_commandScrollAndPan() {
        const viewer = createTemporaryObject(factory, null, {width: 600, height: 400, visible: true})
        viewer.imageSource = Qt.resolvedUrl("first.svg")
        const image = findChild(viewer.contentItem, "viewerImage")
        const area = findChild(viewer.contentItem, "imageViewerDragArea")
        tryCompare(image, "status", Image.Ready)
        mouseWheel(area, 300, 150, 0, 120)
        compare(viewer.zoom, 1)
        mouseWheel(area, 300, 150, 0, 120, Qt.NoButton, Qt.ControlModifier)
        verify(viewer.zoom > 1)
        viewer.setZoom(3)
        verify(viewer.canPan)
        const windowX = viewer.x
        const windowY = viewer.y
        mouseDrag(area, 300, 150, 60, 40, Qt.LeftButton)
        verify(viewer.panX > 0)
        verify(viewer.panY > 0)
        compare(viewer.x, windowX)
        compare(viewer.y, windowY)
        viewer.panTo(100000, -100000)
        compare(viewer.panX, viewer.maximumPanX)
        compare(viewer.panY, -viewer.maximumPanY)
        findChild(viewer.contentItem, "fitImageButton").clicked()
        compare(viewer.panX, 0)
        compare(viewer.panY, 0)
        compare(viewer.canPan, false)
        viewer.close()
    }

    function test_pointerAnchoredZoomAndMiddleReset() {
        const viewer = createTemporaryObject(factory, null, {width: 600, height: 400, visible: true})
        viewer.imageSource = Qt.resolvedUrl("first.svg")
        const image = findChild(viewer.contentItem, "viewerImage")
        const area = findChild(viewer.contentItem, "imageViewerDragArea")
        tryCompare(image, "status", Image.Ready)
        viewer.setZoom(3)
        const imageX = (360 - viewer.width / 2 - viewer.panX) / viewer.zoom
        const imageY = (240 - viewer.height / 2 - viewer.panY) / viewer.zoom
        mouseWheel(area, 360, 240, 0, 60, Qt.NoButton, Qt.ControlModifier)
        compare(viewer.zoom, 3)
        mouseWheel(area, 360, 240, 0, 60, Qt.NoButton, Qt.ControlModifier)
        fuzzyCompare(viewer.zoom, 3.6, 0.001)
        fuzzyCompare((360 - viewer.width / 2 - viewer.panX) / viewer.zoom, imageX, 0.001)
        fuzzyCompare((240 - viewer.height / 2 - viewer.panY) / viewer.zoom, imageY, 0.001)
        mouseClick(area, 300, 150, Qt.MiddleButton)
        compare(viewer.zoom, 1)
        compare(viewer.panX, 0)
        compare(viewer.panY, 0)
        viewer.close()
    }

    function test_zoomControlsAndReset() {
        const viewer = createTemporaryObject(factory, null)
        verify(viewer)
        compare(viewer.zoom, 1)
        findChild(viewer.contentItem, "zoomInButton").clicked()
        verify(viewer.zoom > 1)
        findChild(viewer.contentItem, "zoomOutButton").clicked()
        fuzzyCompare(viewer.zoom, 1, 0.001)
        viewer.setZoom(100)
        compare(viewer.zoom, viewer.maximumZoom)
        compare(findChild(viewer.contentItem, "zoomInButton").enabled, false)
        viewer.setZoom(0)
        compare(viewer.zoom, viewer.minimumZoom)
        compare(findChild(viewer.contentItem, "zoomOutButton").enabled, false)
        findChild(viewer.contentItem, "fitImageButton").clicked()
        compare(viewer.zoom, 1)
        viewer.setZoom(3)
        viewer.imageSource = Qt.resolvedUrl("first.svg")
        compare(viewer.zoom, 1)
        compare(findChild(viewer.contentItem, "imageViewerDragArea").acceptedButtons, Qt.LeftButton | Qt.MiddleButton)
    }
}
