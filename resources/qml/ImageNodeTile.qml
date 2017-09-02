import QtQuick 2.3
import QtQuick.Layouts 1.2
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import radiance 1.0

VideoNodeTile {
    id: tile;

    onVideoNodeChanged: {

    }

    ColumnLayout {
        anchors.fill: parent;
        anchors.margins: 15;

        Label {
            Layout.fillWidth: true;
            text: videoNode.imagePath;
            color: "#ddd";
        }

        Item {
            Layout.preferredHeight: width;
            Layout.fillWidth: true;
            layer.enabled: true;

            CheckerboardBackground {
                anchors.fill: parent;
            }

            VideoNodeRender {
                anchors.fill: parent;
                chain: 0;
                id: vnr;
                videoNode: tile.videoNode;
            }
        }
    }

    Keys.onPressed: {

    }
}