import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Pigeon.Gui 1.0
import "../theme/Format.js" as Format

SurfaceCard {
    id: table
    property var sourceModel: null
    signal conversationActivated(string identity)
    property bool live: true
    property string emptyTitle: qsTr("No matching conversations")
    property string emptyDetail: qsTr("No rows match the current scope.")

    readonly property int tableWidth: 1200
    implicitHeight: 420
    clip: true

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 1
        spacing: 0

        Flickable {
            id: horizontal
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            contentWidth: Math.max(width, table.tableWidth)
            contentHeight: height
            flickableDirection: Flickable.HorizontalFlick
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.horizontal: ScrollBar { policy: ScrollBar.AsNeeded }

            Column {
                width: horizontal.contentWidth
                height: horizontal.height
                spacing: 0

                Rectangle {
                    width: parent.width
                    height: Math.max(45, Theme.textPixels(40))
                    color: Theme.surfaceRaised
                    radius: 12
                    Row {
                        anchors.fill: parent
                        CellText { width: 133; height: parent.height; label: qsTr("APPLICATION"); ink: Theme.textMuted }
                        CellText { width: 245; height: parent.height; label: qsTr("DOMAIN"); ink: Theme.textMuted }
                        CellText { width: 235; height: parent.height; label: qsTr("IP"); ink: Theme.textMuted }
                        CellText { width: 82; height: parent.height; label: qsTr("PROTOCOL"); ink: Theme.textMuted }
                        CellText { width: 120; height: parent.height; label: table.live ? qsTr("DOWN / SEC") : qsTr("DOWNLOADED"); ink: Theme.textMuted; rightAligned: true }
                        CellText { width: 120; height: parent.height; label: table.live ? qsTr("UP / SEC") : qsTr("UPLOADED"); ink: Theme.textMuted; rightAligned: true }
                        CellText { width: 90; height: parent.height; label: qsTr("DIRECTION"); ink: Theme.textMuted }
                        CellText { width: 175; height: parent.height; label: qsTr("LAST SEEN"); ink: Theme.textMuted }
                    }
                }

                ListView {
                    id: list
                    width: parent.width
                    height: parent.height - Math.max(45, Theme.textPixels(40))
                    clip: true
                    model: table.sourceModel
                    activeFocusOnTab: true
                Accessible.name: qsTr("Conversations list")
                Accessible.description: qsTr("Columns: application, domain, IP, protocol, download, upload, direction, last seen. Use Up and Down to select a row, Left and Right to scroll, and Enter to open details.")
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
                Keys.onReturnPressed: {
                        if (list.currentItem)
                            table.conversationActivated(list.currentItem.identity)
                    }
                    Keys.onSpacePressed: {
                        if (list.currentItem)
                            table.conversationActivated(list.currentItem.identity)
                    }
                    boundsBehavior: Flickable.StopAtBounds
                    ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                    delegate: Rectangle {
                        id: row
                        required property string identity
                        required property string application
                        required property string remoteIp
                        required property string remoteName
                        required property int remotePort
                        required property int protocol
                        required property string bytesDownExact
                        required property string bytesUpExact
                        required property var packetsDown
                        required property var packetsUp
                        required property bool ratesValid
                        required property real rateDown
                        required property real rateUp
                        required property real lastSeen
                        required property int index

                        width: list.width
                        height: Math.max(53, Theme.textPixels(47))
                        color: readerController.selectedIdentity === row.identity
                            ? Theme.accentSurface : index % 2 ? Theme.rowAlternate : Theme.surface
                        border.width: rowHover.hovered || (list.activeFocus && list.currentIndex === index) ? 2 : 0
                        border.color: Theme.accent
                        HoverHandler { id: rowHover }
                        TapHandler {
                            onTapped: {
                                list.currentIndex = row.index
                                list.forceActiveFocus()
                                table.conversationActivated(row.identity)
                            }
                        }
                        Row {
                            height: parent.height
                            width: parent.width
                            CellText {
                                width: 133; height: parent.height
                                label: row.application.length ? row.application : qsTr("Unresolved")
                                ink: Theme.textPrimary; prominent: true
                            }
                            CellText {
                                width: 245; height: parent.height
                                label: row.remoteName.length ? row.remoteName : qsTr("Not observed")
                                fullText: row.remoteName.length
                                    ? row.remoteName + "\n" + qsTr("Observed DNS association, not verified ownership")
                                    : qsTr("No observed DNS association")
                            }
                            CellText {
                                width: 235; height: parent.height
                                label: row.remoteIp
                                fullText: row.remoteIp + "\n" + qsTr("Remote port: %1").arg(row.remotePort)
                            }
                            CellText { width: 82; height: parent.height; label: Format.protocol(row.protocol) }
                            CellText {
                                width: 120; height: parent.height; rightAligned: true; ink: Theme.accent
                                label: table.live ? (row.ratesValid ? Format.speed(row.rateDown, true) : qsTr("Waiting"))
                                                  : Format.exactSize(row.bytesDownExact)
                                fullText: table.live ? (row.ratesValid ? label : qsTr("Waiting for a second sample ≥ 1 second later"))
                                                     : row.bytesDownExact + " B"
                            }
                            CellText {
                                width: 120; height: parent.height; rightAligned: true; ink: Theme.positive
                                label: table.live ? (row.ratesValid ? Format.speed(row.rateUp, true) : qsTr("Waiting"))
                                                  : Format.exactSize(row.bytesUpExact)
                                fullText: table.live ? (row.ratesValid ? label : qsTr("Waiting for a second sample ≥ 1 second later"))
                                                     : row.bytesUpExact + " B"
                            }
                            Item {
                                width: 90; height: parent.height
                                Row {
                                    anchors.centerIn: parent
                                    spacing: 10
                                    DirectionIcon { incoming: true; observed: Number(row.packetsDown) > 0 }
                                    DirectionIcon { incoming: false; observed: Number(row.packetsUp) > 0 }
                                }
                                ToolTip.visible: hover.hovered
                                ToolTip.delay: 600
                                ToolTip.text: qsTr("Arrows indicate observed packet direction, not connection initiation")
                                HoverHandler { id: hover }
                            }
                            CellText {
                                width: 175; height: parent.height
                                label: Format.relative(row.lastSeen)
                                fullText: Format.timestamp(row.lastSeen)
                            }
                        }
                    }
                }
            }
        }

        EmptyState {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: list.count === 0
            heading: table.emptyTitle
            explanation: table.emptyDetail
        }
    }
}
