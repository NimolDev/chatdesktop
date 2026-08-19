import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import Theme

Item {
    id: root

    readonly property int padding: AppLayouts.xl_padding
    readonly property int spacing: AppLayouts.x_padding
    property string text: "Button"
    property int number: -1
    property color textColor: "white"
    property Rectangle background: defaultBackground
    signal clicked()


    implicitWidth: padding + txtText.implicitWidth + (number > 0 ? txtNumber.implicitWidth + spacing : 0 ) + padding
    implicitHeight: 45

    onBackgroundChanged: {
        if (background) {
            background.parent = root
            background.anchors.fill = root
            background.z = -1
        }
    }
    Rectangle {
        id: defaultBackground
        anchors.fill: parent
        radius: 4
        color: "transparent"
    }
    Text {
        id: txtText
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left
        anchors.leftMargin: root.padding
        text: root.text
        color: root.textColor
        font.family: Typography.family
        font.pixelSize: Typography.body
        font.weight: Typography.medium

    }
    Item {
        id: txtNumber
        visible: root.number > 0
        anchors.verticalCenter: parent.verticalCenter
        anchors.right: parent.right
        anchors.rightMargin: root.padding
        anchors.left: txtText.right
        anchors.leftMargin: root.spacing

        readonly property int digitCount: Math.max(
            1,
            String(Math.max(displayedNumber, nextNumber, root.number)).length
        )

        implicitWidth: Math.max(
            txtCurrent.implicitWidth,
            txtNext.implicitWidth,
            txtNumberMeasure.implicitWidth
        )
        implicitHeight: txtCurrent.implicitHeight
        width: implicitWidth
        height: implicitHeight

        clip: true
        // These are animation state, not bindings to root.number. Keeping
        // them independent lets onNumberChanged compare old and new values.
        property int displayedNumber: 0
        property int nextNumber: 0

        Component.onCompleted: {
            displayedNumber = root.number
            nextNumber = root.number
        }

        Text {
            id: txtNumberMeasure

            visible: false
            text: "8".repeat(txtNumber.digitCount)
            font.family: Typography.family
            font.pixelSize: Typography.body
            font.weight: Typography.medium
        }

        Text {
            y: 0

            id: txtCurrent
            text: txtNumber.displayedNumber
            color: root.textColor
            font.family: Typography.family
            font.pixelSize: Typography.body
            font.weight: Typography.medium

        }

        Text {
            y: -height
            id: txtNext
            text: txtNumber.nextNumber
            color: root.textColor
            font.family: Typography.family
            font.pixelSize: Typography.body
            font.weight: Typography.medium

        }

        Connections {
            target: root
            function onNumberChanged() {
                if (root.number == txtNumber.displayedNumber) {
                    return
                }

                flipAnimation.stop()
                txtCurrent.y = 0

                let increasing = root.number > txtNumber.displayedNumber
                txtNumber.nextNumber = root.number
                txtNext.y = increasing ? txtNumber.height : -txtNumber.height
                currentAnim.to = increasing ? -txtNumber.height : txtNumber.height
                nextAnim.from = txtNext.y
                flipAnimation.start()
            }
        }
        ParallelAnimation {
            id: flipAnimation
            NumberAnimation {
                id: currentAnim
                target: txtCurrent
                property: "y"
                duration: 180
                easing.type: Easing.OutCubic
            }

            NumberAnimation {
                id: nextAnim
                target: txtNext
                property: "y"
                to: 0
                duration: 180
                easing.type: Easing.OutCubic
            }

            onFinished: {

                txtNumber.displayedNumber = root.number
                txtNumber.nextNumber = root.number
                txtCurrent.y = 0
                txtNext.y = txtNumber.height
            }

        }
    }


    MouseArea {
        id: mouse
        anchors.fill: parent
        onClicked: root.clicked()
        hoverEnabled: true
        onHoveredChanged: {
            if (root.background != defaultBackground) {
                root.background.color = root.background.color.alpha(mouse.containsMouse ? 0.9 : 1)
            }else {
                root.background.color = mouse.containsMouse ? "black" : "transparent"
            }
        }
    }



}
