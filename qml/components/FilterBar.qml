import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Pigeon.Gui 1.0

SurfaceCard {
    id: bar
    property int page: 1 // 1 Live, 2 Top, 3 Applications, 4 New Endpoints
    implicitHeight: filters.implicitHeight + 16
    Layout.fillWidth: true
    visible: page !== 3

    ColumnLayout {
        id: filters
        anchors.fill: parent
        anchors.margins: 8
        spacing: 8

        Flow {
            Layout.fillWidth: true
            // Flow computes its implicitHeight internally; let ColumnLayout
            // allocate enough space for all wrapped rows instead.
            Layout.preferredHeight: childrenRect.height
            spacing: 8
            visible: bar.page !== 3

            TextField {
                id: searchField
                width: Math.min(300, Math.max(150, filters.width - 18))
                height: 38
                placeholderText: bar.page === 4 ? qsTr("Search name or IP")
                                  : qsTr("Search process, domain or IP")
                selectByMouse: true
                color: Theme.textPrimary
                placeholderTextColor: Theme.textMuted
                maximumLength: 127
                text: readerController.search
                background: Rectangle {
                    color: Theme.surface
                    border.color: searchField.activeFocus ? Theme.accent : Theme.border
                    radius: 7
                }
                Accessible.name: qsTr("Search traffic")
                onTextEdited: searchDebounce.restart()
                onAccepted: {
                    searchDebounce.stop()
                    readerController.setSearch(text)
                }
                Timer {
                    id: searchDebounce
                    interval: 260
                    repeat: false
                    onTriggered: readerController.setSearch(searchField.text)
                }
                Connections {
                    target: readerController
                    function onFiltersChanged() {
                        // A drill-down clears search immediately. Do not let a
                        // queued keystroke debounce restore that old search.
                        if (readerController.applicationSelected) {
                            searchDebounce.stop()
                            searchField.text = readerController.search
                        } else if (!searchDebounce.running && !searchField.activeFocus) {
                            searchField.text = readerController.search
                        }
                    }
                }
            }

            ActionButton {
                text: qsTr("Clear filters")
                onClicked: {
                    searchDebounce.stop()
                    searchField.text = ""
                    readerController.clearFilters()
                }
            }

            ScopeCombo {
                caption: qsTr("Address family")
                choices: [qsTr("All addresses"), qsTr("IPv4"), qsTr("IPv6")]
                selectedIndex: readerController.family === 4 ? 1 : readerController.family === 6 ? 2 : 0
                onChosen: function(index) { readerController.setFamily(index === 1 ? 4 : index === 2 ? 6 : 0) }
            }
            ScopeCombo {
                visible: bar.page === 1 || bar.page === 2
                caption: qsTr("Protocol")
                choices: [qsTr("All protocols"), qsTr("TCP"), qsTr("UDP"), qsTr("ICMP"), qsTr("ICMPv6"), qsTr("ESP")]
                selectedIndex: [0, 6, 17, 1, 58, 50].indexOf(readerController.protocol)
                onChosen: function(index) { readerController.setProtocol([0, 6, 17, 1, 58, 50][index]) }
            }
            ScopeCombo {
                visible: bar.page === 1
                caption: qsTr("Recency")
                choices: [qsTr("Last 30s"), qsTr("Last 10s"), qsTr("Last 120s"), qsTr("All recorded")]
                selectedIndex: readerController.recentSeconds === 10 ? 1 : readerController.recentSeconds === 120 ? 2
                               : readerController.recentSeconds === 0 ? 3 : 0
                onChosen: function(index) { readerController.setRecentSeconds([30, 10, 120, 0][index]) }
            }
            ScopeCombo {
                visible: bar.page === 1
                caption: qsTr("Live ranking")
                choices: [qsTr("Newest first"), qsTr("Highest combined rate"),
                          qsTr("Highest download rate"), qsTr("Highest upload rate")]
                selectedIndex: readerController.liveSort
                onChosen: function(index) { readerController.setLiveSort(index) }
            }
            ScopeCombo {
                visible: bar.page === 2
                caption: qsTr("Cumulative ranking")
                choices: [qsTr("Most total"), qsTr("Most downloaded"), qsTr("Most uploaded")]
                selectedIndex: readerController.rankingSort
                onChosen: function(index) { readerController.setRankingSort(index) }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            visible: readerController.applicationSelected && (bar.page === 1 || bar.page === 2)
            Text {
                text: qsTr("Exact process:")
                color: Theme.textSecondary
                font.pixelSize: Theme.textPixels(12)
            }
            Text {
                Layout.fillWidth: true
                text: readerController.selectedApplication.length ? readerController.selectedApplication : qsTr("— (process not identified)")
                textFormat: Text.PlainText
                elide: Text.ElideRight
                color: Theme.accent
                font.pixelSize: Theme.textPixels(12)
            }
            ActionButton {
                text: qsTr("Remove process")
                Accessible.name: qsTr("Show all processes in conversation lists")
                onClicked: readerController.clearApplication()
            }
        }
        Text {
            Layout.fillWidth: true
            visible: readerController.paused
            text: qsTr("Paused · filter changes apply when you resume")
            font.pixelSize: Theme.textPixels(11)
            color: Theme.textSecondary
        }
    }
}
