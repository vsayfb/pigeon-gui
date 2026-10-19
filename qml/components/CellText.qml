import QtQuick
import QtQuick.Controls
import Pigeon.Gui 1.0

Item {
    id: cell
    property string label: ""
    property string fullText: label
    property color ink: Theme.textSecondary
    property bool rightAligned: false
    property bool prominent: false
    property int inset: 8

    implicitHeight: 48

    Text {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: cell.inset
        anchors.rightMargin: cell.inset
        anchors.verticalCenter: parent.verticalCenter
        text: cell.label
        textFormat: Text.PlainText
        color: cell.ink
        font.pixelSize: Theme.textPixels(12)
        font.weight: cell.prominent ? Font.DemiBold : Font.Normal
        elide: Text.ElideRight
        horizontalAlignment: cell.rightAligned ? Text.AlignRight : Text.AlignLeft
        verticalAlignment: Text.AlignVCenter
        ToolTip.visible: hover.hovered && cell.fullText.length > 0
        ToolTip.delay: 600
        ToolTip.text: cell.fullText
        HoverHandler { id: hover }
    }
}
