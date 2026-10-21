import QtQuick
import QtTest
import "subject" as Subject

TestCase {
    id: testCase
    name: "MessageDeletionScroll"
    when: windowShown
    width: 600
    height: 600
    visible: true

    ListModel {
        id: messages
        property bool isLoading: true
        property int fetchCount: 0
        signal initialMessagesLoaded()
        signal olderMessagesLoaded(int insertedCount)
        function fetchNextMessage() {
            ++fetchCount
            olderMessagesLoaded(0)
        }
    }

    Component {
        id: factory
        Subject.MessagingBody {
            width: 600
            height: 600
            model: messages
            isVisible: true
        }
    }

    function test_deleteTallMessageKeepsLatestWithoutFetchingHistory_data() {
        return [{tag: "middle", first: 6, count: 1},
                {tag: "last", first: 7, count: 1},
                {tag: "multiple", first: 5, count: 3}]
    }

    function test_deleteTallMessageKeepsLatestWithoutFetchingHistory(data) {
        messages.isLoading = true
        messages.clear()
        for (let i = 0; i < 8; ++i) {
            messages.append({body: "Message " + i, sentAt: "", isMine: true,
                             msgType: 0, filePath: "", seen: true, section: "Today"})
        }
        messages.setProperty(data.first, "body", "Tall image equivalent\n".repeat(18))
        const body = createTemporaryObject(factory, testCase)
        const list = findChild(body, "messageList")
        messages.initialMessagesLoaded()
        tryVerify(() => list.animateAppends)
        wait(300)
        messages.isLoading = false
        messages.fetchCount = 0
        messages.remove(data.first, data.count)
        wait(1000)
        compare(messages.fetchCount, 0, "Deleting a row must not request older messages")
        const last = list.itemAtIndex(messages.count - 1)
        verify(last)
        verify(Math.abs(last.mapToItem(body, 0, last.height).y - body.height + 16) < 3,
               "Latest message should stay aligned to the bottom")
    }
    function test_wheelStepsAccumulateAndReverse() {
        messages.isLoading = true
        messages.clear()
        for (let i = 0; i < 80; ++i) {
            messages.append({body: "Message " + i, sentAt: "", isMine: true,
                             msgType: 0, filePath: "", seen: true, section: "Today"})
        }
        const body = createTemporaryObject(factory, testCase)
        const list = findChild(body, "messageList")
        messages.initialMessagesLoaded()
        tryVerify(() => list.animateAppends)
        wait(300)
        messages.isLoading = false
        body.followLatest = false
        list.positionViewAtIndex(40, ListView.Beginning)
        wait(50)
        const startY = list.contentY
        mouseWheel(list, 100, 100, 0, 120)
        verify(Math.abs(list.contentY - startY) < 60,
               "A wheel step should animate instead of jumping to its destination")
        mouseWheel(list, 100, 100, 0, 120)
        tryVerify(() => Math.abs(list.contentY - (startY - 120)) < 2)

        mouseWheel(list, 100, 100, 0, 120)
        wait(40)
        const turningY = list.contentY
        mouseWheel(list, 100, 100, 0, -120)
        tryVerify(() => Math.abs(list.contentY - (turningY + 60)) < 2)
    }

    function test_wheelAtTopStillFetchesHistory() {
        messages.isLoading = true
        messages.clear()
        for (let i = 0; i < 30; ++i) {
            messages.append({body: "Message " + i, sentAt: "", isMine: true,
                             msgType: 0, filePath: "", seen: true, section: "Today"})
        }
        const body = createTemporaryObject(factory, testCase)
        const list = findChild(body, "messageList")
        messages.initialMessagesLoaded()
        tryVerify(() => list.animateAppends)
        wait(300)
        messages.isLoading = false
        messages.fetchCount = 0
        body.followLatest = false
        list.positionViewAtBeginning()
        wait(50)
        compare(messages.fetchCount, 0)
        mouseWheel(list, 100, 100, 0, 120)
        tryCompare(messages, "fetchCount", 1)
    }
}
