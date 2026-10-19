import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Pigeon.Gui 1.0
import "../theme/Format.js" as Format

SurfaceCard {
    id: table
    property var sourceModel: null
    readonly property int tableWidth: 860
    implicitHeight: 430
    clip: true

    Flickable {
        id: horizontal
        anchors.fill: parent
        anchors.margins: 1
        contentWidth: Math.max(width, table.tableWidth)
        contentHeight: height
        flickableDirection: Flickable.HorizontalFlick
        boundsBehavior: Flickable.StopAtBounds
        clip: true
        ScrollBar.horizontal: ScrollBar { policy: ScrollBar.AsNeeded }

        Column {
            width: horizontal.contentWidth
            height: horizontal.height

            Rectangle {
                color: Theme.surfaceRaised
                radius: 12
                width: parent.width
                height: 45
                Row {
                    anchors.fill: parent
                    CellText { width: 225; height: parent.height; label: qsTr("APPLICATION"); ink: Theme.textMuted }
                    CellText { width: 136; height: parent.height; label: qsTr("DOWNLOADED"); ink: Theme.textMuted; rightAligned: true }
                    CellText { width: 136; height: parent.height; label: qsTr("UPLOADED"); ink: Theme.textMuted; rightAligned: true }
                    CellText { width: 105; height: parent.height; label: qsTr("CONVERSATIONS"); ink: Theme.textMuted; rightAligned: true }
                    CellText { width: 105; height: parent.height; label: qsTr("REMOTE IPS"); ink: Theme.textMuted; rightAligned: true }
                    CellText { width: 150; height: parent.height; label: qsTr("LAST SEEN"); ink: Theme.textMuted }
                }
            }

            ListView {
                id: list
                width: parent.width
                height: parent.height - 45
                clip: true
                model: table.sourceModel
                boundsBehavior: Flickable.StopAtBounds
                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                delegate: Rectangle {
                    id: entry
                    required property string identity
                    required property string name
                    required property real bytesDown
                    required property real bytesUp
                    required property var conversations
                    required property var remoteIps
                    required property real lastSeen
                    required property int index
                    width: list.width; height: 53
                    color: index % 2 ? Theme.rowAlternate : Theme.surface
                    Row {
                        anchors.fill: parent
                        CellText { width: 225; height: parent.height; label: entry.name; prominent: true; ink: Theme.textPrimary }
                        CellText { width: 136; height: parent.height; label: "≈" + Format.size(entry.bytesDown); ink: Theme.accent; rightAligned: true }
                        CellText { width: 136; height: parent.height; label: "≈" + Format.size(entry.bytesUp); ink: Theme.positive; rightAligned: true }
                        CellText { width: 105; height: parent.height; label: String(entry.conversations); rightAligned: true }
                        CellText { width: 105; height: parent.height; label: String(entry.remoteIps); rightAligned: true }
                        CellText { width: 150; height: parent.height; label: Format.relative(entry.lastSeen); fullText: Format.timestamp(entry.lastSeen) }
                    }
                }
            }
        }
    }

    EmptyState {
        anchors.fill: parent
        visible: list.count === 0
        heading: qsTr("No applications recorded")
        explanation: qsTr("Application names come from collector attribution. Unresolved entries are expected.")
    }
}
