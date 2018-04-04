import QtQuick 2.7
import QtQuick.Layouts 1.2
import QtQuick.Controls 1.4
import radiance 1.0

Item {
    property alias model: view.model
    property alias view: view
    property alias currentOutputName: viewWrapper.currentOutputName
    property alias lastClickedTile: viewWrapper.lastClickedTile
    Layout.fillWidth: true;
    Layout.fillHeight: true;

    function insertVideoNode(videoNode) {
        model.addVideoNode(videoNode);
        if (lastClickedTile) {
            var edges = model.edges;
            for (var i=0; i<edges.length; i++) {
                if (edges[i].fromVertex == lastClickedTile.videoNode) {
                    model.addEdge(videoNode, edges[i].toVertex, edges[i].toInput);
                }
            }
            model.addEdge(lastClickedTile.videoNode, videoNode, 0);
        }
        model.flush();
        view.tileForVideoNode(videoNode).forceActiveFocus();
    }

    Flickable {
        id: flickable
        anchors.fill: parent
        contentWidth: view.width + 600;
        contentHeight: view.height + 400;
        clip: true;

        Item {
            id: viewWrapper
            property var lastClickedTile
            property string currentOutputName: ""
            width: Math.max(view.width + 400, flickable.width)
            height: Math.max(view.height + 400, flickable.height)
            focus: true

            View {
                id: view
                model: model
                delegates: {
                    "EffectNode": "EffectNodeTile",
                    "ImageNode": "ImageNodeTile",
                    "MovieNode": "MovieNodeTile",
                    "ScreenOutputNode": "ScreenOutputNodeTile",
                    "FFmpegOutputNode": "FFmpegOutputNodeTile",
                    "PlaceholderNode": "PlaceholderNodeTile",
                    "KineticsStripOutputNode": "KineticsStripTile",
                    "KineticsPanel72OutputNode": "KineticsPanel72Tile",
                    "": "VideoNodeTile"
                }
                x: (parent.width - width) / 2
                y: (parent.height - height) / 2

                Behavior on x { PropertyAnimation { easing.type: Easing.InOutQuad; duration: 500; } }
                Behavior on y { PropertyAnimation { easing.type: Easing.InOutQuad; duration: 500; } }

                // Temporary
                Keys.onPressed: {
                    if (event.key == Qt.Key_Space) {
                        view.Controls.changeControlRel(0, Controls.Enter, 1);
                    }
                }

                /*
                Rectangle {
                    opacity: 0.5
                    color: "red"
                    anchors.fill: parent
                }
                //*/
            }
        }
    }
}
