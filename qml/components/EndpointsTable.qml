import QtQuick
import QtQuick.Controls
import Pigeon.Gui 1.0
import "../theme/Format.js" as Format

SurfaceCard {
    id: table
    property var sourceModel: null
    readonly property int tableWidth: 900
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
                    CellText { width: 295; height: parent.height; label: qsTr("REMOTE IP"); ink: Theme.textMuted }
                    CellText { width: 270; height: parent.height; label: qsTr("OBSERVED DNS NAME"); ink: Theme.textMuted }
                    CellText { width: 165; height: parent.height; label: qsTr("FIRST SEEN"); ink: Theme.textMuted }
                    CellText { width: 165; height: parent.height; label: qsTr("LAST SEEN"); ink: Theme.textMuted }
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
                    required property string ip
                    required property string name
                    required property real firstSeen
                    required property real lastSeen
                    required property int index
                    width: list.width; height: 53
                    color: index % 2 ? Theme.rowAlternate : Theme.surface
                    Row {
                        anchors.fill: parent
                        CellText { width: 295; height: parent.height; label: entry.ip; prominent: true; ink: Theme.textPrimary }
                        CellText { width: 270; height: parent.height; label: entry.name ? entry.name : qsTr("Not observed"); fullText: entry.name }
                        CellText { width: 165; height: parent.height; label: Format.timestamp(entry.firstSeen) }
                        CellText { width: 165; height: parent.height; label: Format.timestamp(entry.lastSeen) }
                    }
                }
            }
        }
    }

    EmptyState {
        anchors.fill: parent
        visible: list.count === 0
        heading: qsTr("No remote IPs recorded")
        explanation: qsTr("Endpoints appear here when the database has conversations.")
    }
}
