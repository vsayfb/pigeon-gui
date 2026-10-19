import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Pigeon.Gui 1.0

Item {
    id: page
    signal applicationActivated(string application)
    Flickable {
        id: scroll
        anchors.fill: parent
        clip: true
        contentWidth: width
        contentHeight: layout.implicitHeight + 36
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
                Layout.preferredHeight: 102
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 18
                    spacing: 7
                    Text {
                        Layout.fillWidth: true
                        text: qsTr("Application ranking · top 200 by cumulative %1").arg(
                            readerController.rankingSort === 1 ? qsTr("download") :
                            readerController.rankingSort === 2 ? qsTr("upload") : qsTr("total"))
                        font.pixelSize: Theme.textPixels(14)
                        font.weight: Font.DemiBold
                        color: Theme.textPrimary
                    }
                    Text {
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        text: qsTr("Aggregates are computed over all stored conversations before the display limit. Totals are approximate; the empty application name appears as Unresolved.")
                        font.pixelSize: Theme.textPixels(12)
                        color: Theme.textSecondary
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Text { text: qsTr("Rank by"); color: Theme.textSecondary; font.pixelSize: Theme.textPixels(12) }
                ScopeCombo {
                    choices: [qsTr("Combined bytes"), qsTr("Downloaded"), qsTr("Uploaded")]
                    selectedIndex: readerController.rankingSort
                    caption: qsTr("Application ranking")
                    onChosen: function(index) { readerController.setRankingSort(index) }
                }
            }

            ApplicationsTable {
                onApplicationActivated: function(name) { page.applicationActivated(name) }
                Layout.fillWidth: true
                Layout.preferredHeight: Math.max(440, page.height - 175)
                sourceModel: readerController.applicationsModel
            }
            Text {
                Layout.fillWidth: true
                text: qsTr("Select an application to show only its conversations in Top Traffic. Unresolved is an exact empty-name filter.")
                color: Theme.textMuted
                font.pixelSize: Theme.textPixels(11)
                wrapMode: Text.WordWrap
            }
        }
    }
}
