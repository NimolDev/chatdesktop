import QtQuick
import QtQuick.Effects

Item {
    id: root

    property real progress: 0.0
    property bool downloading: true

    property color progressColor: "white"
    property color trackColor: "#404040"
    property color backgroundColor: "blue"

    property real strokeWidth: 5
    property real progressInset: 2
    property real rotationAngle: 0

    property url iconSource: ""
    property var iconColor: null

    signal clicked()

    implicitWidth: 44
    implicitHeight: 44

    readonly property real normalizedProgress:
        Math.max(0, Math.min(1, progress))

    property bool indeterminate: downloading && normalizedProgress <= 0
    readonly property real arcProgress: indeterminate ? 0.2 : normalizedProgress

    Canvas {
        id: canvas
        anchors.fill: parent
        antialiasing: true

        onPaint: {
            const ctx = getContext("2d")
            ctx.reset()

            const cx = width / 2
            const cy = height / 2

            const outerRadius =
                Math.min(width, height) / 2 - 1

            const strokeRadius =
                Math.max(0, outerRadius - Math.max(0, root.progressInset) - root.strokeWidth / 2)

            // Background
            ctx.beginPath()
            ctx.fillStyle = root.backgroundColor

            ctx.arc(
                cx,
                cy,
                outerRadius,
                0,
                Math.PI * 2
            )

            ctx.fill()

            if (!root.downloading || root.strokeWidth <= 0 || strokeRadius <= 0) {
                return
            }

            // Keep the track and progress inside the filled circle.
            ctx.beginPath()
            ctx.lineWidth = root.strokeWidth
            ctx.strokeStyle = root.trackColor
            ctx.lineCap = "round"

            ctx.arc(
                cx,
                cy,
                strokeRadius,
                0,
                Math.PI * 2
            )

            ctx.stroke()

            // Progress
            const start =
                (-90 + root.rotationAngle)
                * Math.PI / 180

            const sweep =
                Math.PI * 2 *
                root.arcProgress

            ctx.beginPath()
            ctx.lineWidth = root.strokeWidth
            ctx.strokeStyle = root.progressColor
            ctx.lineCap = "round"

            ctx.arc(
                cx,
                cy,
                strokeRadius,
                start,
                start + sweep
            )

            ctx.stroke()
        }

        Connections {
            target: root

            function onProgressChanged() {
                canvas.requestPaint()
            }

            function onArcProgressChanged() {
                canvas.requestPaint()
            }

            function onDownloadingChanged() {
                canvas.requestPaint()
            }

            function onRotationAngleChanged() {
                canvas.requestPaint()
            }

            function onProgressColorChanged() {
                canvas.requestPaint()
            }

            function onTrackColorChanged() {
                canvas.requestPaint()
            }

            function onBackgroundColorChanged() {
                canvas.requestPaint()
            }

            function onProgressInsetChanged() {
                canvas.requestPaint()
            }

            function onStrokeWidthChanged() {
                canvas.requestPaint()
            }
        }

        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
    }

    Image {
        id: imgIcon

        anchors.centerIn: parent

        width: 18
        height: 18

        source: root.iconSource

        fillMode: Image.PreserveAspectFit
        smooth: true
        mipmap: true
    }

    MultiEffect {
        anchors.fill: imgIcon
        source: imgIcon

        brightness: 1.0
        colorization: 1.0
        colorizationColor: root.iconColor
    }

    NumberAnimation on rotationAngle {
        from: 0
        to: 360

        duration: 1800
        loops: Animation.Infinite
        running: root.downloading
    }

    TapHandler {
        onTapped: root.clicked()
    }

    HoverHandler {
        cursorShape: Qt.PointingHandCursor
    }
}