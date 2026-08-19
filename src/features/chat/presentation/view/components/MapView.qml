import QtQuick
import QtQuick.Controls.Basic
import QtPositioning
import QtLocation


import Theme

Window {
    id: root

    maximumWidth: 500
    maximumHeight: 500
    width: 500
    height: 500
    minimumWidth: width
    minimumHeight: height

    modality: Qt.ApplicationModal
    flags: Qt.Window | Qt.FramelessWindowHint
    visible: false

    property bool centerOnNextPosition: false

    function centerOnCurrentLocation() {
        const coordinate = positionSource.position.coordinate
        if (coordinate.isValid) {
            map.map.center = coordinate
            map.map.zoomLevel = 16
            return
        }

        centerOnNextPosition = true
        positionSource.update()
    }

    PositionSource {
        id: positionSource
        active: true

        onPositionChanged: {

            if (position.coordinate.isValid && root.centerOnNextPosition) {
                map.map.center = position.coordinate
                map.map.zoomLevel = 16
                root.centerOnNextPosition = false
            }
        }
    }

    // Plugin {
    //     id: mapPlugin
    //     name: "osm"
    // }

    Rectangle {
        id: frameless
        anchors {
            left: parent.left
            top: parent.top
            right: parent .right
        }
        height: 50
        color: Colors.background

        DragHandler {
            target: null
            onActiveChanged: {
                if (active) {
                    root.startSystemMove()
                }
            }
        }

        Button {
            text: "Close"
            anchors {
                top: parent.top
                bottom: parent.bottom
                right: parent.right
            }
            onClicked: {
                root.close()
            }
        }
    }

    MapView {
        id: map
        anchors {
            top: frameless.bottom
            left: parent.left
            right: parent.right
            bottom:parent.bottom
        }

        map.center: QtPositioning.coordinate(11.5564, 104.9282)
        map.zoomLevel: 14

        map.plugin: Plugin {
            id: mapPlugin
            name: "osm"
            PluginParameter {
                name: "osm.useragent"
                value: "ChatApp/1.0"
            }
            PluginParameter {
                name: "osm.mapping.providersrepository.disabled"
                value: true
            }
            PluginParameter {
                name: "osm.mapping.custom.host"
                value: "https://tile.openstreetmap.org/"
            }
        }

        Component.onCompleted: {
            console.log("Supported map types:")
            for (let i = 0; i < map.map.supportedMapTypes.length; ++i) {
                const type = map.map.supportedMapTypes[i]
                console.log(i, type.name, type.style)

                // The custom host is exposed as CustomMap. Selecting a
                // StreetMap can choose a provider that requires an API key.
                if (type.style === MapType.CustomMap) {
                    map.map.activeMapType = type
                    break
                }
                root.centerOnCurrentLocation()
            }
        }

        MapQuickItem {
            id: currentLocationPin

            parent: map.map
            visible: positionSource.position.coordinate.isValid
            coordinate: positionSource.position.coordinate
            autoFadeIn: true
            anchorPoint.x: marker.width / 2
            anchorPoint.y: marker.height / 2
            sourceItem: Rectangle {
                id: marker
                width: 18
                height: 18
                radius: width / 2
                color: "blue"
                border.width: 3
                border.color: "white"
            }
        }
    }

    Button {
        id: btn

        anchors {
            right: parent.right
            bottom: parent.bottom
            margins: 16
        }
        z: 1
        text: qsTr("My location")

        onClicked: root.centerOnCurrentLocation()
    }
}
