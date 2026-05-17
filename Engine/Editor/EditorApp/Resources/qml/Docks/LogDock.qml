import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    anchors.fill: parent
    color: "#1c1c20"

    property var viewModel: dockHost.viewModel("hylux.log")

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 4
        spacing: 4

        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            ComboBox {
                id: levelFilter
                model: ["Trace", "Debug", "Info", "Warn", "Error", "Fatal"]
                currentIndex: viewModel ? viewModel.minLevel : 2
                onCurrentIndexChanged: if (viewModel) viewModel.minLevel = currentIndex
            }
            TextField {
                placeholderText: "Filter category…"
                onTextChanged: if (viewModel) viewModel.categoryFilter = text
                Layout.fillWidth: true
            }
            Button {
                text: "Clear"
                onClicked: if (viewModel) viewModel.Clear()
            }
        }

        ListView {
            id: list
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: viewModel ? viewModel.entries : null
            delegate: Rectangle {
                width: list.width
                height: rowText.implicitHeight + 6
                color: index % 2 === 0 ? "#1c1c20" : "#212126"
                Row {
                    anchors.fill: parent
                    anchors.margins: 4
                    spacing: 8
                    Label { text: category; color: "#7a9cc6"; font.pixelSize: 12 }
                    Label {
                        id: rowText
                        text: message
                        color: "#dcdcdc"
                        font.pixelSize: 12
                        wrapMode: Text.Wrap
                        width: parent.width - 200
                    }
                }
            }
        }
    }
}
