import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: window
    width: 1280
    height: 800
    visible: true
    title: qsTr("Hylux Editor")

    ColumnLayout {
        anchors.centerIn: parent
        spacing: 16

        Label {
            text: qsTr("Hylux Editor")
            font.pixelSize: 32
            Layout.alignment: Qt.AlignHCenter
        }

        Label {
            text: qsTr("Runtime initialized.")
            Layout.alignment: Qt.AlignHCenter
        }
    }
}
