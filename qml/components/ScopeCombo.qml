import QtQuick
import QtQuick.Controls
import Pigeon.Gui 1.0

ComboBox {
    id: combo
    property string caption: ""
    property var choices: []
    property int selectedIndex: 0
    signal chosen(int selected)
    model: choices
    currentIndex: selectedIndex
    onSelectedIndexChanged: {
        if (currentIndex !== selectedIndex)
            currentIndex = selectedIndex
    }
    height: 38
    width: Math.max(128, implicitContentWidth + 32)
    hoverEnabled: true
    Accessible.name: caption
    Accessible.description: currentText
    onActivated: function(index) { combo.chosen(index) }
    contentItem: Text {
        leftPadding: 9
        rightPadding: 23
        text: combo.displayText
        font.pixelSize: Theme.textPixels(12)
        color: Theme.textPrimary
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
    background: Rectangle {
        color: Theme.surface
        border.color: combo.activeFocus ? Theme.accent : Theme.border
        radius: 7
    }
    indicator: Text {
        text: "⌄"
        x: combo.width - 20
        height: combo.height
        verticalAlignment: Text.AlignVCenter
        color: Theme.textSecondary
    }
}
