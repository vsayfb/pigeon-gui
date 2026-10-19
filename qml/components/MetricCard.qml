import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Pigeon.Gui 1.0

SurfaceCard {
    id: metric
    property string label: ""
    property string value: "—"
    property string annotation: ""
    property color valueColor: Theme.textPrimary
    implicitHeight: 124

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 17
        spacing: 5

        Text {
            Layout.fillWidth: true
            text: metric.label
            font.pixelSize: 12
            color: Theme.textSecondary
            elide: Text.ElideRight
        }
        Text {
            Layout.fillWidth: true
            text: metric.value
            font.pixelSize: 23
            font.weight: Font.DemiBold
            color: metric.valueColor
            elide: Text.ElideRight
            ToolTip.visible: totalHover.hovered && truncated
            ToolTip.text: metric.value
            HoverHandler { id: totalHover }
        }
        Text {
            Layout.fillWidth: true
            text: metric.annotation
            font.pixelSize: 11
            color: Theme.textMuted
            elide: Text.ElideRight
        }
    }
}
