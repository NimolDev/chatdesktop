import QtQuick
import QtQuick.Layouts

import Theme

Item {
    id: root

    signal filesDropped(var urls, bool compress)

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
                subtitle: qsTr("to send them without compression")
            }

            DropTarget {
                id: compress
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
            const p = item.mapToItem(dropArea, 0, 0)
            return point.x >= p.x && point.x <= p.x + item.width
                    && point.y >= p.y && point.y <= p.y + item.height
        }

        onPositionChanged: drag => {
            withoutCompress.hovered = contains(withoutCompress, drag)
            compress.hovered = contains(compress, drag)
        }
        onExited: root.resetHover()
        onDropped: drop => {
            if (!drop.hasUrls)
                return

            if (contains(withoutCompress, drop))
                root.filesDropped(drop.urls, false)
            else if (contains(compress, drop))
                root.filesDropped(drop.urls, true)

            root.resetHover()
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
