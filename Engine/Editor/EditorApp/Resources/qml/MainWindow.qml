import QtQuick
import QtQuick.Controls
import com.kdab.dockwidgets as KDDW

/// Top-level editor window. The MenuBar is data-driven off menuRegistry; the
/// DockingArea is populated dynamically from dockHost.dockEntries() — one
/// KDDW.DockWidget per registered factory, placed by its declared DockArea.
/// Multiple docks sharing the same area attach as tabs to the first one in
/// that area. Drag, float, re-dock, tabify are KDDW behaviours, no extra wiring.

ApplicationWindow {
    id: window
    width: 1600
    height: 1000
    visible: true
    title: qsTr("Hylux Editor")

    menuBar: MenuBar {
        Repeater {
            model: menuRegistry.topLevelMenus()
            delegate: Menu {
                title: modelData.title
                Repeater {
                    model: menuRegistry.itemsAt(modelData.fullPath)
                    delegate: MenuItem {
                        text: modelData.title
                        enabled: modelData.isEnabled
                        onTriggered: menuRegistry.trigger(modelData.fullPath)
                    }
                }
            }
        }
    }

    Component {
        id: dockTemplate
        KDDW.DockWidget {}
    }

    KDDW.DockingArea {
        id: dockingArea
        anchors.fill: parent
        uniqueName: "HyluxEditorMain"

        Component.onCompleted: layoutDocks()

        readonly property int locOnLeft:   1
        readonly property int locOnTop:    2
        readonly property int locOnRight:  3
        readonly property int locOnBottom: 4

        function locationForArea(area) {
            if (area === 0) return locOnLeft
            if (area === 1) return locOnRight
            if (area === 2) return locOnBottom
            return locOnTop
        }

        function layoutDocks() {
            var entries = dockHost.dockEntries()
            var firstInArea = ({})
            for (var i = 0; i < entries.length; ++i) {
                var entry = entries[i]
                var dock = dockTemplate.createObject(dockingArea, {
                    uniqueName: entry.id,
                    title: entry.title,
                    source: entry.qmlSource
                })
                var existing = firstInArea[entry.defaultArea]
                if (existing === undefined) {
                    dockingArea.addDockWidget(dock, locationForArea(entry.defaultArea))
                    firstInArea[entry.defaultArea] = dock
                } else {
                    existing.addDockWidgetAsTab(dock)
                }
            }
        }
    }
}
