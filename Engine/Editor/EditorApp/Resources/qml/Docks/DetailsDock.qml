import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    anchors.fill: parent
    color: "#1c1c20"

    property var viewModel: dockHost.viewModel("hylux.details")

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 4

        Label {
            text: qsTr("Selection")
            color: "#cfcfcf"
            font.bold: true
        }

        Repeater {
            model: viewModel ? viewModel.selectionNames : []
            delegate: Label {
                text: "• " + modelData
                color: "#dcdcdc"
            }
        }

        Label {
            visible: !viewModel || viewModel.selectionNames.length === 0
            text: qsTr("(nothing selected)")
            color: "#6a6a72"
        }

        Item { Layout.fillHeight: true }
    }
}
