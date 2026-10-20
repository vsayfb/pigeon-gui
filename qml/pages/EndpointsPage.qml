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
                Layout.preferredHeight: 108
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 18
                    spacing: 7
                    Text {
                        Layout.fillWidth: true
                        text: qsTr("New to this database · 200 most recently first observed")
                        color: Theme.textPrimary
                        font.pixelSize: Theme.textPixels(14)
                        font.weight: Font.DemiBold
                    }
                    Text {
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        text: qsTr("Each remote IP appears once across processes, ports and local addresses. First seen means earliest recorded observation, not when an internet endpoint was created.")
                        font.pixelSize: Theme.textPixels(12)
                        color: Theme.textSecondary
                    }
                }
            }
            EndpointsTable {
                Layout.fillWidth: true
                Layout.preferredHeight: Math.max(440, page.height - 175)
                sourceModel: readerController.endpointsModel
            }
            Text {
                Layout.fillWidth: true
                text: qsTr("Names are historical observed DNS associations. Missing names are normal.")
                color: Theme.textMuted
                font.pixelSize: Theme.textPixels(11)
                wrapMode: Text.WordWrap
            }
        }
    }
}
