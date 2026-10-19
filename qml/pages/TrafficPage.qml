import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Pigeon.Gui 1.0

Item {
    id: page
    property bool live: true
    signal conversationActivated()

    Flickable {
        id: scroll
        anchors.fill: parent
        clip: true
        contentWidth: width
        contentHeight: layout.implicitHeight + 20
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

        ColumnLayout {
            id: layout
            x: 2
            width: Math.max(0, scroll.width - 12)
            spacing: 14

            MetricsRow {
                Layout.fillWidth: true
            }

            TrafficChart {
                visible: page.live
                Layout.fillWidth: true
                Layout.preferredHeight: 245
                history: readerController.trafficHistory
            }

            FilterBar {
                Layout.fillWidth: true
                page: page.live ? 1 : 2
            }

            Text {
                Layout.fillWidth: true
                text: page.live ? qsTr("Recent conversations · newest 200 matches before rate ranking")
                    : qsTr("Top 200 conversations · all recorded protocol-payload totals")
                color: Theme.textSecondary
                font.pixelSize: Theme.textPixels(11)
            }

            ConversationTable {
                Layout.fillWidth: true
                Layout.preferredHeight: Math.max(310, page.height - (page.live ? 440 : 190))
                sourceModel: page.live ? readerController.liveModel : readerController.topModel
                live: page.live
                onConversationActivated: function(identity) {
                    readerController.selectConversation(identity, page.live)
                    page.conversationActivated()
                }
                emptyDetail: page.live
                    ? qsTr("No matching packets in this recency window. Try All recorded or Top Traffic. This does not indicate collector status.")
                    : qsTr("No stored conversations match these filters.")
            }

            Text {
                Layout.fillWidth: true
                text: qsTr("DNS names are observed associations, not verified ownership. Arrows show observed packet directions. Rates are estimated from database changes.")
                font.pixelSize: Theme.textPixels(10)
                color: Theme.textMuted
                wrapMode: Text.WordWrap
            }
        }
    }
}
