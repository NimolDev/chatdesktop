import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Dialogs

import Theme
import Shared.UI

Menu {

    id: root

    padding: 6
    implicitWidth: 150
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
            attachmentDialog.open()
        }
    }
    ContextMenuItem {
        text: "File"
        isShowIcon: false
        onTriggered: {
            attachmentDialog.open()
        }
    }
    ContextMenuItem {
        text: "Location"
        isShowIcon: false
        onTriggered: {
            // mapView.show()
            // mapView.raise()
            // mapView.requestActivate()
        }

    }

    FileDialog {
        id: attachmentDialog
        title: "Select file to send"
        fileMode: FileDialog.OpenFiles
        nameFilters: ["All files (*)"]

        onAccepted: {
            // root.sele
            console.log("select: ", selectedFile)
        }
        onRejected: {
            console.log("Reject select file:")
        }
    }



            // MapView  {
            //     id: mapView
            // }


}
