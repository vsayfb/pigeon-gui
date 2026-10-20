import QtQuick

Item {
    implicitWidth: 40
    implicitHeight: 40
    Accessible.name: qsTr("Pigeon")

    Image {
        anchors.fill: parent
        source: "qrc:/qt/qml/Pigeon/Gui/assets/pigeon.png"
        sourceSize.width: 160
        sourceSize.height: 160
        fillMode: Image.PreserveAspectFit
        smooth: true
        mipmap: true
    }
}
