import QtQuick
import QtTest
import "subject" as Subject

TestCase {
    name: "SingleImageMessage"
    when: windowShown
    width: 600
    height: 600
    visible: true

    Component {
        id: factory
        Subject.ImageMessageView { width: 400 }
    }

    function test_grid_data() {
        return [1, 2, 3, 4, 5, 7].map(n => ({tag: String(n), count: n}))
    }

    function test_grid(data) {
        const contents = []
        for (let i = 0; i < data.count; ++i) {
            contents.push({msgType: 4, filePath: "file:///missing-image-" + i + ".png"})
        }
        const view = createTemporaryObject(factory, this, {viewmodel: {contents: contents}})
        verify(view)
        compare(String(view.imageSource), contents[0].filePath)
        const tile = findChild(view, "imageTile")
        verify(tile)
        compare(findChild(view, "imageGrid"), null)
        compare(findChild(view, "imageRepeater"), null)
        tryVerify(function() { return tile.width > 0 && tile.height > 0 })
    }

    function test_aspectRatio_data() {
        return [
            {tag: "landscape", source: "first.svg", ratio: 1.25, width: 400},
            {tag: "portrait", source: "removed.svg", ratio: 0.2, width: 400},
            {tag: "narrow-portrait", source: "removed.svg", ratio: 0.2, width: 70},
            {tag: "narrow-landscape", source: "first.svg", ratio: 1.25, width: 200}
        ]
    }

    function test_aspectRatio(data) {
        const view = createTemporaryObject(factory, this, {
            width: data.width, viewmodel: {id: data.tag, filePath: Qt.resolvedUrl(data.source)}
        })
        tryCompare(findChild(view, "messagePhoto"), "status", Image.Ready)
        const tile = findChild(view, "imageTile")
        tryCompare(view.bubbleItem, "width", Math.min(150, data.width))
        tryCompare(tile, "height", Math.min(view.maxImageHeight, tile.width))
        const photo = findChild(view, "messagePhoto")
        verify(Math.abs(photo.paintedWidth / photo.paintedHeight - data.ratio) < 0.01)
        verify(view.bubbleItem.width <= view.width)
    }

    function test_rememberedDimensionsBeforeLoading() {
        const view = createTemporaryObject(factory, this, {viewmodel: {
            id: "cached", filePath: "file:///missing.png", imageAspectRatio: 0.2
        }})
        const tile = findChild(view, "imageTile")
        tryCompare(tile, "height", Math.min(view.maxImageHeight, tile.width))
    }

    function test_sourceReplacementKeepsGeometry() {
        const view = createTemporaryObject(factory, this, {viewmodel: {
            id: "same", filePath: Qt.resolvedUrl("removed.svg")
        }})
        tryCompare(findChild(view, "messagePhoto"), "status", Image.Ready)
        const tile = findChild(view, "imageTile")
        const height = tile.height
        const width = view.bubbleItem.width
        view.viewmodel = {id: "same", filePath: Qt.resolvedUrl("first.svg")}
        tryCompare(findChild(view, "messagePhoto"), "status", Image.Ready)
        compare(tile.height, height)
        compare(view.bubbleItem.width, width)
        view.viewmodel = {id: "same", filePath: "file:///missing-replacement.png"}
        compare(tile.height, height)
        compare(view.bubbleItem.width, width)
        view.viewmodel = {id: "different", filePath: ""}
        compare(String(findChild(view, "messagePhoto").source), "")
    }

    function test_legacySingleImage() {
        const view = createTemporaryObject(factory, this, {viewmodel: {filePath: "file:///single.png"}})
        compare(String(view.imageSource), "file:///single.png")
    }
}
