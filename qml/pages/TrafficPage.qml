import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Pigeon.Gui 1.0

Item {
    id: page
    property bool live: true

    Flickable {
        id: scroll
        anchors.fill: parent
        clip: true
        contentWidth: width
        contentHeight: layout.implicitHeight + 42
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
        ColumnLayout {
            id: layout
            x: 3
            y: 4
            width: Math.max(0, scroll.width - 14)
            spacing: 18

            SurfaceCard {
                Layout.fillWidth: true
                Layout.preferredHeight: page.live ? 96 : 92
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 18
                    spacing: 6
                    Text {
                        Layout.fillWidth: true
                        text: page.live ? qsTr("Last 30 seconds · newest 200 matches")
                                        : qsTr("All recorded data · top 200 by cumulative total")
                        font.pixelSize: 14
                        font.weight: Font.DemiBold
                        color: Theme.textPrimary
                    }
                    Text {
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        text: page.live
                            ? qsTr("Estimated rates require a second sample at least one second later. An empty list does not mean the collector has stopped.")
                            : qsTr("Downloaded and uploaded values are lifetime transport-payload byte totals, not traffic within a recent time window.")
                        font.pixelSize: 12
                        color: Theme.textSecondary
                    }
                }
            }

            ConversationTable {
                Layout.fillWidth: true
                Layout.preferredHeight: Math.max(440, page.height - 175)
                sourceModel: page.live ? readerController.liveModel : readerController.topModel
                live: page.live
                emptyDetail: page.live
                    ? qsTr("No recent packets in the 30-second window. Check Top Traffic for historical conversations.")
                    : qsTr("No stored conversations yet. Open a database containing collector observations.")
            }

            Text {
                Layout.fillWidth: true
                text: qsTr("Observed DNS names are historical associations, not verified endpoint ownership. Arrows indicate seen packet directions.")
                font.pixelSize: 11
                color: Theme.textMuted
                wrapMode: Text.WordWrap
            }
        }
    }
}
