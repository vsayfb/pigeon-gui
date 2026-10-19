import QtQuick
import QtQuick.Layouts
import Pigeon.Gui 1.0
import "../theme/Format.js" as Format

GridLayout {
    columns: width >= 1050 ? 5 : width >= 650 ? 3 : 2
    columnSpacing: 12
    rowSpacing: 12

    MetricCard {
        Layout.fillWidth: true
        Layout.preferredWidth: 1
        label: qsTr("Total conversations")
        value: readerController.conversationCount
        annotation: qsTr("All recorded data")
        symbol: "☷"
    }

    MetricCard {
        Layout.fillWidth: true
        Layout.preferredWidth: 1
        label: qsTr("Remote IPs")
        value: readerController.remoteIpCount
        symbol: "◎"
    }

    MetricCard {
        Layout.fillWidth: true
        Layout.preferredWidth: 1
        label: qsTr("Downloaded payload")
        value: Format.exactSize(readerController.downloaded)
        symbol: "↓"
    }

    MetricCard {
        Layout.fillWidth: true
        Layout.preferredWidth: 1
        label: qsTr("Inbound traffic")
        value: readerController.ratesMeasured ? Format.speed(readerController.downloadRateValue, true) : qsTr("Sampling…")
        valueColor: Theme.download
        symbol: "↓"
    }

    MetricCard {
        Layout.fillWidth: true
        Layout.preferredWidth: 1
        label: qsTr("Outbound traffic")
        value: readerController.ratesMeasured ? Format.speed(readerController.uploadRateValue, true) : qsTr("Sampling…")
        valueColor: Theme.upload
        symbol: "↑"
    }
}
