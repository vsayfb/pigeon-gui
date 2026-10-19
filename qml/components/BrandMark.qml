import QtQuick

Canvas {
    implicitWidth: 40
    implicitHeight: 40
    Accessible.name: qsTr("Pigeon")
    onPaint: {
        var ctx = getContext("2d");
        ctx.clearRect(0, 0, width, height);
        ctx.save();
        ctx.scale(width / 40, height / 40);

        function polygon(points, ink) {
            ctx.fillStyle = ink;
            ctx.beginPath();
            ctx.moveTo(points[0][0], points[0][1]);
            for (var i = 1; i < points.length; ++i)
                ctx.lineTo(points[i][0], points[i][1]);
            ctx.closePath();
            ctx.fill();
        }

        polygon([[4, 3], [22, 17], [13, 24]], "#008FFF");
        polygon([[12, 5], [28, 14], [19, 28]], "#356AF6");
        polygon([[7, 34], [18, 17], [29, 16], [27, 26]], "#3150EB");
        polygon([[3, 32], [17, 27], [12, 36]], "#00AEF0");
        polygon([[24, 18], [29, 13], [34, 15], [36, 21], [29, 25]], "#3863ED");
        polygon([[35, 18], [40, 21], [35, 22]], "#00A8F5");
        ctx.fillStyle = "white";
        ctx.beginPath();
        ctx.arc(31.8, 17.5, 1, 0, Math.PI * 2);
        ctx.fill();
        ctx.restore();
    }
}
