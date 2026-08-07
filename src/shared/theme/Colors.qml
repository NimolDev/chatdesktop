pragma Singleton
import QtQuick

QtObject {

    // Primary
    readonly property color primary50: "#FAF6EB"
    readonly property color primary100: "#F3EBCE"
    readonly property color primary200: "#E6DAAC"
    readonly property color primary300: "#D1C79C"
    readonly property color primary400: "#BDB38D"
    readonly property color primary500: "#ABA27F"
    readonly property color primary600: "#867F63"
    readonly property color primary700: "#645E49"
    readonly property color primary800: "#444030"
    readonly property color primary900: "#242218"
    readonly property color primary950: "#17160E"

    // Yellow
    readonly property color yellow50: "#F4B740"
    readonly property color yellow100: "#E6AC3C"
    readonly property color yellow200: "#CB9834"
    readonly property color yellow300: "#B1842C"
    readonly property color yellow400: "#946E23"
    readonly property color yellow500: "#7C5B1C"
    readonly property color yellow600: "#644914"
    readonly property color yellow700: "#4D380E"
    readonly property color yellow800: "#352507"
    readonly property color yellow900: "#211603"
    readonly property color yellow950: "#150D01"

    // Green
    readonly property color green50: "#4CAF50"
    readonly property color green100: "#48A64C"
    readonly property color green200: "#3E9242"
    readonly property color green300: "#357E38"
    readonly property color green400: "#2C6B2E"
    readonly property color green500: "#245B26"
    readonly property color green600: "#1B491D"
    readonly property color green700: "#133815"
    readonly property color green800: "#0B270C"
    readonly property color green900: "#061A06"
    readonly property color green950: "#020E03"

    // White
    readonly property color white50: "#F1F1F1"
    readonly property color white100: "#E2E2E2"
    readonly property color white200: "#C9C9C9"
    readonly property color white300: "#AEAEAE"
    readonly property color white400: "#969696"
    readonly property color white500: "#7C7C7C"
    readonly property color white600: "#636363"
    readonly property color white700: "#494949"
    readonly property color white800: "#323232"
    readonly property color white900: "#1B1B1B"
    readonly property color white950: "#111111"

    // Black
    readonly property color black50: "#E2E6E9"
    readonly property color black100: "#C3CAD1"
    readonly property color black200: "#8D97A1"
    readonly property color black300: "#5F666D"
    readonly property color black400: "#373C40"
    readonly property color black500: "#111315"
    readonly property color black600: "#0F1113"
    readonly property color black700: "#0A0B0D"
    readonly property color black800: "#060709"
    readonly property color black900: "#030404"
    readonly property color black950: "#030404"

    // Red
    readonly property color red50: "#FFECEC"
    readonly property color red100: "#FFDDDD"
    readonly property color red200: "#FFB7B7"
    readonly property color red300: "#FF9191"
    readonly property color red400: "#FF6565"
    readonly property color red500: "#FF0000"
    readonly property color red600: "#CE0000"
    readonly property color red700: "#9C0000"
    readonly property color red800: "#6D0000"
    readonly property color red900: "#450000"
    readonly property color red950: "#2D0000"

    // Blue
    readonly property color blue50: "#EFF4FF"
    readonly property color blue100: "#E2EBFF"
    readonly property color blue200: "#C4D8FF"
    readonly property color blue300: "#A4C5FF"
    readonly property color blue400: "#7FB2FF"
    readonly property color blue500: "#4A9FFF"
    readonly property color blue600: "#007FDB"
    readonly property color blue700: "#005EA4"
    readonly property color blue800: "#003F71"
    readonly property color blue900: "#002242"
    readonly property color blue950: "#00142A"


    // --- App Color ---
    readonly property color primary: primary500
    readonly property color primaryHover: primary300
    readonly property color primaryPressed: primary700

    readonly property color background: black500
    readonly property color suface1: white900 // for card view
    readonly property color suface2: white800
    readonly property color suface3: black400
    readonly property color border: black300

    readonly property color textPrimary: white50
    readonly property color textSecond: white300
    readonly property color textDisable: black300

    readonly property color success: green100
    readonly property color warning: yellow100
    readonly property color error: red500
    readonly property color info: blue500





}