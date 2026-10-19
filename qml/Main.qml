import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Pigeon.Gui 1.0

ApplicationWindow {
    id: root

    width: 1040
    height: 700
    minimumWidth: 620
    minimumHeight: 480
    visible: true
    title: qsTr("Pigeon")
    color: Theme.background

    readonly property bool compact: width < 820

    RowLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.preferredWidth: 232
            Layout.fillHeight: true
            visible: !root.compact
            color: Theme.sidebar

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 24
                spacing: 0

                RowLayout {
                    spacing: 12

                    Rectangle {
                        Layout.preferredWidth: 36
                        Layout.preferredHeight: 36
                        radius: 10
                        color: Theme.accentSurface

                        Text {
                            anchors.centerIn: parent
                            text: "P"
                            font.pixelSize: 22
                            font.bold: true
                            color: Theme.accent
                        }
                    }

                    ColumnLayout {
                        spacing: 2

                        Text {
                            text: qsTr("Pigeon")
                            color: Theme.textPrimary
                            font.pixelSize: 20
                            font.weight: Font.DemiBold
                        }

                        Text {
                            text: qsTr("NETWORK VIEWER")
                            color: Theme.textMuted
                            font.pixelSize: 10
                            font.letterSpacing: 1.2
                        }
                    }
                }

                Item { Layout.preferredHeight: 52 }

                Text {
                    text: qsTr("WORKSPACE")
                    color: Theme.textMuted
                    font.pixelSize: 11
                    font.letterSpacing: 1.2
                }

                Item { Layout.preferredHeight: 14 }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 43
                    color: Theme.accentSurface
                    radius: 9

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left
                        anchors.leftMargin: 16
                        text: qsTr("Overview")
                        font.pixelSize: 14
                        font.weight: Font.DemiBold
                        color: Theme.accent
                    }
                }

                Item { Layout.preferredHeight: 22 }

                Text {
                    Layout.fillWidth: true
                    text: qsTr("Traffic, applications and endpoints will appear in later slices.")
                    wrapMode: Text.WordWrap
                    color: Theme.textMuted
                    font.pixelSize: 12
                    lineHeight: 1.4
                }

                Item { Layout.fillHeight: true }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 1
                    color: Theme.border
                }

                Item { Layout.preferredHeight: 16 }

                Text {
                    text: qsTr("Standalone desktop viewer")
                    font.pixelSize: 11
                    color: Theme.textSecondary
                }
            }
        }

        Flickable {
            id: workspace

            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            contentWidth: width
            contentHeight: content.implicitHeight + 64
            boundsBehavior: Flickable.StopAtBounds

            ScrollBar.vertical: ScrollBar { }

            ColumnLayout {
                id: content

                x: root.compact ? 24 : 40
                y: 32
                width: workspace.width - x * 2
                spacing: 24

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 16

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 3

                        Text {
                            visible: root.compact
                            text: qsTr("PIGEON")
                            color: Theme.accent
                            font.pixelSize: 11
                            font.bold: true
                            font.letterSpacing: 1.3
                        }

                        Text {
                            text: qsTr("Overview")
                            color: Theme.textPrimary
                            font.pixelSize: 26
                            font.weight: Font.DemiBold
                        }
                    }

                    Rectangle {
                        Layout.preferredWidth: 98
                        Layout.preferredHeight: 30
                        color: Theme.accentSurface
                        radius: 15

                        Text {
                            anchors.centerIn: parent
                            text: qsTr("Shell only")
                            color: Theme.accent
                            font.pixelSize: 12
                            font.weight: Font.Medium
                        }
                    }
                }

                SurfaceCard {
                    Layout.fillWidth: true
                    Layout.preferredHeight: root.compact ? 274 : 254

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: root.compact ? 22 : 30
                        spacing: 13

                        Rectangle {
                            Layout.preferredWidth: 78
                            Layout.preferredHeight: 30
                            radius: 8
                            color: Theme.accentSurface

                            Text {
                                anchors.centerIn: parent
                                text: "SQLite"
                                color: Theme.accent
                                font.pixelSize: 12
                                font.weight: Font.DemiBold
                            }
                        }

                        Item { Layout.preferredHeight: 2 }

                        Text {
                            Layout.fillWidth: true
                            text: qsTr("The desktop shell is ready")
                            color: Theme.textPrimary
                            font.pixelSize: root.compact ? 21 : 24
                            font.weight: Font.DemiBold
                            wrapMode: Text.WordWrap
                        }

                        Text {
                            Layout.fillWidth: true
                            text: qsTr("The next slice will connect to an existing schema-v4 Pigeon database in read-only mode. No traffic data is loaded yet.")
                            color: Theme.textSecondary
                            font.pixelSize: 14
                            wrapMode: Text.WordWrap
                            lineHeight: 1.35
                        }

                        Item { Layout.fillHeight: true }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 1
                            color: Theme.border
                        }

                        Text {
                            Layout.fillWidth: true
                            text: qsTr("No collector or capture privileges required")
                            color: Theme.positive
                            font.pixelSize: 12
                            wrapMode: Text.WordWrap
                        }
                    }
                }

                Text {
                    Layout.fillWidth: true
                    text: qsTr("This is the first, layout-only implementation slice. Database access, live rates and data views are not implemented in this build.")
                    color: Theme.textMuted
                    font.pixelSize: 12
                    wrapMode: Text.WordWrap
                    lineHeight: 1.4
                }
            }
        }
    }
}
