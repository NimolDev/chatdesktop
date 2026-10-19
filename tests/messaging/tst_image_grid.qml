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
        const view = createTemporaryObject(factory, this, {contents: contents})
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
            width: data.width, fileLocation: Qt.resolvedUrl(data.source)
        })
        tryVerify(function() { return view.imageAspectRatio > 0 })
        const tile = findChild(view, "imageTile")
        const expectedWidth = data.ratio < 1
            ? Math.min(data.width, view.maxImageHeight * data.ratio + view.imageHorizontalPadding)
            : data.width
        tryVerify(function() { return Math.abs(view.bubbleItem.width - expectedWidth) < 0.1 })
        tryVerify(function() { return Math.abs(tile.width / tile.height - data.ratio) < 0.01 })
        if (data.ratio < 1 && data.width === 400) {
            compare(tile.height, view.maxImageHeight)
        }
        verify(view.bubbleItem.width <= view.width)
    }

    function test_legacySingleImage() {
        const view = createTemporaryObject(factory, this, {fileLocation: "file:///single.png"})
        compare(String(view.imageSource), "file:///single.png")
    }
}
