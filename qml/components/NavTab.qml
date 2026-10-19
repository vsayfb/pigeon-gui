import QtQuick
import QtQuick.Controls
import Pigeon.Gui 1.0

Button {
    id: tab

    property bool selected: false
    property string symbol: "•"
    property bool compact: false

    implicitWidth: compact ? 110 : 180
    implicitHeight: 43
    hoverEnabled: true
    Accessible.name: text
    Accessible.description: selected ? qsTr("Current section") : qsTr("Show section")
    Accessible.role: Accessible.Button

    background: Rectangle {
        radius: 9
        color: tab.selected ? Theme.accentSurface : tab.hovered ? Theme.surfaceRaised : "transparent"
        border.width: tab.activeFocus ? 2 : 0
        border.color: Theme.accent
    }

    contentItem: Row {
        spacing: 10
        x: 11
        anchors.verticalCenter: parent.verticalCenter

        Text {
            text: tab.symbol
            font.pixelSize: Theme.textPixels(17)
            color: tab.selected ? Theme.accent : Theme.textSecondary
            verticalAlignment: Text.AlignVCenter
        }

        Text {
            text: tab.text
            font.pixelSize: tab.compact ? 12 : 14
            font.weight: tab.selected ? Font.DemiBold : Font.Normal
            color: tab.selected ? Theme.accent : Theme.textSecondary
            verticalAlignment: Text.AlignVCenter
        }
    }
}
