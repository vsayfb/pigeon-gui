import QtQuick
import QtQuick.Layouts
import Pigeon.Gui 1.0

Item {
    id: empty
    property string heading: qsTr("Nothing to show")
    property string explanation: ""
    implicitHeight: 210

    ColumnLayout {
        anchors.centerIn: parent
        width: Math.min(empty.width - 40, 430)
        spacing: 10
        Text {
            Layout.alignment: Qt.AlignHCenter
            text: "○"
            color: Theme.accent
            font.pixelSize: Theme.textPixels(32)
        }
        Text {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            text: empty.heading
            font.pixelSize: Theme.textPixels(17)
            font.weight: Font.DemiBold
            color: Theme.textPrimary
        }
        Text {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            text: empty.explanation
            font.pixelSize: Theme.textPixels(12)
            color: Theme.textSecondary
        }
    }
}
