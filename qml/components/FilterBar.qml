import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Pigeon.Gui 1.0

SurfaceCard {
    id: bar
    property int page: 1 // 1 Live, 2 Top, 3 Applications, 4 New Endpoints
    implicitHeight: filters.implicitHeight + 26
    Layout.fillWidth: true

    ColumnLayout {
        id: filters
        anchors.fill: parent
        anchors.margins: 13
        spacing: 10

        RowLayout {
            Layout.fillWidth: true
            spacing: 12
            Text {
                Layout.fillWidth: true
                text: bar.page === 3 ? qsTr("Applications: unfiltered totals across all conversations")
                     : bar.page === 4 ? qsTr("New Endpoints: name/IP search and address family only · global first-seen")
                     : qsTr("Live / Top: text, protocol, IP family and exact application · 200-row limit")
                wrapMode: Text.WordWrap
                color: Theme.textSecondary
                font.pixelSize: 11
            }
            Button {
                text: qsTr("Clear filters")
                Accessible.name: qsTr("Clear all filters and restore default sorts")
                onClicked: {
                    searchDebounce.stop()
                    searchField.text = ""
                    readerController.clearFilters()
                }
            }
        }

        Flow {
            Layout.fillWidth: true
            // Flow computes its implicitHeight internally; let ColumnLayout
            // allocate enough space for all wrapped rows instead.
            Layout.preferredHeight: childrenRect.height
            spacing: 10
            visible: bar.page !== 3

            TextField {
                id: searchField
                width: Math.min(270, Math.max(150, filters.width - 18))
                height: 38
                placeholderText: bar.page === 4 ? qsTr("Search name or IP")
                                  : qsTr("Search app, domain or IP")
                selectByMouse: true
                color: Theme.textPrimary
                placeholderTextColor: Theme.textMuted
                maximumLength: 127
                text: readerController.search
                background: Rectangle {
                    color: Theme.surfaceRaised
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

            ScopeCombo {
                caption: qsTr("Address family")
                choices: [qsTr("All addresses"), qsTr("IPv4"), qsTr("IPv6")]
                selectedIndex: readerController.family === 4 ? 1 : readerController.family === 6 ? 2 : 0
                onChosen: function(index) { readerController.setFamily(index === 1 ? 4 : index === 2 ? 6 : 0) }
            }
            ScopeCombo {
                visible: bar.page === 1 || bar.page === 2
                caption: qsTr("Protocol")
                choices: [qsTr("TCP + UDP"), qsTr("TCP"), qsTr("UDP")]
                selectedIndex: readerController.protocol === 6 ? 1 : readerController.protocol === 17 ? 2 : 0
                onChosen: function(index) { readerController.setProtocol(index === 1 ? 6 : index === 2 ? 17 : 0) }
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
                text: qsTr("Exact application:")
                color: Theme.textSecondary
                font.pixelSize: 12
            }
            Text {
                Layout.fillWidth: true
                text: readerController.selectedApplication.length ? readerController.selectedApplication : qsTr("Unresolved (empty name)")
                textFormat: Text.PlainText
                elide: Text.ElideRight
                color: Theme.accent
                font.pixelSize: 12
            }
            Button {
                text: qsTr("Remove application")
                Accessible.name: qsTr("Show all applications in conversation lists")
                onClicked: readerController.clearApplication()
            }
        }
        Text {
            Layout.fillWidth: true
            visible: readerController.paused
            text: qsTr("Paused · filter changes apply when you resume")
            font.pixelSize: 11
            color: Theme.textSecondary
        }
    }
}
