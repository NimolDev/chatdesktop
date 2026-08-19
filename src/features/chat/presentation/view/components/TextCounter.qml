import QtQuick 2.15

Item {
    id: counter

    width: 40
    height: 30
    clip: true

    property int value: 1
    property int oldValue: value
    property string text: ""
    property color color:  "white"
    property font font: font()

    function setValue(newValue) {
        if (newValue === value)
            return

        oldValue = value

        let increasing = newValue > value
        value = newValue

        oldText.text = oldValue
        newText.text = value

        oldText.y = 0
        newText.y = increasing ? height : -height

        oldText.opacity = 1
        newText.opacity = 0

        oldAnimation.to = increasing ? -height : height
        newAnimation.from = increasing ? height : -height

        animation.start()
    }

    Text {
        id: oldText
        anchors.horizontalCenter: parent.horizontalCenter
        text: counter.value
        color: "white"
    }

    Text {
        id: newText
        anchors.horizontalCenter: parent.horizontalCenter
        color: "white"
    }

    ParallelAnimation {
        id: animation

        NumberAnimation {
            id: oldAnimation
            target: oldText
            property: "y"
            to: -counter.height
            duration: 180
            easing.type: Easing.OutCubic
        }

        NumberAnimation {
            target: oldText
            property: "opacity"
            to: 0
            duration: 150
        }

        NumberAnimation {
            id: newAnimation
            target: newText
            property: "y"
            from: counter.height
            to: 0
            duration: 180
            easing.type: Easing.OutCubic
        }

        NumberAnimation {
            target: newText
            property: "opacity"
            from: 0
            to: 1
            duration: 180
        }
    }
}