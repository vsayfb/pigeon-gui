import QtQuick
import QtQuick.Layouts
import Pigeon.Gui 1.0
import "../theme/Format.js" as Format

SurfaceCard {
    id: chart
    property var history: []
    readonly property real duration: {
        var seconds = 0;
        for (var i = 0; i < history.length; ++i)
            seconds += history[i].seconds;
        return seconds;
    }
    readonly property real scale: {
        var peak = 0;
        for (var i = 0; i < history.length; ++i)
            peak = Math.max(peak, history[i].down, history[i].up);
        if (peak <= 0) return 1024;
        var order = Math.pow(10, Math.floor(Math.log(peak) / Math.LN10));
        return Math.ceil(peak / order / 2) * order * 2;
    }
    implicitHeight: 270
    onHistoryChanged: plot.requestPaint()
    onScaleChanged: plot.requestPaint()
    onDurationChanged: plot.requestPaint()

    Connections {
        target: Theme
        function onDarkChanged() { plot.requestPaint() }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 18
        spacing: 8

        RowLayout {
            Layout.fillWidth: true

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 3

                Text {
                    text: qsTr("Traffic volume")
                    font.pixelSize: Theme.textPixels(15)
                    font.weight: Font.DemiBold
                    color: Theme.textPrimary
                }

                Text {
                    text: qsTr("Payload rates · viewer session · up to 60 samples")
                    font.pixelSize: Theme.textPixels(10)
                    color: Theme.textMuted
                }
            }

            Item { Layout.fillWidth: true }

            Repeater {
                model: [qsTr("↓ Inbound"), qsTr("↑ Outbound")]

                Rectangle {
                    required property int index
                    required property string modelData
                    Layout.preferredWidth: 105
                    Layout.preferredHeight: 43
                    radius: 8
                    color: Theme.surface
                    border.color: Theme.border

                    Column {
                        anchors.centerIn: parent
                        spacing: 3

                        Text {
                            text: modelData
                            color: index === 0 ? Theme.download : Theme.upload
                            font.pixelSize: Theme.textPixels(10)
                        }

                        Text {
                            text: readerController.ratesMeasured
                                ? Format.speed(index === 0 ? readerController.downloadRateValue : readerController.uploadRateValue, true) : "—"
                            color: Theme.textPrimary
                            font.pixelSize: Theme.textPixels(11)
                            font.weight: Font.DemiBold
                        }
                    }
                }
            }
        }

        Canvas {
            id: plot
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 100
            onWidthChanged: requestPaint()
            onHeightChanged: requestPaint()

            onPaint: {
                var ctx = getContext("2d");
                ctx.clearRect(0, 0, width, height);
                var left = 70, top = 10, right = width - 10, bottom = height - 22;
                if (right <= left || bottom <= top) return;

                ctx.font = Theme.textPixels(10) + "px sans-serif";
                ctx.lineWidth = 1;
                for (var g = 0; g <= 4; ++g) {
                    var y = top + (bottom - top) * g / 4;
                    ctx.strokeStyle = Theme.border;
                    ctx.beginPath();
                    ctx.moveTo(left, y);
                    ctx.lineTo(right, y);
                    ctx.stroke();
                    ctx.fillStyle = Theme.textMuted;
                    ctx.textAlign = "right";
                    ctx.fillText(g === 4 ? "0" : Format.size(chart.scale * (4 - g) / 4) + "/s", left - 9, y + 4);
                }

                for (var xline = 0; xline <= 6; ++xline) {
                    var gridX = left + (right - left) * xline / 6;
                    ctx.strokeStyle = Theme.border;
                    ctx.beginPath();
                    ctx.moveTo(gridX, top);
                    ctx.lineTo(gridX, bottom);
                    ctx.stroke();
                }

                ctx.textAlign = "left";
                ctx.fillStyle = Theme.textMuted;
                ctx.fillText(chart.duration > 0 ? "−" + chart.duration.toFixed(0) + "s" : qsTr("Awaiting samples"), left, height - 3);
                ctx.textAlign = "right";
                ctx.fillText(qsTr("Now"), right, height - 3);
                if (chart.history.length === 0) return;

                function trace(key, ink) {
                    var points = [];
                    var elapsed = 0;
                    var first = chart.history[0].seconds;
                    var span = chart.duration - first;
                    for (var i = 0; i < chart.history.length; ++i) {
                        var sample = chart.history[i];
                        elapsed += sample.seconds;
                        points.push({x: span > 0 ? left + (right - left) * (elapsed - first) / span : right,
                            y: bottom - (bottom - top) * Math.max(0, sample[key]) / chart.scale});
                    }

                    if (points.length > 1) {
                        var gradient = ctx.createLinearGradient(0, top, 0, bottom);
                        gradient.addColorStop(0, Qt.rgba(ink.r, ink.g, ink.b, 0.26));
                        gradient.addColorStop(1, Qt.rgba(ink.r, ink.g, ink.b, 0.015));
                        ctx.fillStyle = gradient;
                        ctx.beginPath();
                        ctx.moveTo(points[0].x, bottom);
                        for (var p = 0; p < points.length; ++p)
                            ctx.lineTo(points[p].x, points[p].y);
                        ctx.lineTo(right, bottom);
                        ctx.closePath();
                        ctx.fill();
                    }

                    ctx.strokeStyle = ink;
                    ctx.lineWidth = 2;
                    ctx.lineJoin = "round";
                    ctx.beginPath();
                    ctx.moveTo(points[0].x, points[0].y);
                    for (var j = 1; j < points.length; ++j)
                        ctx.lineTo(points[j].x, points[j].y);
                    ctx.stroke();
                    ctx.fillStyle = ink;
                    var last = points[points.length - 1];
                    ctx.beginPath();
                    ctx.arc(last.x, last.y, 2.5, 0, Math.PI * 2);
                    ctx.fill();
                }

                trace("down", Theme.download);
                trace("up", Theme.upload);
            }
        }
    }
}
