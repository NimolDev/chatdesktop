import QtQuick
import QtTest
import "subject" as Subject

TestCase {
    name: "ImageMessageGrid"
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
        compare(view.imageUrls.length, Math.min(data.count, 5))
        wait(50)
        const grid = findChild(view, "imageGrid")
        const tiles = grid.children.filter(item => item.objectName === "imageTile")
        compare(tiles.length, Math.min(data.count, 5))
        for (const tile of tiles) {
            verify(tile.width > 0 && tile.height > 0)
            verify(tile.x >= 0 && tile.x + tile.width <= grid.width + 1)
            verify(tile.y + tile.height <= grid.height + 1)
        }
    }

    function test_legacySingleImage() {
        const view = createTemporaryObject(factory, this, {fileLocation: "file:///single.png"})
        compare(view.imageUrls.length, 1)
    }
}
