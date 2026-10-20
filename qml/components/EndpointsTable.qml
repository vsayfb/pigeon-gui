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
                height: Math.max(45, Theme.textPixels(40))
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
                height: parent.height - Math.max(45, Theme.textPixels(40))
                clip: true
                model: table.sourceModel
                activeFocusOnTab: true
                Accessible.name: qsTr("New endpoints list")
                Accessible.description: qsTr("Use Up and Down to navigate rows, Left and Right to scroll columns")
                Keys.onLeftPressed: horizontal.contentX = Math.max(0, horizontal.contentX - 120)
                Keys.onRightPressed: horizontal.contentX = Math.min(
                    horizontal.contentWidth - horizontal.width, horizontal.contentX + 120)
                highlightMoveDuration: 0
                highlight: Rectangle {
                    visible: list.activeFocus && list.count > 0
                    color: "transparent"
                    border.width: 2
                    border.color: Theme.accent
                    radius: 6
                }
                boundsBehavior: Flickable.StopAtBounds
                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                delegate: Rectangle {
                    id: entry
                    required property string ip
                    readonly property string addressScope: readerController.addressScope(ip)
                    required property string name
                    required property real firstSeen
                    required property real lastSeen
                    required property int index
                    width: list.width; height: Math.max(53, Theme.textPixels(47))
                    color: index % 2 ? Theme.rowAlternate : Theme.surface
                    Row {
                        anchors.fill: parent
                        CellText { width: 295; height: parent.height; label: entry.ip; prominent: true; ink: Theme.textPrimary }
                        CellText { width: 270; height: parent.height; label: readerController.endpointName(entry.name, entry.ip)
                            fullText: entry.name || (entry.addressScope ? entry.addressScope + "\n" + qsTr("Address-range label, not an observed DNS name or proof of the same subnet.") : qsTr("No observed DNS association")) }
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
