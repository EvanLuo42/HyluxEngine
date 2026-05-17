import QtQuick
import QtQuick.Controls

Rectangle {
    anchors.fill: parent
    color: "#1c1c20"
    Label {
        anchors.centerIn: parent
        color: "#6a6a72"
        text: qsTr("Outliner — empty\n(populated once entity store is wired)")
        horizontalAlignment: Text.AlignHCenter
    }
}
