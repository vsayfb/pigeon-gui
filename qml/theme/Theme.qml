pragma Singleton

import QtQuick

QtObject {
    // Follow larger platform fonts without shrinking the established layout.
    // The bounded factor helps fixed-width traffic tables stay usable.
    readonly property real textScale: Qt.application.font.pointSize > 10
        ? Math.min(1.4, Qt.application.font.pointSize / 10) : 1

    function textPixels(base) {
        return Math.round(base * textScale)
    }

    readonly property color background: "#101827"
    readonly property color sidebar: "#151F30"
    readonly property color surface: "#1C293B"
    readonly property color surfaceRaised: "#24344B"
    readonly property color border: "#314055"
    readonly property color textPrimary: "#F1F5FA"
    readonly property color textSecondary: "#B9C5D6"
    readonly property color textMuted: "#92A2B8"
    readonly property color accent: "#66D7E8"
    readonly property color accentSurface: "#193C4C"
    readonly property color positive: "#76D9AB"
    readonly property color warning: "#F5C985"
    readonly property color danger: "#F9A8A8"
    readonly property color rowAlternate: "#203046"
}
