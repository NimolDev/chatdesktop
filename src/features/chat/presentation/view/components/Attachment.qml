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
            attachmentDialog.title = "Select photo or video to send"
            attachmentDialog.nameFilters = [
                "Images (*.avif *.bmp *.gif *.heic *.heif *.jfif *.jpeg *.jpg *.jxl *.pbm *.pgm *.png *.ppm *.webp *.xbm *.xpm)",
                "Videos (*.mp4 *.mov *.m4v)"
            ]
            attachmentDialog.selectedNameFilter.index = 0
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
                "Documents (*.pdf *.doc *.docx *.odt *.rtf *.txt *.md)",
                "Spreadsheets (*.xls *.xlsx *.ods *.csv *.tsv)",
                "Presentations (*.ppt *.pptx *.odp)",
                "Archives (*.zip *.rar *.7z *.tar *.gz *.bz2 *.xz)",
                "Audio (*.mp3 *.wav *.m4a *.aac *.ogg *.flac *.aiff)",
                "Images (*.avif *.bmp *.gif *.heic *.heif *.jfif *.jpeg *.jpg *.jxl *.pbm *.pgm *.png *.ppm *.webp *.xbm *.xpm)",
                "Videos (*.mp4 *.mov *.m4v)"
            ]
            attachmentDialog.selectedNameFilter.index = 0
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

        onAccepted: {
            // root.sele
            console.log("select: ", selectedFile)
        }
        onRejected: {
            console.log("Reject select file:")
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
