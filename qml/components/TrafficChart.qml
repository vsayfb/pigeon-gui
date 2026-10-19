import QtQuick
import QtQuick.Layouts
import Pigeon.Gui 1.0
import "../theme/Format.js" as Format

SurfaceCard {
    id: chart
    property var history: []
    readonly property real duration: {
        var n = 0;
        for (var i = 0; i < history.length; ++i)
            n += history[i].seconds;
        return n;
    }
    readonly property real scale: {
        var peak = 0;
        for (var i = 0; i < history.length; ++i)
            peak = Math.max(peak, history[i].down, history[i].up);
        if (peak <= 0) return 1;
        var order = Math.pow(10, Math.floor(Math.log(peak) / Math.LN10));
        return Math.ceil(peak / order / 2) * order * 2;
    }
    implicitHeight: 295
    onHistoryChanged: plot.requestPaint()
    onScaleChanged: plot.requestPaint()
    onDurationChanged: plot.requestPaint()

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 12

        RowLayout {
            Layout.fillWidth: true
            Text {
                Layout.fillWidth: true
                text: qsTr("Traffic rates")
                font.pixelSize: 16
                font.weight: Font.DemiBold
                color: Theme.textPrimary
            }
            Text {
                text: qsTr("Viewer session · up to 60 samples")
                font.pixelSize: 11
                color: Theme.textSecondary
            }
        }

        Canvas {
            id: plot
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 158
            onWidthChanged: requestPaint()
            onHeightChanged: requestPaint()

            onPaint: {
                var ctx = getContext("2d");
                ctx.clearRect(0, 0, width, height);
                var l = 6, t = 12, r = width - 5, b = height - 14;
                if (r <= l || b <= t) return;
                ctx.strokeStyle = Theme.border;
                ctx.lineWidth = 1;
                ctx.setLineDash([4, 5]);
                for (var g = 0; g <= 4; g++) {
                    var y = t + (b - t) * g / 4;
                    ctx.beginPath(); ctx.moveTo(l, y); ctx.lineTo(r, y); ctx.stroke();
                }
                ctx.setLineDash([]);
                if (chart.history.length === 0) return;

                // Time spacing follows the real monotonic interval between samples.
                var elapsed = 0;
                function trace(key, ink) {
                    ctx.strokeStyle = ink;
                    ctx.lineWidth = 2.5;
                    ctx.lineJoin = "round";
                    ctx.beginPath();
                    elapsed = 0;
                    for (var i = 0; i < chart.history.length; ++i) {
                        var point = chart.history[i];
                        elapsed += point.seconds;
                        var x = chart.duration > 0 ? l + (r - l) * elapsed / chart.duration : r;
                        var y = b - (b - t) * Math.max(0, point[key]) / chart.scale;
                        if (i === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);
                    }
                    if (chart.history.length === 1) {
                        ctx.fillStyle = ink;
                        ctx.beginPath();
                        ctx.arc(r, b - (b - t) * chart.history[0][key] / chart.scale, 3, 0, 2 * Math.PI);
                        ctx.fill();
                    } else {
                        ctx.stroke();
                    }
                }
                trace("down", Theme.accent);
                trace("up", Theme.positive);
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Text {
                text: Format.size(chart.scale) + "/s"
                color: Theme.textMuted
                font.pixelSize: 11
            }
            Item { Layout.fillWidth: true }
            Text {
                text: chart.duration > 0 ? qsTr("Past %1s · now").arg(chart.duration.toFixed(0))
                                          : qsTr("Awaiting rate samples")
                color: Theme.textMuted
                font.pixelSize: 11
            }
        }
        RowLayout {
            spacing: 18
            Rectangle { width: 14; height: 3; radius: 2; color: Theme.accent }
            Text { text: qsTr("Download"); color: Theme.textSecondary; font.pixelSize: 12 }
            Rectangle { width: 14; height: 3; radius: 2; color: Theme.positive }
            Text { text: qsTr("Upload"); color: Theme.textSecondary; font.pixelSize: 12 }
            Item { Layout.fillWidth: true }
            Text {
                visible: chart.history.length === 0
                text: qsTr("Waiting for a second sample ≥ 1s later")
                font.pixelSize: 11
                color: Theme.warning
            }
        }
    }
}
