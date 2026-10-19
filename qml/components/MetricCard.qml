import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Pigeon.Gui 1.0

SurfaceCard {
    id: metric
    property string label: ""
    property string value: "—"
    property string annotation: ""
    property string symbol: "≋"
    property color valueColor: Theme.textPrimary
    implicitHeight: 94
    radius: 12

    RowLayout {
        anchors.fill: parent
        anchors.margins: 15
        spacing: 12

        Rectangle {
            Layout.preferredWidth: 38
            Layout.preferredHeight: 42
            radius: 12
            color: Theme.accentSurface

            Text {
                anchors.centerIn: parent
                text: metric.symbol
                color: metric.valueColor === Theme.textPrimary ? Theme.accent : metric.valueColor
                font.pixelSize: 25
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 4

            Text {
                Layout.fillWidth: true
                text: metric.label
                font.pixelSize: Theme.textPixels(11)
                color: Theme.textSecondary
                elide: Text.ElideRight
            }

            Text {
                Layout.fillWidth: true
                text: metric.value
                font.pixelSize: Theme.textPixels(21)
                font.weight: Font.DemiBold
                color: metric.valueColor
                elide: Text.ElideRight
                ToolTip.visible: hover.hovered
                ToolTip.text: metric.value + (metric.annotation ? " · " + metric.annotation : "")
                HoverHandler { id: hover }
            }
        }
    }
}
