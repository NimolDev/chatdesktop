import QtQuick
import QtTest
import "subject" as Subject

TestCase {
    id: testCase
    name: "SmoothMessageAppend"
    when: windowShown
    width: 600
    height: 400
    visible: true

    ListModel {
        id: messages
        property bool isLoading: true
        property var voiceController: testMessaging
        signal initialMessagesLoaded()
        signal olderMessagesLoaded(int insertedCount)
        function fetchNextMessage() { olderMessagesLoaded(0) }
    }

    Component {
        id: factory
        Subject.MessagingBody { width: 600; height: 400; model: messages; isVisible: true }
    }

    function appendMessage() {
        messages.append({body: "", sentAt: "", isMine: true, msgType: 1,
                         filePath: "", duration: 3000, seen: true, section: "Today"})
    }

    function init() {
        messages.isLoading = true
        messages.clear()
    }

    function test_appendScrollsGraduallyAndHandlesBurst() {
        for (let i = 0; i < 20; ++i) {
            appendMessage()
        }
        const body = createTemporaryObject(factory, testCase)
        verify(body)
        const list = findChild(body, "messageList")
        verify(list)
        messages.initialMessagesLoaded()
        tryVerify(() => list.animateAppends)
        messages.isLoading = false
        const previousY = list.contentY
        appendMessage()
        body.scrollToLatest()
        wait(60)
        verify(list.contentY > previousY)
        verify(list.contentY < list.maximumContentY)
        appendMessage()
        appendMessage()
        body.scrollToLatest()
        tryVerify(() => Math.abs(list.contentY - list.maximumContentY) < 1)
    }

    function test_modelAppendScrollsWithoutExplicitRequest() {
        for (let i = 0; i < 20; ++i) {
            appendMessage()
        }
        const body = createTemporaryObject(factory, testCase)
        const list = findChild(body, "messageList")
        messages.initialMessagesLoaded()
        tryVerify(() => list.animateAppends)
        messages.isLoading = false
        appendMessage()
        appendMessage()
        tryVerify(() => list.count === messages.count)
        tryVerify(() => Math.abs(list.contentY - list.maximumContentY) < 1)
    }

    function test_delayedMessageHeightKeepsLatestVisible() {
        for (let i = 0; i < 20; ++i) {
            messages.append({body: "Message", sentAt: "", isMine: true, msgType: 0,
                             filePath: "", duration: 0, seen: true, section: "Today"})
        }
        const body = createTemporaryObject(factory, testCase)
        const list = findChild(body, "messageList")
        messages.initialMessagesLoaded()
        tryVerify(() => list.animateAppends)
        messages.isLoading = false
        wait(300)
        messages.setProperty(19, "body", "Loaded content\n".repeat(12))
        wait(100)
        tryVerify(() => Math.abs(list.contentY - list.maximumContentY) < 1)
        body.followLatest = false
        list.contentY = list.maximumContentY - 150
        const readingPosition = list.contentY
        messages.setProperty(19, "body", "Loaded content\n".repeat(16))
        wait(100)
        verify(Math.abs(list.contentY - readingPosition) < 1)
    }
    function test_scrollToLatestAlignsLastMessage_data() {
        return [{tag: "animated", animated: true},
                {tag: "immediate", animated: false}]
    }

    function test_scrollToLatestAlignsLastMessage(data) {
        for (let i = 0; i < 100; ++i) {
            messages.append({body: "Message " + i + "\n".repeat(i < 50 ? 8 : 1),
                             sentAt: "", isMine: true, msgType: 0,
                             filePath: "", duration: 0, seen: true, section: "Today"})
        }
        const body = createTemporaryObject(factory, testCase)
        const list = findChild(body, "messageList")
        messages.initialMessagesLoaded()
        tryVerify(() => list.animateAppends)
        messages.isLoading = false
        list.positionViewAtBeginning()
        wait(300)
        body.scrollToLatest(data.animated)
        if (data.animated) {
            wait(190)
            const beforeFinish = list.contentY
            wait(150)
            // OutCubic has covered almost the entire final viewport by now.
            // A correction to an estimated destination would visibly snap.
            verify(Math.abs(list.contentY - beforeFinish) < body.height * 0.15,
                   "Scroll jumped at animation completion")
        }
        wait(300)
        const last = list.itemAtIndex(messages.count - 1)
        verify(last)
        const bottom = last.mapToItem(body, 0, last.height).y
        verify(Math.abs(body.height - bottom - 16) < 3,
               "Unexpected bottom gap: " + (body.height - bottom))
    }

    function test_playbackBarKeepsMessageScreenPosition() {
        for (let i = 0; i < 30; ++i) {
            appendMessage()
        }
        const body = createTemporaryObject(factory, testCase)
        const list = findChild(body, "messageList")
        messages.initialMessagesLoaded()
        tryVerify(() => list.animateAppends)
        messages.isLoading = false
        wait(260)
        const last = list.itemAtIndex(messages.count - 1)
        verify(last)
        const screenY = last.mapToItem(testCase, 0, 0).y
        // Opening the player moves the top edge while keeping the composer fixed.
        body.y = 30
        body.height = 370
        tryVerify(() => !body.resizing)
        fuzzyCompare(last.mapToItem(testCase, 0, 0).y, screenY, 1)
        body.y = 0
        body.height = 400
        tryVerify(() => !body.resizing)
        fuzzyCompare(last.mapToItem(testCase, 0, 0).y, screenY, 1)
    }

    function test_resizeHeight_data() {
        return [{tag: "long", count: 20}, {tag: "short", count: 2}]
    }

    function test_resizeHeight(data) {
        for (let i = 0; i < data.count; ++i) {
            appendMessage()
        }
        const body = createTemporaryObject(factory, testCase)
        const list = findChild(body, "messageList")
        messages.initialMessagesLoaded()
        tryVerify(() => list.animateAppends)
        messages.isLoading = false
        wait(260)
        for (const newHeight of [250, 500, 300, 400]) {
            body.height = newHeight
            tryVerify(() => !body.resizing)
            verify(Math.abs(list.contentY - list.maximumContentY) < 1, "height=" + newHeight + " y=" + list.contentY + " max=" + list.maximumContentY)
            verify(Math.abs(list.y + list.height - body.height) < 1)
        }
    }

    function test_resizePreservesDistanceFromBottomWhileReading() {
        for (let i = 0; i < 30; ++i) {
            appendMessage()
        }
        const body = createTemporaryObject(factory, testCase)
        const list = findChild(body, "messageList")
        messages.initialMessagesLoaded()
        tryVerify(() => list.animateAppends)
        messages.isLoading = false
        list.contentY = list.maximumContentY - 250
        const distance = list.maximumContentY - list.contentY
        body.height = 320
        body.height = 280
        body.height = 250
        tryVerify(() => !body.resizing)
        verify(Math.abs(list.maximumContentY - list.contentY - distance) < 1)
        body.height = 400
        tryVerify(() => !body.resizing)
        verify(Math.abs(list.maximumContentY - list.contentY - distance) < 1)
    }

    function test_removeDissolvesAndClosesGap_data() {
        return [{tag: "scrolling", count: 20}, {tag: "short", count: 5}]
    }

    function test_removeDissolvesAndClosesGap(data) {
        for (let i = 0; i < data.count; ++i) {
            appendMessage()
        }
        const body = createTemporaryObject(factory, testCase)
        verify(body)
        const list = findChild(body, "messageList")
        messages.initialMessagesLoaded()
        tryVerify(() => list.animateAppends)
        messages.isLoading = false
        wait(260)
        const removed = list.itemAtIndex(data.count - 3)
        const following = list.itemAtIndex(data.count - 2)
        verify(removed)
        verify(following)
        const previousY = following.y
        const targetY = removed.y
        messages.remove(data.count - 3)
        wait(60)
        verify(removed.removalProgress > 0 && removed.removalProgress < 1)
        compare(removed.y, targetY)
        compare(removed.scale, 1)
        wait(350)
        compare(following.y, previousY)
        verify(removed.removalProgress < 1)
        tryVerify(() => following.y < previousY)
        tryVerify(() => Math.abs(following.y - targetY) < 1)
        tryCompare(list, "count", data.count - 1)
    }

}
