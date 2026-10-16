import QtQuick
import QtTest
import "subject" as Subject

TestCase {
    id: testCase
    name: "DeleteMedia"
    when: windowShown
    width: 600
    height: 600
    visible: true

    Component {
        id: factory
        Subject.MessagingBody {
            width: 600
            height: 600
            model: messageModel
            isVisible: true
        }
    }

    function test_removeKeepsMediaAndGeometry_data() {
        return [{tag: "short", height: 600, older: 0},
                {tag: "scrolling", height: 400, older: 20}]
    }

    function test_removeKeepsMediaAndGeometry(data) {
        let rows = [
            {id: "a", msgType: 4, body: "first image", filePath: Qt.resolvedUrl("first.svg").toString()},
            {id: "b", msgType: 5, body: "first file", filePath: "file:///first-file.png"},
            {id: "c", msgType: 4, body: "deleted image", filePath: Qt.resolvedUrl("removed.svg").toString()},
            {id: "d", msgType: 5, body: "last file", filePath: "file:///last-file.png"}
        ]
        for (let i = 0; i < data.older; ++i) {
            rows.unshift({id: "older" + i, msgType: 0, body: "Earlier message " + i, filePath: ""})
        }
        messageFixture.loadMessages(rows)
        tryVerify(() => !messageModel.isLoading)
        const body = createTemporaryObject(factory, testCase, {height: data.height})
        const list = findChild(body, "messageList")
        tryVerify(() => list.animateAppends)
        wait(600)
        const removed = list.itemAtIndex(data.older + 2)
        const last = list.itemAtIndex(data.older + 3)
        verify(removed)
        verify(last)
        const removedHeight = removed.height
        messageModel.deleteMessage([data.older + 2])
        wait(180)
        compare(removed.msgType, 4)
        compare(removed.fileLocation.toString(), rows[data.older + 2].filePath)
        compare(removed.height, removedHeight)
        compare(last.msg, rows[data.older + 3].body)
        compare(last.fileLocation.toString(), rows[data.older + 3].filePath)
        // Inspect the closing gap frame by frame, including delegates created
        // above the viewport as the list follows the latest message.
        for (let frame = 0; frame < 40; ++frame) {
            wait(20)
            verifyNoOverlap(list)
        }
        compare(list.count, data.older + 3)
        compare(list.itemAtIndex(data.older + 2).fileLocation.toString(), rows[data.older + 3].filePath)
        verify(Math.abs(last.mapToItem(body, 0, last.height).y - body.height + 16) < 3)
    }

    function verifyNoOverlap(list) {
        for (let i = 0; i < list.count - 1; ++i) {
            const current = list.itemAtIndex(i)
            const next = list.itemAtIndex(i + 1)
            if (current && next) {
                verify(current.y + current.height <= next.y + 1,
                       "Overlapping rows " + i + ": " + (current.y + current.height) + " > " + next.y)
            }
        }
    }
}
