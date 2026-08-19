import QtQuick 2.15
import Theme

Item {

    property bool isShow: false
    id: sideMenu
    visible: isShow
    implicitWidth: 250


    Rectangle {
        anchors.fill: parent
        anchors.margins: AppLayouts.l_padding
        color: Colors.suface1
        radius: AppLayouts.l_radius
        border.color: Colors.suface3
    }

}
