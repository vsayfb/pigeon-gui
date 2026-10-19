import QtQuick
import QtQuick.Controls
import Pigeon.Gui 1.0

Item {
    id: cell
    property string application: ""
    property string domain: ""
    readonly property var iconInfo: appIcons.resolve(application, domain)

    Image {
        id: icon
        anchors.left: parent.left
        anchors.leftMargin: 12
        anchors.verticalCenter: parent.verticalCenter
        width: 21
        height: 21
        sourceSize: Qt.size(32, 32)
        source: cell.iconInfo.url
        fillMode: Image.PreserveAspectFit
    }

    Text {
        anchors.left: icon.right
        anchors.leftMargin: 10
        anchors.right: parent.right
        anchors.rightMargin: 8
        anchors.verticalCenter: parent.verticalCenter
        text: cell.application.length ? cell.application : qsTr("Unresolved")
        textFormat: Text.PlainText
        color: Theme.textPrimary
        font.pixelSize: Theme.textPixels(12)
        font.weight: Font.DemiBold
        elide: Text.ElideRight
    }

    HoverHandler { id: hover }
    ToolTip.visible: hover.hovered
    ToolTip.text: (application.length ? application : qsTr("Application not attributed by the collector"))
        + (iconInfo.site ? "\n" + qsTr("Site icon matched from %1; does not identify the application").arg(domain) : "")
}
