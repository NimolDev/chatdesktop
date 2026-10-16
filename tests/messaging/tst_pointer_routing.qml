import QtQuick
import QtTest
import "subject" as Subject

TestCase {
    id: testCase
    name: "MessagingBodyPointerRouting"
    when: windowShown
    width: 600
    height: 400
    visible: true
    property var body
    property var action
    property var slider

    ListModel {
        id: messages
        property var voiceController: testMessaging
        property bool voiceDownloading: testMessaging.voiceDownloading
        property real voiceDownloadProgress: testMessaging.voiceDownloadProgress
        property url voicePlaybackSource: testMessaging.voicePlaybackSource
        property bool voicePlaybackPlaying: testMessaging.voicePlaybackPlaying
        property bool voicePlaybackSeekable: testMessaging.voicePlaybackSeekable
        property real voicePlaybackPosition: testMessaging.voicePlaybackPosition
        property real voicePlaybackDuration: testMessaging.voicePlaybackDuration
        function playVoice(source) { testMessaging.playVoice(source) }
        function seekVoice(position) { testMessaging.seekVoice(position) }
        property bool isLoading: true
        signal initialMessagesLoaded()
        signal olderMessagesLoaded(int insertedCount)
        ListElement { body: "message"; sentAt: ""; isMine: false; msgType: 1; filePath: ""; duration: 3000; seen: true; section: "Today" }
    }
    Component {
        id: bodyFactory
        Subject.MessagingBody { width: 600; height: 400; model: messages }
    }
    SignalSpy { id: actions; signalName: "clicked" }
    SignalSpy { id: selections; signalName: "selectionRequested" }
    SignalSpy { id: menus; signalName: "contextMenuRequested" }

    function init() {
        messages.setProperty(0, "filePath", Qt.resolvedUrl("audio.wav").toString())
        testMessaging.playVoice(Qt.resolvedUrl("audio.wav"))
        testMessaging.stopVoicePlayback()
        messages.setProperty(0, "msgType", 1)
        messages.setProperty(0, "isMine", false)
        body = createTemporaryObject(bodyFactory, testCase)
        verify(body)
        selections.target = body
        menus.target = body
        selections.clear()
        menus.clear()
        tryVerify(() => findChild(body, "action") !== null)
        action = findChild(body, "action")
        slider = findChild(body, "slider")
        actions.target = action
        actions.clear()
        wait(30)
    }
    function test_actionClick_data() {
        return [{tag: "incoming", outgoing: false}, {tag: "outgoing", outgoing: true}]
    }
    function test_actionClick(data) {
        messages.setProperty(0, "isMine", data.outgoing)
        wait(30)
        if (data.outgoing) {
            action = findChild(body, "outgoingAction")
            actions.target = action
        }
        mouseClick(action, action.width / 2, action.height / 2)
        compare(actions.count, 1)
        compare(selections.count, 0)
    }
    function test_sliderDrag() {
        tryCompare(slider, "enabled", true, 10000)
        mouseDrag(slider, 10, slider.height / 2, slider.width - 25, 0)
        verify(slider.value > slider.to * 0.7)
        compare(selections.count, 0)
    }
    function test_actionHover_data() {
        return [{tag: "incoming", outgoing: false}, {tag: "outgoing", outgoing: true}]
    }
    function test_actionHover(data) {
        messages.setProperty(0, "isMine", data.outgoing)
        wait(30)
        if (data.outgoing) {
            action = findChild(body, "outgoingAction")
        }
        const hover = findChild(action, "actionHover")
        verify(hover)
        mouseMove(body, body.width - 1, 1)
        mouseMove(action, action.width / 2, action.height / 2)
        tryCompare(hover, "hovered", true)
        compare(hover.cursorShape, Qt.PointingHandCursor)
        mouseMove(body, body.width - 1, 1)
        tryCompare(hover, "hovered", false)
    }
    function test_selectionModeConsumesAction() {
        body.selectionMode = true
        mouseClick(action, action.width / 2, action.height / 2)
        compare(actions.count, 0)
        compare(selections.count, 1)
    }
    function test_contextMenu() {
        mouseClick(action, action.width / 2, action.height / 2, Qt.RightButton)
        compare(actions.count, 0)
        compare(menus.count, 1)
        compare(menus.signalArguments[0][1], false)
    }
    function test_dragSelectionOutsideBubble() {
        const point = action.mapToItem(body, 350, 20)
        mouseDrag(body, point.x, point.y, 0, 30)
        verify(selections.count > 0)
        compare(body.dragSelecting, false)
    }
    function test_textBubbleKeepsDragSelection() {
        let delegate = action.parent
        while (delegate && delegate.msgType === undefined) {
            delegate = delegate.parent
        }
        verify(delegate)
        messages.setProperty(0, "msgType", 0)
        wait(30)
        const bubble = delegate.bubbleItem
        verify(bubble)
        mouseDrag(bubble, bubble.width / 2, bubble.height / 2, 0, 10)
        verify(selections.count > 0)
        compare(body.dragSelecting, false)
    }
}
