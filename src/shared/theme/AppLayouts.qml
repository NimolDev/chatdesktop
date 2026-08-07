pragma Singleton
import QtQuick

QtObject {

    // Window frame

    readonly property int minWidth: 640
    readonly property int minHeight: 480
    readonly property int width: 1100
    readonly property int height: 720

    // App Layout Padding
    readonly property int s_padding: 4
    readonly property int m_padding: 6
    readonly property int l_padding: 8
    readonly property int x_padding: 12
    readonly property int xl_padding: 16
    readonly property int xxl_padding: 24
    readonly property int xxxl_padding: 32

    // App Corner Raius
    readonly property int s_radius: 4
    readonly property int m_radius: 6
    readonly property int l_radius: 8
    readonly property int x_radius: 12
    readonly property int xl_radius: 16
    readonly property int xxl_radius: 24
    readonly property int xxxl_radius: 32


}
