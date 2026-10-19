import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Pigeon.Gui 1.0
import "pages"

ApplicationWindow {
    id: root

    width: 1500
    height: 900
    minimumWidth: 620
    minimumHeight: 490
    visible: true
    title: qsTr("Pigeon GUI")
    color: Theme.background
    palette.window: Theme.background
    palette.base: Theme.surface
    palette.button: Theme.surface
    palette.buttonText: Theme.textPrimary
    palette.text: Theme.textPrimary
    palette.highlight: Theme.accent
    palette.highlightedText: "white"

    property int currentPage: 1
    property bool darkTheme: false
    onDarkThemeChanged: Theme.dark = darkTheme
    readonly property bool trafficPage: currentPage === 1 || currentPage === 2
    readonly property bool compact: width < 940
    readonly property var titles: [qsTr("Overview"), qsTr("Live Traffic"),
                                   qsTr("Top Traffic"), qsTr("Applications"), qsTr("New Endpoints")]
    readonly property var symbols: ["▦", "∿", "▥", "▤", "◎"]

    Binding {
        target: captureController
        property: "databasePath"
        value: readerController.databasePath
    }

    Dialog {
        id: captureDialog
        objectName: "captureInterfaceDialog"
        title: qsTr("Capture source")
        anchors.centerIn: parent
        width: Math.min(root.width - 32, 580)
        height: Math.min(root.height - 32, 420)
        modal: true
        standardButtons: Dialog.Close
        onOpened: captureController.refresh()
        ColumnLayout {
            anchors.fill: parent
            spacing: 12
            Label {
                Layout.fillWidth: true
                text: captureController.status
                wrapMode: Text.Wrap
                color: Theme.textPrimary
            }
            RowLayout {
                ActionButton {
                    text: qsTr("Automatic")
                    enabled: !captureController.busy
                    onClicked: captureController.selectInterface("")
                }
                ActionButton {
                    text: qsTr("Refresh interfaces")
                    enabled: !captureController.busy
                    onClicked: captureController.refresh()
                }
                BusyIndicator { running: captureController.busy; implicitWidth: 30; implicitHeight: 30 }
            }
            ListView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                model: captureController.interfaces
                spacing: 4
                ScrollBar.vertical: ScrollBar { }
                delegate: ItemDelegate {
                    required property var modelData
                    width: ListView.view.width
                    enabled: !captureController.busy && modelData.active && modelData.supported
                    text: modelData.name + (modelData.name === captureController.currentInterface ? qsTr(" [current]") : "")
                          + " — " + modelData.state
                    onClicked: captureController.selectInterface(modelData.name)
                }
            }
            Label {
                Layout.fillWidth: true
                text: qsTr("Changes the running collector. Recorded history is preserved. Automatic selects once; it does not follow route changes.")
                wrapMode: Text.Wrap
                font.pixelSize: Theme.textPixels(11)
                color: Theme.textSecondary
            }
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.preferredWidth: 224
            Layout.fillHeight: true
            visible: !root.compact
            color: Theme.sidebar

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 20
                spacing: 0

                RowLayout {
                    spacing: 10
                    BrandMark {
                        Layout.preferredWidth: 40
                        Layout.preferredHeight: 40
                    }
                    ColumnLayout {
                        spacing: 1
                        Text {
                            text: "Pigeon"
                            font.pixelSize: Theme.textPixels(20)
                            font.weight: Font.DemiBold
                            color: Theme.textPrimary
                        }
                        Text {
                            text: qsTr("Network Viewer")
                            font.pixelSize: Theme.textPixels(10)
                            font.letterSpacing: 0.2
                            color: Theme.textMuted
                        }
                    }
                }

                Item { Layout.preferredHeight: 26 }
                Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }
                Item { Layout.preferredHeight: 22 }
                Text {
                    text: qsTr("WORKSPACE")
                    font.pixelSize: Theme.textPixels(11)
                    font.letterSpacing: 1
                    color: Theme.textMuted
                }
                Item { Layout.preferredHeight: 10 }
                Repeater {
                    model: 5
                    NavTab {
                        required property int index
                        Layout.fillWidth: true
                        Layout.preferredHeight: 44
                        text: root.titles[index]
                        symbol: root.symbols[index]
                        selected: root.currentPage === index
                        onClicked: root.currentPage = index
                    }
                }
                Item { Layout.fillHeight: true }
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 1
                    color: Theme.border
                }
                Item { Layout.preferredHeight: 15 }
                Text {
                    Layout.fillWidth: true
                    text: qsTr("SQLite · read-only")
                    font.pixelSize: Theme.textPixels(11)
                    color: Theme.textSecondary
                }
                Text {
                    Layout.fillWidth: true
                    text: qsTr("Collector runs independently")
                    font.pixelSize: Theme.textPixels(11)
                    color: Theme.textMuted
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumWidth: 0
            spacing: 0

            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: root.compact ? 18 : 28
                Layout.rightMargin: root.compact ? 18 : 28
                Layout.topMargin: 14
                Layout.bottomMargin: 5
                spacing: 5

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2
                        Text {
                            visible: root.compact
                            text: qsTr("PIGEON")
                            font.pixelSize: Theme.textPixels(11)
                            font.bold: true
                            font.letterSpacing: 1.4
                            color: Theme.accent
                        }
                        Text {
                            text: root.titles[root.currentPage]
                            font.pixelSize: Theme.textPixels(root.compact ? 23 : 27)
                            font.weight: Font.DemiBold
                            color: Theme.textPrimary
                        }
                    }
                    ActionButton {
                        text: qsTr("Interfaces")
                        Accessible.name: qsTr("Choose collector capture interface")
                        onClicked: captureDialog.open()
                    }
                    ActionButton {
                        text: readerController.paused ? qsTr("Resume") : qsTr("Pause")
                        enabled: readerController.hasSnapshot
                        Accessible.name: readerController.paused ? qsTr("Resume automatic database refresh")
                                                               : qsTr("Pause automatic database refresh")
                        onClicked: readerController.setPaused(!readerController.paused)
                    }
                    Rectangle {
                        width: 106
                        height: 30
                        radius: 15
                        color: Theme.accentSurface
                        Text {
                            anchors.centerIn: parent
                            text: readerController.error.length > 0 ? qsTr("Read failed")
                                : readerController.paused ? qsTr("Paused")
                                : readerController.hasSnapshot ? qsTr("Read-only") : qsTr("No data")
                            color: readerController.error.length > 0 ? Theme.danger : Theme.accent
                            font.pixelSize: Theme.textPixels(12)
                            font.weight: Font.Medium
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 9

                    Text {
                        Layout.fillWidth: true
                        visible: !root.compact
                        text: root.currentPage === 1 ? qsTr("Recent network activity captured by Pigeon")
                            : root.currentPage === 2 ? qsTr("The conversations using the most data")
                            : qsTr("Understand who your laptop speaks to")
                        color: Theme.textSecondary
                        font.pixelSize: Theme.textPixels(11)
                        elide: Text.ElideRight
                    }

                    TextField {
                        id: databasePath
                        Layout.fillWidth: root.compact
                        Layout.preferredWidth: root.compact ? 300 : Math.min(540, root.width * 0.36)
                        Layout.minimumWidth: 140
                        implicitHeight: 34
                        text: readerController.databasePath
                        placeholderText: qsTr("Existing Pigeon SQLite database path")
                        selectByMouse: true
                        color: Theme.textPrimary
                        placeholderTextColor: Theme.textMuted
                        background: Rectangle {
                            radius: 8
                            color: Theme.surface
                            border.color: databasePath.activeFocus ? Theme.accent : Theme.border
                        }
                        onAccepted: readerController.selectDatabase(text)
                        Accessible.name: qsTr("Database path")
                    }
                    ActionButton {
                        text: qsTr("Open")
                        primary: true
                        Accessible.name: qsTr("Open existing database read-only")
                        onClicked: readerController.selectDatabase(databasePath.text)
                    }
                    ActionButton {
                        text: Theme.dark ? "☀" : "☾"
                        Accessible.name: Theme.dark ? qsTr("Use light theme") : qsTr("Use dark theme")
                        onClicked: root.darkTheme = !root.darkTheme
                    }
                }

                Text {
                    Layout.fillWidth: true
                    Accessible.name: qsTr("Database status: %1").arg(text)
                    text: readerController.status
                    color: Theme.textMuted
                    font.pixelSize: Theme.textPixels(11)
                    elide: Text.ElideMiddle
                    ToolTip.visible: statusHover.hovered
                    ToolTip.text: text
                    HoverHandler { id: statusHover }
                }

                Rectangle {
                    Layout.fillWidth: true
                    visible: readerController.error.length > 0
                    implicitHeight: Math.max(56, errorLabel.implicitHeight + 20)
                    radius: 9
                    color: Theme.surfaceRaised
                    border.color: Theme.danger
                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 12
                        Text {
                            id: errorLabel
                            Layout.fillWidth: true
                            color: Theme.danger
                            font.pixelSize: Theme.textPixels(12)
                            wrapMode: Text.WrapAnywhere
                            text: readerController.error
                            Accessible.name: qsTr("Database read error: %1").arg(text)
                        }
                        Button {
                            text: qsTr("Retry")
                            Accessible.name: qsTr("Retry reading the selected database")
                            onClicked: readerController.retryRead()
                        }
                    }
                }

                // Keep five complete touch/focus targets at narrow widths.
                // Horizontal scrolling also works with a trackpad or keyboard.
                Flickable {
                    id: compactNavigation
                    Layout.fillWidth: true
                    Layout.preferredHeight: 48
                    visible: root.compact
                    clip: true
                    contentWidth: compactTabs.width
                    contentHeight: height
                    flickableDirection: Flickable.HorizontalFlick
                    boundsBehavior: Flickable.StopAtBounds
                    ScrollBar.horizontal: ScrollBar { policy: ScrollBar.AsNeeded }

                    Row {
                        id: compactTabs
                        spacing: 6
                        Repeater {
                            model: 5
                            NavTab {
                                required property int index
                                width: 136
                                height: 44
                                compact: true
                                text: root.titles[index]
                                symbol: ""
                                selected: root.currentPage === index
                                onClicked: root.currentPage = index
                                onActiveFocusChanged: {
                                    if (activeFocus)
                                        compactNavigation.contentX = Math.max(0,
                                            Math.min(x - 8, compactNavigation.contentWidth - compactNavigation.width))
                                }
                            }
                        }
                    }
                }
            }

            FilterBar {
                Layout.fillWidth: true
                Layout.leftMargin: root.compact ? 14 : 24
                Layout.rightMargin: root.compact ? 14 : 24
                Layout.topMargin: 8
                Layout.bottomMargin: 4
                visible: readerController.hasSnapshot && root.currentPage >= 3
                page: root.currentPage
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 0
                color: Theme.border
            }

            StackLayout {
                id: body
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumWidth: 0
                Layout.leftMargin: root.compact ? 14 : 24
                Layout.rightMargin: root.compact ? 14 : 24
                Layout.topMargin: 12
                visible: readerController.hasSnapshot
                currentIndex: root.currentPage
                OverviewPage { onConversationActivated: detailsDrawer.open() }
                TrafficPage {
                    live: true
                    onConversationActivated: detailsDrawer.open()
                }
                TrafficPage {
                    live: false
                    onConversationActivated: detailsDrawer.open()
                }
                ApplicationsPage {
                    onApplicationActivated: function(name) {
                        readerController.selectApplication(name)
                        root.currentPage = 2
                    }
                }
                EndpointsPage { }
            }

            EmptyState {
                Layout.fillWidth: true
                Layout.fillHeight: true
                visible: !readerController.hasSnapshot
                heading: qsTr("Open a Pigeon database")
                explanation: qsTr("Select an existing schema-v4 SQLite file to view network activity. No capture privileges are needed.")
            }
        }
    }

    ConversationDetails {
        id: detailsDrawer
    }
}
