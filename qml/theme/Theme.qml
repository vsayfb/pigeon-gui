pragma Singleton

import QtQuick

QtObject {
    property bool dark: false

    readonly property real textScale: Qt.application.font.pointSize > 10
        ? Math.min(1.4, Qt.application.font.pointSize / 10) : 1

    function textPixels(base) {
        return Math.round(base * textScale)
    }

    readonly property color background: dark ? "#101827" : "#F7F9FD"
    readonly property color sidebar: dark ? "#151F30" : "#FFFFFF"
    readonly property color surface: dark ? "#1C293B" : "#FFFFFF"
    readonly property color surfaceRaised: dark ? "#24344B" : "#F3F6FB"
    readonly property color border: dark ? "#314055" : "#E3EAF4"
    readonly property color textPrimary: dark ? "#F1F5FA" : "#122344"
    readonly property color textSecondary: dark ? "#B9C5D6" : "#526582"
    readonly property color textMuted: dark ? "#92A2B8" : "#7789A6"
    readonly property color accent: dark ? "#70B3FF" : "#2874F5"
    readonly property color accentSurface: dark ? "#193C59" : "#EAF3FF"
    readonly property color positive: dark ? "#70DDAE" : "#00AC78"
    readonly property color download: positive
    readonly property color upload: dark ? "#70B3FF" : "#009AFF"
    readonly property color warning: dark ? "#F5C985" : "#997019"
    readonly property color danger: dark ? "#F9A8A8" : "#BF3E56"
    readonly property color rowAlternate: dark ? "#203046" : "#F7F9FD"
}
