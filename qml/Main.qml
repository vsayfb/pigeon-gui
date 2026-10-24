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
    title: qsTr("Pigeon")
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
                                   qsTr("Top Traffic"), qsTr("Processes"), qsTr("New Endpoints")]
    readonly property var symbols: ["▦", "∿", "▥", "▤", "◎"]

    function confirmConversationDeletion(identities) {
        historyConfirm.clearAll = false
        historyConfirm.identities = identities.slice()
        historyConfirm.conversation = ""
        historyConfirm.database = readerController.databasePath
        historyConfirm.open()
    }

    function navigateTo(page) {
        // A process drill-down belongs to the view opened from Processes.
        // Explicit navigation returns to all processes, keeping other filters.
        readerController.clearApplication()
        root.currentPage = page
    }

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

    Dialog {
        id: historyConfirm
        objectName: "deleteHistoryDialog"
        property string conversation: ""
        property bool clearAll: false
        property var identities: []
        property string database: ""
        title: clearAll ? qsTr("Clear all history?") : identities.length > 1
            ? qsTr("Delete %1 conversations?").arg(identities.length) : qsTr("Delete conversation?")
        anchors.centerIn: parent
        width: Math.min(root.width - 32, 500)
        modal: true
        padding: 24
        closePolicy: Popup.CloseOnEscape
        background: Rectangle {
            color: Theme.surface
            radius: 16
            border.color: Theme.border
        }
        header: Label {
            text: historyConfirm.title
            leftPadding: 24
            rightPadding: 24
            topPadding: 24
            bottomPadding: 0
            color: Theme.textPrimary
            font.pixelSize: Theme.textPixels(22)
            font.bold: true
        }
        footer: Item {
            implicitHeight: 76
            Row {
                anchors.right: parent.right
                anchors.rightMargin: 24
                anchors.verticalCenter: parent.verticalCenter
                spacing: 10
                ActionButton {
                    text: qsTr("Cancel")
                    focus: true
                    onClicked: historyConfirm.reject()
                }
                ActionButton {
                    text: historyConfirm.clearAll ? qsTr("Clear history") : historyConfirm.identities.length > 1
                        ? qsTr("Delete %1 conversations").arg(historyConfirm.identities.length) : qsTr("Delete conversation")
                    destructive: true
                    enabled: !settingsController.busy
                    onClicked: historyConfirm.accept()
                }
            }
        }
        onAccepted: {
            if (clearAll)
                settingsController.clearHistory(database)
            else
                settingsController.deleteConversations(identities, database)
        }
        contentItem: ColumnLayout {
            spacing: 18
            Label {
                Layout.fillWidth: true
                text: historyConfirm.clearAll
                    ? qsTr("All conversations and saved DNS associations will be permanently deleted.")
                    : historyConfirm.identities.length > 1
                    ? qsTr("These %1 selected conversations will be permanently deleted.").arg(historyConfirm.identities.length)
                    : qsTr("This recorded conversation will be permanently deleted.")
                wrapMode: Text.Wrap
                color: Theme.textSecondary
                font.pixelSize: Theme.textPixels(13)
            }
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: warningText.implicitHeight + 24
                radius: 8
                color: Theme.dark ? "#3B242C" : "#FFF0F2"
                Label {
                    id: warningText
                    anchors.fill: parent
                    anchors.margins: 12
                    text: qsTr("This cannot be undone. New traffic will still be recorded.")
                    wrapMode: Text.Wrap
                    color: Theme.danger
                    font.pixelSize: Theme.textPixels(12)
                    font.weight: Font.Medium
                }
            }
            Label {
                Layout.fillWidth: true
                visible: !historyConfirm.clearAll && historyConfirm.conversation.length > 0
                text: historyConfirm.conversation
                textFormat: Text.PlainText
                wrapMode: Text.WrapAnywhere
                color: Theme.textPrimary
                font.pixelSize: Theme.textPixels(13)
                font.bold: true
            }
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 5
                Label {
                    text: qsTr("DATABASE")
                    color: Theme.textMuted
                    font.pixelSize: Theme.textPixels(10)
                    font.weight: Font.Medium
                }
                Label {
                    Layout.fillWidth: true
                    text: historyConfirm.database
                    textFormat: Text.PlainText
                    wrapMode: Text.WrapAnywhere
                    color: Theme.textSecondary
                    font.pixelSize: Theme.textPixels(11)
                }
            }
        }
    }
    Dialog {
        id: historyResultDialog
        property string message: ""
        title: qsTr("Traffic history")
        anchors.centerIn: parent
        width: Math.min(root.width - 32, 500)
        modal: true
        standardButtons: Dialog.Close
        Label { width: parent.width; wrapMode: Text.Wrap; text: historyResultDialog.message; color: Theme.textPrimary }
    }
    Connections {
        target: settingsController
        function onHistoryResult(success, message) {
            historyResultDialog.message = message
            historyResultDialog.open()
        }
    }

    Dialog {
        id: settingsDialog
        objectName: "settingsDialog"
        title: qsTr("Settings")
        anchors.centerIn: parent
        width: Math.min(root.width - 32, 620)
        height: Math.min(root.height - 32, 570)
        modal: true
        padding: 20
        background: Rectangle { color: Theme.surface; radius: 12; border.color: Theme.border }
        header: Label {
            text: qsTr("Settings")
            padding: 20
            color: Theme.textPrimary
            font.pixelSize: Theme.textPixels(20)
            font.bold: true
        }
        footer: Item {
            implicitHeight: 64
            ActionButton {
                anchors.right: parent.right
                anchors.rightMargin: 20
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("Close")
                onClicked: settingsDialog.close()
            }
        }
        onOpened: {
            databaseSetting.text = settingsController.databasePath
            settingsController.refresh()
        }
        Connections {
            target: settingsController
            function onDatabaseChanged(path) { databaseSetting.text = path }
        }
        ColumnLayout {
            anchors.fill: parent
            spacing: 14
            RowLayout {
                Layout.fillWidth: true
                ColumnLayout {
                    Layout.fillWidth: true
                    Label { text: qsTr("Start at boot"); font.bold: true; color: Theme.textPrimary }
                    Label {
                        Layout.fillWidth: true
                        text: qsTr("Keep Pigeon watching after reboots.")
                        wrapMode: Text.Wrap
                        color: Theme.textSecondary
                    }
                }
                Switch {
                    id: bootSwitch
                    objectName: "autostartSwitch"
                    checked: settingsController.autostart
                    enabled: settingsController.bootSupported && !settingsController.busy
                    implicitWidth: 88
                    implicitHeight: 32
                    text: checked ? qsTr("On") : qsTr("Off")
                    indicator: Rectangle {
                        implicitWidth: 44
                        implicitHeight: 24
                        y: (bootSwitch.height - height) / 2
                        radius: 12
                        color: bootSwitch.checked ? Theme.accent : Theme.border
                        opacity: bootSwitch.enabled ? 1 : 0.5
                        Rectangle {
                            width: 18
                            height: 18
                            radius: 9
                            y: 3
                            x: bootSwitch.checked ? parent.width - width - 3 : 3
                            color: "white"
                            Behavior on x { NumberAnimation { duration: 120 } }
                        }
                    }
                    contentItem: Text {
                        text: bootSwitch.text
                        leftPadding: 52
                        verticalAlignment: Text.AlignVCenter
                        color: Theme.textPrimary
                        opacity: bootSwitch.enabled ? 1 : 0.5
                        font.pixelSize: Theme.textPixels(13)
                    }
                    Accessible.name: qsTr("Start Pigeon automatically at boot")
                    onClicked: {
                        const requested = checked
                        checked = settingsController.autostart
                        settingsController.setAutostart(requested)
                    }
                    Binding { target: bootSwitch; property: "checked"; value: settingsController.autostart }
                }
            }
            Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }
            Label { text: qsTr("Database location"); font.bold: true; color: Theme.textPrimary }
            TextField {
                id: databaseSetting
                objectName: "databaseLocationSetting"
                Layout.fillWidth: true
                enabled: !settingsController.busy
                placeholderText: settingsController.defaultDatabasePath
                selectByMouse: true
                Accessible.name: qsTr("Collector database file path")
            }
            Label {
                Layout.fillWidth: true
                text: qsTr("Default: ") + settingsController.defaultDatabasePath
                wrapMode: Text.WrapAnywhere
                color: Theme.textMuted
                font.pixelSize: Theme.textPixels(11)
            }
            Label {
                Layout.fillWidth: true
                text: qsTr("Saving preserves existing history and restarts active capture. The original file is kept.")
                wrapMode: Text.Wrap
                color: Theme.textSecondary
                font.pixelSize: Theme.textPixels(11)
            }
            RowLayout {
                ActionButton {
                    text: qsTr("Use default")
                    enabled: !settingsController.busy
                    onClicked: databaseSetting.text = settingsController.defaultDatabasePath
                }
                ActionButton {
                    text: qsTr("Save location")
                    enabled: settingsController.available && !settingsController.busy
                        && databaseSetting.text !== settingsController.databasePath
                    onClicked: settingsController.setDatabasePath(databaseSetting.text)
                }
                BusyIndicator { running: settingsController.busy; implicitWidth: 26; implicitHeight: 26 }
            }
            Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }
            ActionButton {
                text: qsTr("Clear all history…")
                enabled: !settingsController.busy && readerController.databasePath.length > 0
                onClicked: {
                    historyConfirm.clearAll = true
                    historyConfirm.identities = []
                    historyConfirm.database = readerController.databasePath
                    historyConfirm.open()
                }
            }
            Label {
                Layout.fillWidth: true
                text: settingsController.message
                wrapMode: Text.Wrap
                color: Theme.textSecondary
            }
            Item { Layout.fillHeight: true }
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
                        onClicked: root.navigateTo(index)
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
                    text: qsTr("Traffic history")
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
                        text: qsTr("Settings")
                        Accessible.name: qsTr("Open Pigeon settings")
                        onClicked: settingsDialog.open()
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
                                : readerController.hasSnapshot ? qsTr("Viewing") : qsTr("No data")
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
                                onClicked: root.navigateTo(index)
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
                OverviewPage {
                    onConversationActivated: detailsDrawer.open()
                    onDeleteRequested: function(identities) { root.confirmConversationDeletion(identities) }
                }
                TrafficPage {
                    onDeleteRequested: function(identities) { root.confirmConversationDeletion(identities) }
                    live: true
                    onConversationActivated: detailsDrawer.open()
                }
                TrafficPage {
                    onDeleteRequested: function(identities) { root.confirmConversationDeletion(identities) }
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
        onDeleteRequested: function(identity) {
            historyConfirm.clearAll = false
            historyConfirm.conversation = (readerController.selectedDetails.application || "")
                + " · " + (readerController.selectedDetails.remoteIp || "")
                + ":" + (readerController.selectedDetails.remotePort || 0)
            historyConfirm.identities = [identity]
            historyConfirm.database = readerController.databasePath
            historyConfirm.open()
        }
        id: detailsDrawer
    }
}
