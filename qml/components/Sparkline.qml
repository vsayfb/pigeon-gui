import QtQuick
import QtQuick.Controls
import Pigeon.Gui 1.0

Item {
    id: spark
    property var samples: []
    onSamplesChanged: graph.requestPaint()

    Connections {
        target: Theme
        function onDarkChanged() { graph.requestPaint() }
    }

    Canvas {
        id: graph
        anchors.fill: parent
        anchors.margins: 10
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
        onPaint: {
            var ctx = getContext("2d");
            ctx.clearRect(0, 0, width, height);
            if (width <= 0 || height <= 0) return;
            ctx.strokeStyle = Theme.border;
            ctx.lineWidth = 1;
            ctx.beginPath();
            ctx.moveTo(0, height - 2);
            ctx.lineTo(width, height - 2);
            ctx.stroke();
            if (spark.samples.length === 0) return;

            var peak = 1;
            for (var i = 0; i < spark.samples.length; ++i)
                peak = Math.max(peak, spark.samples[i]);
            ctx.strokeStyle = Theme.download;
            ctx.lineWidth = 1.7;
            ctx.lineJoin = "round";
            ctx.beginPath();
            for (var j = 0; j < spark.samples.length; ++j) {
                var x = spark.samples.length > 1 ? j * width / (spark.samples.length - 1) : width;
                var y = height - 2 - spark.samples[j] / peak * (height - 4);
                if (j === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);
            }
            ctx.stroke();
        }
    }

    HoverHandler { id: hover }
    ToolTip.visible: hover.hovered
    ToolTip.text: samples.length ? qsTr("Combined payload rate · up to 30 viewer-session samples · individually scaled")
        : qsTr("Waiting for measured rates")
}
