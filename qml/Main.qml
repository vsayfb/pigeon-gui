import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Pigeon.Gui 1.0
import "pages"

ApplicationWindow {
    id: root

    width: 1240
    height: 780
    minimumWidth: 620
    minimumHeight: 490
    visible: true
    title: qsTr("Pigeon GUI")
    color: Theme.background

    property int currentPage: 0
    readonly property bool compact: width < 940
    readonly property var titles: [qsTr("Overview"), qsTr("Live Traffic"),
                                   qsTr("Top Traffic"), qsTr("Applications"), qsTr("New Endpoints")]
    readonly property var symbols: ["▦", "↓↑", "≡", "▤", "◎"]

    RowLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.preferredWidth: 218
            Layout.fillHeight: true
            visible: !root.compact
            color: Theme.sidebar

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 20
                spacing: 0

                RowLayout {
                    spacing: 10
                    Rectangle {
                        Layout.preferredWidth: 38
                        Layout.preferredHeight: 38
                        radius: 11
                        color: Theme.accentSurface
                        Text {
                            anchors.centerIn: parent
                            text: "P"
                            font.pixelSize: 23
                            font.bold: true
                            color: Theme.accent
                        }
                    }
                    ColumnLayout {
                        spacing: 1
                        Text {
                            text: "Pigeon"
                            font.pixelSize: 20
                            font.weight: Font.DemiBold
                            color: Theme.textPrimary
                        }
                        Text {
                            text: qsTr("NETWORK VIEWER")
                            font.pixelSize: 10
                            font.letterSpacing: 1.1
                            color: Theme.textMuted
                        }
                    }
                }

                Item { Layout.preferredHeight: 45 }
                Text {
                    text: qsTr("WORKSPACE")
                    font.pixelSize: 11
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
                    text: qsTr("READ-ONLY · SQLITE V4")
                    font.pixelSize: 11
                    color: Theme.textSecondary
                }
                Text {
                    Layout.fillWidth: true
                    text: qsTr("Independent of collector")
                    font.pixelSize: 11
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
                Layout.topMargin: 19
                Layout.bottomMargin: 13
                spacing: 10

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2
                        Text {
                            visible: root.compact
                            text: qsTr("PIGEON")
                            font.pixelSize: 11
                            font.bold: true
                            font.letterSpacing: 1.4
                            color: Theme.accent
                        }
                        Text {
                            text: root.titles[root.currentPage]
                            font.pixelSize: root.compact ? 23 : 27
                            font.weight: Font.DemiBold
                            color: Theme.textPrimary
                        }
                    }
                    Rectangle {
                        width: 106
                        height: 30
                        radius: 15
                        color: readerController.error.length > 0 ? "#493039" : Theme.accentSurface
                        Text {
                            anchors.centerIn: parent
                            text: readerController.error.length > 0 ? qsTr("Read failed")
                                : readerController.hasSnapshot ? qsTr("Read-only") : qsTr("No data")
                            color: readerController.error.length > 0 ? Theme.danger : Theme.accent
                            font.pixelSize: 12
                            font.weight: Font.Medium
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 9
                    TextField {
                        id: databasePath
                        Layout.fillWidth: true
                        Layout.minimumWidth: 140
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
                    Button {
                        text: qsTr("Open")
                        contentItem: Text {
                            text: parent.text
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            color: Theme.background
                            font.weight: Font.DemiBold
                        }
                        background: Rectangle {
                            radius: 8
                            color: Theme.accent
                        }
                        Accessible.name: qsTr("Open existing database read-only")
                        onClicked: readerController.selectDatabase(databasePath.text)
                    }
                }

                Text {
                    Layout.fillWidth: true
                    text: readerController.status
                    color: Theme.textMuted
                    font.pixelSize: 11
                    elide: Text.ElideMiddle
                    ToolTip.visible: statusHover.hovered
                    ToolTip.text: text
                    HoverHandler { id: statusHover }
                }

                Rectangle {
                    Layout.fillWidth: true
                    visible: readerController.error.length > 0
                    implicitHeight: errorLabel.implicitHeight + 20
                    radius: 9
                    color: "#452A37"
                    border.color: "#A66070"
                    Text {
                        id: errorLabel
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.margins: 10
                        color: Theme.danger
                        font.pixelSize: 12
                        wrapMode: Text.WrapAnywhere
                        text: readerController.error
                    }
                }

                // On narrow windows navigation remains keyboard-accessible.
                RowLayout {
                    Layout.fillWidth: true
                    visible: root.compact
                    spacing: 2
                    Repeater {
                        model: 5
                        NavTab {
                            required property int index
                            Layout.fillWidth: true
                            Layout.minimumWidth: 0
                            Layout.preferredWidth: 110
                            compact: true
                            text: root.titles[index]
                            symbol: ""
                            selected: root.currentPage === index
                            onClicked: root.currentPage = index
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 1
                color: Theme.border
            }

            Loader {
                id: body
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumWidth: 0
                Layout.leftMargin: root.compact ? 14 : 24
                Layout.rightMargin: root.compact ? 14 : 24
                Layout.topMargin: 17
                visible: readerController.hasSnapshot
                active: visible
                sourceComponent: root.currentPage === 0 ? overviewComponent
                    : root.currentPage === 1 ? liveComponent
                    : root.currentPage === 2 ? topComponent
                    : root.currentPage === 3 ? applicationsComponent : endpointsComponent
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

    Component { id: overviewComponent; OverviewPage { } }
    Component { id: liveComponent; TrafficPage { live: true } }
    Component { id: topComponent; TrafficPage { live: false } }
    Component { id: applicationsComponent; ApplicationsPage { } }
    Component { id: endpointsComponent; EndpointsPage { } }
}
