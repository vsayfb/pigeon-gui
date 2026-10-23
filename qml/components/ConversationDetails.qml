import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Pigeon.Gui 1.0
import "../theme/Format.js" as Format

Drawer {
    id: inspector
    signal deleteRequested(string identity)
    edge: Qt.RightEdge
    modal: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    width: Math.min(450, parent ? parent.width * 0.9 : 400)
    height: parent ? parent.height : 780

    background: Rectangle { color: Theme.sidebar; border.color: Theme.border }

    ScrollView {
        anchors.fill: parent
        anchors.margins: 22
        clip: true

        ColumnLayout {
            width: inspector.availableWidth - 44
            spacing: 12
            readonly property var details: readerController.selectedDetails

            RowLayout {
                Layout.fillWidth: true
                Text {
                    Layout.fillWidth: true
                    text: qsTr("Conversation details")
                    color: Theme.textPrimary
                    font.pixelSize: Theme.textPixels(20)
                    font.weight: Font.DemiBold
                }
                Button {
                    id: closeDetailsButton
                    text: qsTr("Close")
                    Accessible.name: qsTr("Close conversation details")
                    onClicked: inspector.close()
                }
            }
            ActionButton {
                text: qsTr("Delete conversation…")
                destructive: true
                enabled: !settingsController.busy && readerController.selectedIdentity.length > 0
                onClicked: {
                    inspector.deleteRequested(readerController.selectedIdentity)
                    inspector.close()
                }
            }
            Text {
                Layout.fillWidth: true
                visible: !parent.details.inCurrentView
                text: qsTr("This conversation is outside the current list. Showing its last selected details.")
                color: Theme.textSecondary
                wrapMode: Text.WordWrap
                font.pixelSize: Theme.textPixels(12)
            }
            Text {
                Layout.fillWidth: true
                text: parent.details.application || qsTr("No selected conversation")
                textFormat: Text.PlainText
                font.pixelSize: Theme.textPixels(17)
                color: Theme.accent
                font.weight: Font.DemiBold
                wrapMode: Text.WrapAnywhere
            }
            Text {
                Layout.fillWidth: true
                text: qsTr("Observed DNS association, not verified service ownership")
                color: Theme.textSecondary
                wrapMode: Text.WordWrap
                font.pixelSize: Theme.textPixels(12)
            }
            Repeater {
                model: [
                    [qsTr("Endpoint name"), readerController.endpointName(parent.details.remoteName || "", parent.details.remoteIp || "")],
                    [qsTr("Address information"), readerController.addressScope(parent.details.remoteIp || "") || "—"],
                    [qsTr("Remote IP"), parent.details.remoteIp || "—"],
                    [qsTr("Local IP"), parent.details.localIp || "—"],
                    [qsTr("Remote port"), !Format.hasPorts(parent.details.protocol) || parent.details.remotePort === undefined ? "—" : String(parent.details.remotePort)],
                    [qsTr("Protocol"), Format.protocol(parent.details.protocol, parent.details.quic)],
                    [qsTr("Downloaded payload"), (parent.details.bytesDown || "0") + " B"],
                    [qsTr("Uploaded payload"), (parent.details.bytesUp || "0") + " B"],
                    [qsTr("Incoming packets"), parent.details.packetsDown || "0"],
                    [qsTr("Outgoing packets"), parent.details.packetsUp || "0"],
                    [qsTr("First observed"), Format.timestamp(parent.details.firstSeen)],
                    [qsTr("Last observed"), Format.timestamp(parent.details.lastSeen)],
                    [qsTr("Current download"), parent.details.ratesValid ? Format.speed(parent.details.rateDown, true) : qsTr("Waiting / unavailable")],
                    [qsTr("Current upload"), parent.details.ratesValid ? Format.speed(parent.details.rateUp, true) : qsTr("Waiting / unavailable")]
                ]
                delegate: ColumnLayout {
                    required property var modelData
                    Layout.fillWidth: true
                    spacing: 3
                    Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.border }
                    Text { text: modelData[0]; color: Theme.textMuted; font.pixelSize: Theme.textPixels(11) }
                    TextEdit {
                        Layout.fillWidth: true
                        Layout.preferredHeight: contentHeight
                        text: String(modelData[1]); color: Theme.textPrimary
                        wrapMode: TextEdit.WrapAnywhere
                        font.pixelSize: Theme.textPixels(13)
                        textFormat: TextEdit.PlainText
                        readOnly: true
                        selectByMouse: true
                        selectByKeyboard: true
                    }
                }
            }
        }
    }
    onOpened: closeDetailsButton.forceActiveFocus()
    onClosed: readerController.clearSelection()
}
