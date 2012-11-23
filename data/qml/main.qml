import QtQuick 2.0

Rectangle {
    id: rootItem
    color: "transparent"

    Text {
        id: fpsText
        anchors.top: rootItem.top
        anchors.left: rootItem.left
        anchors.margins: 5
        visible: emuView.fpsVisible;

        color: "yellow"
        text: "fps: " + hostVideo.fps;
    }
}
