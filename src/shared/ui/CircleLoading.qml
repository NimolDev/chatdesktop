import QtQuick

Item {
    id: root

    property bool running: true
    property color progressColor: "#2196F3"
    property color trackColor: "transparent"
    property real strokeWidth: 3
    property int duration: 1300

    readonly property real inset: 0

    // One phase = expand + shrink
    property real phase: 0

    readonly property real minSweep: 20
    readonly property real maxSweep: 324
    readonly property real sweepRange: maxSweep - minSweep

    implicitWidth: 40
    implicitHeight: 40

    onPhaseChanged: canvas.requestPaint()
    onProgressColorChanged: canvas.requestPaint()
    onTrackColorChanged: canvas.requestPaint()
    onStrokeWidthChanged: canvas.requestPaint()

    Canvas {
        id: canvas
        anchors.fill: parent

        antialiasing: true
        renderTarget: Canvas.Image

        onPaint: {
            const ctx = getContext("2d")
            ctx.reset()

            const cx = width / 2
            const cy = height / 2

            const radius = Math.max(
                0,
                Math.min(width, height) / 2
                    - root.inset
                    - root.strokeWidth / 2
                    - 1
            )

            // Each full phase advances the arc by 252 degrees
            const cycle = Math.floor(root.phase)
            const progress = root.phase - cycle

            const baseAngle = -90 + cycle * root.sweepRange

            let startAngle
            let endAngle

            if (progress < 0.5) {
                // Expand: head moves forward
                const t = progress * 2
                const eased = t * t * (3 - 2 * t)

                startAngle = baseAngle
                endAngle = baseAngle
                         + root.minSweep
                         + root.sweepRange * eased
            } else {
                // Shrink: tail catches up to head
                const t = (progress - 0.5) * 2
                const eased = t * t * (3 - 2 * t)

                startAngle = baseAngle + root.sweepRange * eased
                endAngle = baseAngle + root.maxSweep
            }

            const start = startAngle * Math.PI / 180
            const end = endAngle * Math.PI / 180

            ctx.lineWidth = root.strokeWidth
            ctx.lineCap = "round"

            if (root.trackColor.a > 0) {
                ctx.beginPath()
                ctx.strokeStyle = root.trackColor
                ctx.arc(cx, cy, radius, 0, Math.PI * 2)
                ctx.stroke()
            }

            ctx.beginPath()
            ctx.strokeStyle = root.progressColor
            ctx.arc(cx, cy, radius, start, end)
            ctx.stroke()
        }
        RotationAnimator on rotation {
            from: 0
            to: 360
            duration: root.duration * 2
            loops: Animation.Infinite
            running: root.running && root.visible
        }
    }

    NumberAnimation on phase {
        from: 0
        to: 10000
        duration: root.duration * 2 * 10000
        loops: 1
        running: root.running && root.visible
    }
}