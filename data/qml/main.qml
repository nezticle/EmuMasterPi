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

    Rectangle {
        visible: false;

        anchors.centerIn: parent
        width: rootItem.width / 3;
        height: 300
        radius: 25
        border.color: "steelblue"
        border.width: 5
        color: "lightsteelblue";

        Text {
            id: menuTitleText
            anchors.horizontalCenter: parent.horizontalCenter;
            anchors.top: parent.top;
            anchors.topMargin: 25;
            text: "Menu"
            font.pointSize: 32
        }

        Column {
            anchors.top: menuTitleText.bottom
            anchors.bottom: parent.bottom
            anchors.margins: 25
            anchors.right: parent.right
            anchors.left: parent.left

            Toggle {
                id: fpsVisibleToggle
                text: "FPS Visible"
                target: emuView
                checked: true
                property: "fpsVisible"
            }

            Toggle {
                id: audioEnabledToggle
                text: "Audio Enabled"
                target: emuView
                checked: true
                property: "audioEnable"
            }

            Toggle {
                id: keepAspectRatioToggle
                text: "Keep Aspect Ratio"
                target: emuView
                checked: true
                property: "keepAspectRatio"
            }

        }
    }
}
