import QtQuick
import QtQuick.Controls
import Pigeon.Gui 1.0

Button {
    id: control
    property bool primary: false
    implicitHeight: 34
    implicitWidth: Math.max(36, implicitContentWidth + 28)
    padding: 8
    hoverEnabled: true
    opacity: enabled ? 1 : 0.45

    contentItem: Text {
        text: control.text
        color: control.primary ? "white" : Theme.textSecondary
        font.pixelSize: Theme.textPixels(11)
        font.weight: Font.Medium
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    background: Rectangle {
        radius: 8
        color: control.primary ? Theme.accent
            : control.hovered ? Theme.accentSurface : Theme.surface
        border.color: control.activeFocus ? Theme.accent : Theme.border
        border.width: control.activeFocus ? 2 : 1
    }
}
