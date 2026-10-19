import QtQuick
import QtQuick.Controls
import Pigeon.Gui 1.0

Item {
    id: direction
    property bool incoming: true
    property bool observed: false
    implicitWidth: 18
    implicitHeight: 18
    opacity: observed ? 1 : 0.22
    Accessible.name: incoming ? qsTr("Incoming packets") : qsTr("Outgoing packets")
    Accessible.description: observed ? qsTr("Observed") : qsTr("Not observed")

    Connections {
        target: Theme
        function onDarkChanged() { canvas.requestPaint() }
    }

    Canvas {
        id: canvas
        anchors.fill: parent
        onPaint: {
            var ctx = getContext("2d");
            ctx.clearRect(0, 0, width, height);
            ctx.strokeStyle = direction.incoming ? Theme.download : Theme.upload;
            ctx.lineWidth = Math.max(1.6, width / 9);
            ctx.lineCap = "round";
            ctx.lineJoin = "round";
            ctx.beginPath();
            if (direction.incoming) {
                ctx.moveTo(width / 2, height * .19);
                ctx.lineTo(width / 2, height * .81);
                ctx.moveTo(width * .27, height * .58);
                ctx.lineTo(width / 2, height * .81);
                ctx.lineTo(width * .73, height * .58);
            } else {
                ctx.moveTo(width / 2, height * .81);
                ctx.lineTo(width / 2, height * .19);
                ctx.moveTo(width * .27, height * .42);
                ctx.lineTo(width / 2, height * .19);
                ctx.lineTo(width * .73, height * .42);
            }
            ctx.stroke();
        }
    }
}
