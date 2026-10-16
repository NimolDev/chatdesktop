import QtQuick
import QtQuick.Layouts

import Theme

Item {
    id: root

    signal filesDropped(var files, bool compress)

    property bool canCompress: false

    function fileEntries(urls, compress) {
        const files = []
        for (let i = 0; i < urls.length; ++i) {
            files.push({ url: String(urls[i]), compress: compress })
        }
        return files
    }

    function compressibleUrls(urls) {
        const mediaUrls = []
        if (!urls || urls.length === 0) {
            return mediaUrls
        }
        const mediaExtension = /\.(avif|bmp|gif|heic|heif|jfif|jpeg|jpg|jxl|pbm|pgm|png|ppm|webp|xbm|xpm|mov|m4v)$/
        for (let i = 0; i < urls.length; ++i) {
            const path = String(urls[i]).split(/[?#]/)[0].toLowerCase()
            if (mediaExtension.test(path)) {
                mediaUrls.push(urls[i])
            }
        }
        return mediaUrls
    }

    function resetHover() {
        withoutCompress.hovered = false
        compress.hovered = false
    }

    Rectangle {
        anchors.fill: parent
        visible: dropArea.containsDrag
        color: "transparent"
        z: 1

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: AppLayouts.m_padding
            spacing: 10

            DropTarget {
                id: withoutCompress
                Layout.fillWidth: true
                Layout.fillHeight: true
                title: qsTr("Drop Here")
                subtitle: root.canCompress ? qsTr("to send them without compression")
                                           : qsTr("to send them as files")
            }

            DropTarget {
                id: compress
                visible: root.canCompress
                Layout.fillWidth: true
                Layout.fillHeight: true
                title: qsTr("Drop Here")
                subtitle: qsTr("to send them in a quick way")
            }
        }
    }

    DropArea {
        id: dropArea
        anchors.fill: parent
        z: 2

        function contains(item, point) {
            if (!item.visible) {
                return false
            }
            const p = item.mapToItem(dropArea, 0, 0)
            return point.x >= p.x && point.x <= p.x + item.width
                    && point.y >= p.y && point.y <= p.y + item.height
        }

        onEntered: drag => {
            drag.accepted = drag.hasUrls
            root.canCompress = drag.hasUrls && root.compressibleUrls(drag.urls).length > 0
            root.resetHover()
        }
        onPositionChanged: drag => {
            withoutCompress.hovered = contains(withoutCompress, drag)
            compress.hovered = contains(compress, drag)
        }
        onExited: {
            root.resetHover()
            root.canCompress = false
        }
        onDropped: drop => {
            if (!drop.hasUrls) {
                root.resetHover()
                root.canCompress = false
                return
            }

            if (contains(withoutCompress, drop)) {
                root.filesDropped(root.fileEntries(drop.urls, false), false)
            } else if (contains(compress, drop)) {
                const mediaUrls = root.compressibleUrls(drop.urls)
                if (mediaUrls.length > 0) {
                    root.filesDropped(root.fileEntries(mediaUrls, true), true)
                }
            }

            root.resetHover()
            root.canCompress = false
        }
    }

    component DropTarget: Rectangle {
        id: target

        property bool hovered: false
        property string title
        property string subtitle

        color: Colors.suface1
        border.color: hovered ? Colors.primary : Colors.suface3
        radius: AppLayouts.l_radius

        ColumnLayout {
            anchors.centerIn: parent
            width: parent.width

            Text {
                Layout.fillWidth: true
                text: target.title
                horizontalAlignment: Text.AlignHCenter
                font.family: Typography.family
                font.pixelSize: Typography.title1
                font.weight: Typography.bold
                color: target.hovered ? Colors.primary : Colors.textPrimary
            }
            Text {
                Layout.fillWidth: true
                text: target.subtitle
                horizontalAlignment: Text.AlignHCenter
                font.family: Typography.family
                font.pixelSize: Typography.title3
                font.weight: Typography.bold
                color: target.hovered ? Colors.primary : Colors.textPrimary
            }
        }
    }
}
