import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Dialogs

import Theme
import Shared.UI

Menu {

    id: root

    padding: 6
    implicitWidth: 150




    signal filePicker(var urls)
    signal imageAndVideoFileSelected(var urls)
    signal filesSelected(var urls)
    signal locationSelected(var coordinate)

    background: Rectangle {
        radius: AppLayouts.x_radius
        color: Colors.suface1
        border.color: Colors.suface3
    }
    transformOrigin: Item.BottomLeft
    enter: Transition {
        ParallelAnimation {
            NumberAnimation {
                property: "opacity"
                from: 0
                to: 1
                duration: 150
                easing.type: Easing.OutCubic
            }
            NumberAnimation {
                property: "scale"
                from: 0.5
                to: 1
                duration: 150
                easing.type: Easing.OutQuad
            }
        }
    }
    exit: Transition {
        ParallelAnimation {
            NumberAnimation {
                property: "opacity"
                from: 1
                to: 0
                duration: 150
            }
            NumberAnimation {
                property: "scale"
                from: 0.5
                to: 0
                duration: 150
            }
        }
    }
    ContextMenuItem {
        text: "Photo or Video"
        isShowIcon: false
        onTriggered: {
            attachmentDialog.title = "Select photo or video to send"
            const imageAndVideo = FileFormatesQml.imageAndVideo
            attachmentDialog.nameFilters = [
                        imageAndVideo,
                        "All files (*)"
                    ]
            attachmentDialog.selectedNameFilter.index = 0
            attachmentDialog.isFileSelect = false
            attachmentDialog.open()
        }
    }
    ContextMenuItem {
        text: "File"
        isShowIcon: false
        onTriggered: {
            attachmentDialog.title = "Select file to send"
            attachmentDialog.nameFilters = [
                "All files (*)",
                FileFormatesQml.imageFilters
            ]
            attachmentDialog.selectedNameFilter.index = 0
            attachmentDialog.isFileSelect = true
            attachmentDialog.open()
        }
    }
    ContextMenuItem {
        text: "Location"
        isShowIcon: false
        onTriggered: {
            if (mapLoader.item) {
                mapLoader.item.show()
            } else {
                mapLoader.active = true
            }
        }

    }

    FileDialog {
        id: attachmentDialog
        title: "Select file to send"
        fileMode: FileDialog.OpenFiles
        nameFilters: ["All files (*)"]

        property var isFileSelect: false
        onAccepted: {
            // console.log("select: ", selectedFile)
            if (isFileSelect) {
                root.filesSelected(selectedFiles)
            } else {
                root.imageAndVideoFileSelected(selectedFiles)
            }

            // root.filePicker(selectedFiles)
        }
        onRejected: {
            // console.log("Reject select file:")
        }

    }

    // Do not create a hidden Window while previewing the messaging Item:
    // Qt Preview would reuse that frameless window as its preview host.
    Loader {
        id: mapLoader
        active: false
        sourceComponent: Component {
            MapView {
                objectName: "mapView"
            }
        }
        onLoaded: item.show()
    }


}
