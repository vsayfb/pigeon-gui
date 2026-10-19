import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Pigeon.Gui 1.0
import "../theme/Format.js" as Format

Item {
    id: page

    Flickable {
        id: scroll
        anchors.fill: parent
        clip: true
        contentWidth: width
        contentHeight: layout.implicitHeight + 46
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

        ColumnLayout {
            id: layout
            x: 3
            y: 4
            width: Math.max(0, scroll.width - 14)
            spacing: 18

            Text {
                Layout.fillWidth: true
                text: qsTr("Cumulative traffic")
                font.pixelSize: 14
                font.weight: Font.DemiBold
                color: Theme.textSecondary
            }

            GridLayout {
                Layout.fillWidth: true
                columns: page.width >= 1040 ? 4 : 2
                columnSpacing: 12
                rowSpacing: 12

                MetricCard {
                    Layout.fillWidth: true
                    label: qsTr("Downloaded")
                    value: Format.exactSize(readerController.downloaded)
                    valueColor: Theme.accent
                    annotation: qsTr("All recorded payload bytes")
                    ToolTip.visible: downHover.hovered
                    ToolTip.text: readerController.downloaded + qsTr(" bytes exactly")
                    HoverHandler { id: downHover }
                }
                MetricCard {
                    Layout.fillWidth: true
                    label: qsTr("Uploaded")
                    value: Format.exactSize(readerController.uploaded)
                    valueColor: Theme.positive
                    annotation: qsTr("All recorded payload bytes")
                    ToolTip.visible: upHover.hovered
                    ToolTip.text: readerController.uploaded + qsTr(" bytes exactly")
                    HoverHandler { id: upHover }
                }
                MetricCard {
                    Layout.fillWidth: true
                    label: qsTr("Conversations")
                    value: readerController.conversationCount
                    annotation: qsTr("All stored rows")
                }
                MetricCard {
                    Layout.fillWidth: true
                    label: qsTr("Remote IPs")
                    value: readerController.remoteIpCount
                    annotation: qsTr("Distinct addresses")
                }
            }

            GridLayout {
                Layout.fillWidth: true
                columns: 2
                columnSpacing: 12
                rowSpacing: 12
                MetricCard {
                    Layout.fillWidth: true
                    label: qsTr("Current download estimate")
                    value: Format.speed(readerController.downloadRateValue, readerController.ratesMeasured)
                    valueColor: Theme.accent
                    annotation: qsTr("Viewer-session counter difference")
                }
                MetricCard {
                    Layout.fillWidth: true
                    label: qsTr("Current upload estimate")
                    value: Format.speed(readerController.uploadRateValue, readerController.ratesMeasured)
                    valueColor: Theme.positive
                    annotation: qsTr("Viewer-session counter difference")
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 10
                Text {
                    text: qsTr("Latest packet")
                    color: Theme.textSecondary
                    font.pixelSize: 12
                }
                Text {
                    Layout.fillWidth: true
                    text: readerController.lastPacketAge
                    color: Theme.textPrimary
                    font.pixelSize: 12
                }
                Text {
                    text: qsTr("Age is not collector status")
                    color: Theme.textMuted
                    font.pixelSize: 11
                }
            }

            TrafficChart {
                Layout.fillWidth: true
                Layout.preferredHeight: 305
                history: readerController.trafficHistory
            }

            RowLayout {
                Layout.fillWidth: true
                Text {
                    Layout.fillWidth: true
                    text: qsTr("Recent conversations")
                    color: Theme.textPrimary
                    font.pixelSize: 17
                    font.weight: Font.DemiBold
                }
                Text {
                    text: qsTr("30s window · newest 200 matches")
                    color: Theme.textSecondary
                    font.pixelSize: 11
                }
            }

            ConversationTable {
                Layout.fillWidth: true
                Layout.preferredHeight: 350
                sourceModel: readerController.liveModel
                live: true
                emptyDetail: qsTr("Nothing observed in the past 30 seconds. Older recordings remain in Top Traffic.")
            }

            Text {
                Layout.fillWidth: true
                text: qsTr("Rates and chart points are sampled after this viewer opens; the database contains cumulative totals only.")
                color: Theme.textMuted
                font.pixelSize: 11
                wrapMode: Text.WordWrap
            }
        }
    }
}
