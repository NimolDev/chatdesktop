pragma Singleton
import QtQuick

QtObject {
    readonly property string family: "Inter 18pt"

    readonly property int display: 32
    readonly property int title1: 28
    readonly property int title2: 22
    readonly property int title3: 18
    readonly property int title4: 16
    readonly property int body: 14
    readonly property int secondText: 13
    readonly property int caption: 12

    readonly property int light: Font.Light
    readonly property int regular: Font.Normal
    readonly property int medium: Font.Medium
    readonly property int semiBold: Font.DemiBold
    readonly property int bold: Font.Bold


}
