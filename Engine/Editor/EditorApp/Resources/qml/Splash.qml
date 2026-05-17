import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    color: "#1e1e22"

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 32
        spacing: 16

        Label {
            text: "Hylux Editor"
            color: "#f3f3f3"
            font.pixelSize: 28
            font.bold: true
        }

        Label {
            text: bootstrap.stepName.length > 0 ? bootstrap.stepName : "Starting..."
            color: "#d0d0d0"
            font.pixelSize: 14
            Layout.fillWidth: true
        }

        ProgressBar {
            from: 0
            to: 100
            value: bootstrap.percent
            Layout.fillWidth: true
        }

        Label {
            text: bootstrap.message
            color: "#9a9a9a"
            font.pixelSize: 12
            elide: Text.ElideMiddle
            Layout.fillWidth: true
            visible: bootstrap.message.length > 0
        }

        Item { Layout.fillHeight: true }
    }
}
