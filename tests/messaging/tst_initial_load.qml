import QtQuick
import QtTest
import "subject" as Subject

TestCase {
    id: testCase
    name: "InitialMessageLoad"
    when: windowShown
    width: 600
    height: 400
    visible: true

    ListModel {
        id: messages
        property bool isLoading: true
        signal initialMessagesLoaded()
        signal olderMessagesLoaded(int insertedCount)
        function fetchNextMessage() { olderMessagesLoaded(0) }
    }
    Component {
        id: factory
        Subject.MessagingBody { width: 600; height: 400; model: messages; isVisible: true }
    }
    function init() {
        messages.isLoading = true
        messages.clear()
    }
    function test_initialReveal_data() {
        return [{tag: "empty", count: 0}, {tag: "short", count: 2}, {tag: "long", count: 60}]
    }
    function test_initialReveal(data) {
        const body = createTemporaryObject(factory, testCase)
        const list = findChild(body, "messageList")
        wait(40)
        compare(list.opacity, 0)
        for (let i = 0; i < data.count; ++i) {
            messages.append({body: "Message " + i + "\n".repeat(i % 4), sentAt: "", isMine: true,
                             msgType: 0, filePath: "", duration: 0, seen: true, section: "Today"})
        }
        compare(list.opacity, 0)
        messages.isLoading = false
        messages.initialMessagesLoaded()
        tryVerify(() => body.initialPositionReady)
        verify(list.opacity < 1, "Messages should fade in rather than appear in one frame")
        tryCompare(list, "opacity", 1)
        verify(Math.abs(list.contentY - list.maximumContentY) < 2)
        if (data.count > 0) {
            const last = list.itemAtIndex(data.count - 1)
            verify(last)
            verify(Math.abs(body.height - last.mapToItem(body, 0, last.height).y - 16) < 3)
        }
    }
}
