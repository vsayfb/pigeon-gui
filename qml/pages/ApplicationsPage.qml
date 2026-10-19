import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Pigeon.Gui 1.0

Item {
    id: page
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
                        text: qsTr("Application ranking · top 200 by total bytes")
                        font.pixelSize: 14
                        font.weight: Font.DemiBold
                        color: Theme.textPrimary
                    }
                    Text {
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        text: qsTr("Aggregates are computed over all stored conversations before the display limit. Totals are approximate; the empty application name appears as Unresolved.")
                        font.pixelSize: 12
                        color: Theme.textSecondary
                    }
                }
            }

            ApplicationsTable {
                Layout.fillWidth: true
                Layout.preferredHeight: Math.max(440, page.height - 175)
                sourceModel: readerController.applicationsModel
            }
            Text {
                Layout.fillWidth: true
                text: qsTr("Application drill-down and ranking controls follow in Commit 4.")
                color: Theme.textMuted
                font.pixelSize: 11
                wrapMode: Text.WordWrap
            }
        }
    }
}
