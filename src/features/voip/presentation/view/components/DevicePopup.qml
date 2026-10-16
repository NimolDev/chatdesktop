pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Layouts
import QtQuick.Templates as T

import Theme

T.Popup {
    id: root
    implicitWidth: Math.max(
            txtTitle.implicitWidth + 20,
            repeater.maxItemWidth + 16
    )
    implicitHeight: contentColumn.implicitHeight + AppLayouts.x_padding

    property string title: "Camera"
    property var model: []
    property int currentIndex: 0

    signal selected(int index, var item)

    closePolicy: T.Popup.CloseOnEscape
                 | T.Popup.CloseOnPressOutside

    background: Rectangle {
        radius: AppLayouts.x_radius
        color: Colors.suface1

        border {
            width: 1
            color: Colors.suface2
        }
    }
    contentItem: Column {
        id: contentColumn
        spacing: 0
        Text {
            id: txtTitle
            width: parent.width
            height: 40
            leftPadding: 20
            text: root.title
            color: Colors.textPrimary
            font.family: Typography.family
            font.pixelSize: Typography.title4
            font.weight: Typography.semiBold
            verticalAlignment: Text.AlignVCenter
        }
        T.ButtonGroup {
            id: radioGroup
        }
        Repeater {
            id: repeater
            model: root.model

            readonly property real maxItemWidth: {
                let maxWidth = 0
                for (let i = 0; i < count; ++i) {
                    const item = itemAt(i)
                    if (item) {
                        maxWidth = Math.max(maxWidth, item.implicitWidth)
                    }
                }
                return maxWidth
            }

            delegate: Item {
                id:  delegate

                required property int index
                required property var modelData

                implicitWidth: btnRadio.implicitWidth
                width: root.width
                height: 40

                Rectangle {
                    anchors.fill: parent
                    color: mouseArea.containsMouse
                           ? Colors.suface3
                           : "transparent"
                }

                RowLayout {
                    anchors {
                        fill: parent
                        leftMargin: 8
                        rightMargin: 8
                    }
                    T.RadioButton {
                        id: btnRadio

                        text: delegate.modelData.name
                        checked: root.currentIndex === delegate.index
                        T.ButtonGroup.group: radioGroup

                        spacing: 10

                        implicitWidth: indicator.implicitWidth
                                       + spacing
                                       + contentItem.implicitWidth

                        implicitHeight: Math.max(
                            indicator.implicitHeight,
                            contentItem.implicitHeight
                        )

                        indicator: Rectangle {
                            implicitWidth: 20
                            implicitHeight: 20

                            radius: width / 2
                            color: "transparent"

                            border {
                                width: 2
                                color: btnRadio.checked
                                       ? Colors.primary500
                                       : Colors.textSecond
                            }

                            Rectangle {
                                anchors.centerIn: parent

                                width: 10
                                height: 10
                                radius: width / 2

                                visible: btnRadio.checked
                                color: Colors.primary500
                            }
                        }

                        contentItem: Text {
                            text: btnRadio.text
                            leftPadding: btnRadio.indicator.width + btnRadio.spacing
                            color: Colors.textPrimary


                            font.family: Typography.family
                            font.pixelSize: Typography.body
                            font.weight: Typography.medium

                            verticalAlignment: Text.AlignVCenter
                        }
                    }

                }


                MouseArea {
                    id: mouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked:  {
                        root.currentIndex = delegate.index
                        root.selected(delegate.index, delegate.modelData)
                        root.close()
                    }

                }
            }
        }
    }


}
