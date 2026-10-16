import QtQuick
import QtQuick.Controls as T

import Shared.UI
import Theme
Item {
    id: root

    implicitWidth: 40
    implicitHeight: 40

    property url icon
    property bool showSwape: true
    property color background: Colors.primary700
    property int iconPadding: 12

    // Transparent distance between the two buttons
    property real gapSize: 2

    signal clicked()
    signal swapeClicked()

    /*
     * Small button position is calculated manually because
     * Canvas needs its coordinates for the transparent cutout.
     */
    CircleButton {
        id: btnChangeCamera

        visible: root.showSwape

        width: 25
        height: width

        // Same positioning as your original anchors
        x: root.width - width + width /4
        y: -height + height / 1.4

        z: 3

        color: root.background
        borderWidth: 0

        iconSource: AppAssets.icChevronUp
        padding: 4

        onClicked: root.swapeClicked()
    }

    /*
     * Big button background.
     *
     * We first draw the big circle, then physically erase
     * the area around btnChangeCamera.
     */
    Canvas {
        id: bigButtonBackground

        anchors.fill: parent
        z: 0

        onPaint: {
            const ctx = getContext("2d")

            ctx.reset()

            // --------------------------------
            // Draw big circle
            // --------------------------------

            ctx.globalCompositeOperation = "source-over"
            ctx.fillStyle = root.background

            ctx.beginPath()
            ctx.arc(
                width / 2,
                height / 2,
                Math.min(width, height) / 2,
                0,
                Math.PI * 2
            )
            ctx.closePath()
            ctx.fill()

            // --------------------------------
            // Cut transparent hole
            // --------------------------------

            if (root.showSwape) {

                const centerX =
                        btnChangeCamera.x +
                        btnChangeCamera.width / 2

                const centerY =
                        btnChangeCamera.y +
                        btnChangeCamera.height / 2

                const radius =
                        btnChangeCamera.width / 2 +
                        root.gapSize

                ctx.globalCompositeOperation = "destination-out"

                ctx.beginPath()
                ctx.arc(
                    centerX,
                    centerY,
                    radius,
                    0,
                    Math.PI * 2
                )
                ctx.closePath()
                ctx.fill()
            }

            ctx.globalCompositeOperation = "source-over"
        }

        Connections {
            target: root

            function onBackgroundChanged() {
                bigButtonBackground.requestPaint()
            }

            function onGapSizeChanged() {
                bigButtonBackground.requestPaint()
            }

            function onShowSwapeChanged() {
                bigButtonBackground.requestPaint()
            }
        }
    }

    /*
     * Big button icon
     */
    Image {
        id: mainIcon

        anchors {
            fill: parent
            margins: root.iconPadding
        }

        z: 1

        source: root.icon

        fillMode: Image.PreserveAspectFit
        smooth: true

        // Don't intercept mouse events
        enabled: false
    }

    /*
     * Big button click handling
     */
    T.AbstractButton {
        id: bigButtonMouseArea

        anchors.fill: parent
        z: 2

        background: null

        onClicked: root.clicked()
    }

    /*
     * Keep small button above big button's click area.
     */
    Component.onCompleted: {
        bigButtonBackground.requestPaint()
    }
}