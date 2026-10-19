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

    readonly property int tableWidth: 1110 + (live ? 100 : 0)
    readonly property real domainWidth: 220 + Math.max(0, horizontal.contentWidth - tableWidth) * 0.6
    readonly property real ipWidth: 190 + Math.max(0, horizontal.contentWidth - tableWidth) * 0.4
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
            visible: list.count > 0
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
                    height: Math.max(34, Theme.textPixels(28))
                    color: Theme.surfaceRaised
                    radius: 12
                    Row {
                        anchors.fill: parent
                        CellText { width: 150; height: parent.height; label: qsTr("APPLICATION"); ink: Theme.textMuted }
                        CellText { width: table.domainWidth; height: parent.height; label: qsTr("DOMAIN"); ink: Theme.textMuted }
                        CellText { width: table.ipWidth; height: parent.height; label: qsTr("IP"); ink: Theme.textMuted }
                        CellText { width: 55; height: parent.height; label: qsTr("PORT"); ink: Theme.textMuted }
                        CellText { width: 75; height: parent.height; label: qsTr("PROTOCOL"); ink: Theme.textMuted }
                        CellText { width: 115; height: parent.height; label: table.live ? qsTr("DOWN / SEC") : qsTr("DOWNLOADED"); ink: Theme.textMuted; rightAligned: true }
                        CellText { width: 115; height: parent.height; label: table.live ? qsTr("UP / SEC") : qsTr("UPLOADED"); ink: Theme.textMuted; rightAligned: true }
                        CellText { width: 85; height: parent.height; label: qsTr("DIRECTION"); ink: Theme.textMuted }
                        CellText { visible: table.live; width: visible ? 100 : 0; height: parent.height; label: qsTr("RECENT RATE"); ink: Theme.textMuted }
                        CellText { width: 105; height: parent.height; label: qsTr("LAST SEEN"); ink: Theme.textMuted }
                    }
                }

                ListView {
                    id: list
                    width: parent.width
                    height: parent.height - Math.max(34, Theme.textPixels(28))
                    clip: true
                    model: table.sourceModel
                    activeFocusOnTab: true
                Accessible.name: qsTr("Conversations list")
                Accessible.description: qsTr("Columns: application, domain, IP, port, protocol, download, upload, direction, recent rate, last seen. Use Up and Down to select a row, Left and Right to scroll, and Enter to open details.")
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
                        required property var rateHistory
                        required property real rateUp
                        required property real lastSeen
                        required property int index

                        width: list.width
                        height: Math.max(39, Theme.textPixels(33))
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
                            ApplicationCell {
                                width: 150
                                height: parent.height
                                application: row.application
                                domain: row.remoteName
                            }
                            CellText {
                                width: table.domainWidth; height: parent.height
                                label: row.remoteName.length ? row.remoteName : qsTr("Not observed")
                                fullText: row.remoteName.length
                                    ? row.remoteName + "\n" + qsTr("Observed DNS association, not verified ownership")
                                    : qsTr("No observed DNS association")
                            }
                            CellText {
                                width: table.ipWidth; height: parent.height
                                label: row.remoteIp
                                fullText: row.remoteIp + "\n" + qsTr("Remote port: %1").arg(row.remotePort)
                            }
                            CellText { width: 55; height: parent.height; label: String(row.remotePort) }
                            CellText { width: 75; height: parent.height; label: Format.protocol(row.protocol) }
                            CellText {
                                width: 115; height: parent.height; rightAligned: true; ink: Theme.download
                                label: table.live ? (row.ratesValid ? Format.speed(row.rateDown, true) : qsTr("Waiting"))
                                                  : Format.exactSize(row.bytesDownExact)
                                fullText: table.live ? (row.ratesValid ? label : qsTr("Waiting for a second sample ≥ 1 second later"))
                                                     : row.bytesDownExact + " B"
                            }
                            CellText {
                                width: 115; height: parent.height; rightAligned: true; ink: Theme.upload
                                label: table.live ? (row.ratesValid ? Format.speed(row.rateUp, true) : qsTr("Waiting"))
                                                  : Format.exactSize(row.bytesUpExact)
                                fullText: table.live ? (row.ratesValid ? label : qsTr("Waiting for a second sample ≥ 1 second later"))
                                                     : row.bytesUpExact + " B"
                            }
                            Item {
                                width: 85; height: parent.height
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
                            Sparkline {
                                visible: table.live
                                width: visible ? 100 : 0
                                height: parent.height
                                samples: row.rateHistory
                            }
                            CellText {
                                width: 105; height: parent.height
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
            Layout.preferredHeight: 180
            visible: list.count === 0
            heading: table.emptyTitle
            explanation: table.emptyDetail
        }
    }
}
