import QtQuick

QtObject {
    id: root
    property var desktopWindow: null
    property Component desktopFactory: Component { DesktopThermometer {} }
    function rebuildDesktop() {
        if (desktopWindow) desktopWindow.destroy()
        // A QObject parent keeps this a real top-level window. An unattached
        // visual Loader would make it wait for a parent window that never exists.
        desktopWindow = desktopFactory.createObject(root)
    }
    Component.onCompleted: rebuildDesktop()
    property Timer placementReload: Timer {
        interval: 0
        onTriggered: root.rebuildDesktop()
    }
    property Connections placementConnection: Connections {
        target: backend
        function onPlacementChanged() { placementReload.restart() }
    }
    property SettingsWindow settingsWindow: SettingsWindow {}
}
