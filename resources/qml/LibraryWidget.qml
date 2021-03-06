import QtQuick 2.7
import QtQuick.Layouts 1.2
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import QtQml.Models 2.2
import radiance 1.0

Item {
    id: libraryWidget
    property var graph
    property var registry
    property var context

    ColumnLayout {
        anchors.fill: parent

        TreeView {
            id: librarytree

            /* Workaround for QTBUG-47243,
               which seems to still be present in
               Qt 5.10 */
            selection: ItemSelectionModel {
                id: selModel
                    model: librarytree.model
            }
            onClicked: {
                selModel.clearCurrentIndex();
                selModel.setCurrentIndex(index, 0x0002 | 0x0010);
            }
            /* End workaround */

            Layout.fillHeight: true;
            Layout.fillWidth: true;
            model: registry.library;
            backgroundVisible: false;
            headerVisible: false;
            alternatingRowColors: false
            horizontalScrollBarPolicy: Qt.ScrollBarAlwaysOff;
            verticalScrollBarPolicy: Qt.ScrollBarAlwaysOff;

            TableViewColumn {
                role: "name"
            }
            style: TreeViewStyle {
                TextMetrics {
                    id: tm
                    text: "MM"
                }
                indentation: tm.width;
                branchDelegate: Text {
                    TextMetrics {
                        id: tm
                        text: "M"
                    }
                    width: tm.width
                    text: styleData.isExpanded ? "▼" : "▶"
                    color: "#aaa"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                frame: Item {}
            }
            itemDelegate: Item {
                id: container
                Text {
                    color: styleData.selected ? "white" : "#aaa"
                    font.bold: styleData.selected
                    text: styleData.value
                    verticalAlignment: Text.AlignVCenter
                }
            }
            rowDelegate: Item {
                height: tm.height;
                TextMetrics {
                    id: tm
                    text: " "
                }
            }
            onDoubleClicked: {
                addSelected();
            }

            Connections {
                target: librarytree.model

                onFilterChanged: {
                    if (!filter) return;
                    selModel.clearCurrentIndex();
                    function selectOrExpand(index) {
                        if (librarytree.model.hasChildren(index)) {
                            librarytree.expand(index);
                            selectOrExpand(librarytree.model.index(0, 0, index));
                        } else {
                            selModel.setCurrentIndex(index, 0x0002 | 0x0010);
                        }
                    }
                    selectOrExpand(librarytree.model.index(0, 0));
                }
            }

            TextField {
                id: searchBox
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                visible: false
                onAccepted: {
                    addSelected();
                    stopSearching();
                }
                Keys.onPressed: {
                    if (event.key == Qt.Key_Escape) {
                        stopSearching();
                    }
                }
                Component.onCompleted: {
                    registry.library.filter = Qt.binding(function() { return text; });
                }
            }
        }

        // pass
        Action {
            id: nodeAddAction
            onTriggered: {
                addSelected();
            }
        }
        Button {
            id: nodeAddButton
            text: "Add"
            action: nodeAddAction
        }
        Label {
            id: nodeAddDescription
            Layout.fillWidth: true;
            elide: Text.ElideRight;
            color: "#ddd";
            //text: NodeRegistry.nodeTypes[nodeSelector.currentText].description;
        }
    }

    function search() {
        searchBox.visible = true;
        searchBox.focus = true;
        searchBox.forceActiveFocus();
    }

    function stopSearching() {
        searchBox.text = "";
        searchBox.visible = false;
        searchBox.focus = false;
    }

    function finishCreation() {
    }

    function addSelected() {
        var filename = librarytree.model.data(librarytree.selection.currentIndex, Library.FileRole);

        if (filename.substr(-4) == ".qml") {
            var comp = Qt.createComponent("Instantiators/" + filename);

            function finishCreation() {
                if (comp.status == Component.Ready) {
                    var obj = comp.createObject(libraryWidget, {"graph": graph, "registry": registry, "context": context});
                    if (obj == null) {
                        // Error Handling
                        console.log("Error creating object");
                    }
                } else if (comp.status == Component.Error) {
                    // Error Handling
                    console.log("Error loading component:", comp.errorString());
                }
            }

            if (comp.status == Component.Ready) {
                finishCreation();
            } else if (comp.status == Component.Loading) {
                comp.statusChanged.connect(finishCreation);
            } else if (comp.status == Component.Error) {
                console.log("Error loading component:", comp.errorString());
            }
        } else {
            var vn = registry.createFromFile(context, filename);
            if (vn) {
                graph.insertVideoNode(vn);
            }
        }
        stopSearching();
    }

    Action {
        id: searchAction
        shortcut: ":"
        onTriggered: search()
    }
}
