import QtQuick
import QtQuick.Window
import org.kde.layershell as LayerShell

Window {
    id: thermoWindow
    visible: true
    color: "transparent"
    flags: Qt.FramelessWindowHint

    LayerShell.Window.layer: LayerShell.Window.LayerBottom
    LayerShell.Window.anchors: LayerShell.Window.AnchorRight | LayerShell.Window.AnchorTop | LayerShell.Window.AnchorBottom
    LayerShell.Window.exclusionZone: 0
    LayerShell.Window.keyboardInteractivity: LayerShell.Window.KeyboardInteractivityNone
    LayerShell.Window.scope: "thermo-desktop-strip"

    property bool isHovered: mouseArea.containsMouse || (readoutPill.opacity > 0.1 && pillMouseArea.containsMouse)
    property bool isDragging: mouseArea.drag.active || pillMouseArea.drag.active

    // Resting width is narrow so it never blocks desktop clicks when idle.
    // Expands on hover/drag so the readout pill and interaction zone have full room.
    width: (isHovered || isDragging) ? 220 : (backend.idleWidth + 8)

    Behavior on width {
        NumberAnimation { duration: 180; easing.type: Easing.OutCubic }
    }

    Item {
        id: railContainer
        anchors.right: parent.right
        anchors.rightMargin: 2
        y: backend.railY
        width: (isHovered || isDragging) ? backend.expandedWidth : backend.idleWidth
        height: backend.barHeight

        Behavior on width {
            NumberAnimation { duration: 180; easing.type: Easing.OutBack }
        }

        // Frosted glass background rail track
        Rectangle {
            id: trackBackground
            anchors.fill: parent
            radius: width / 2
            color: "#161922bb"
            border.color: "#ffffff22"
            border.width: 1

            // Liquid temperature column filling from bottom
            Item {
                id: fillClip
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: Math.max(parent.width, Math.min(parent.height, parent.height * backend.normalizedTemperature))
                clip: true

                Behavior on height {
                    NumberAnimation { duration: 650; easing.type: Easing.OutCubic }
                }

                Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    height: trackBackground.height
                    radius: trackBackground.radius

                    gradient: Gradient {
                        orientation: Gradient.Vertical
                        GradientStop { position: 0.00; color: "#dc2626" } // Crimson Red (hot)
                        GradientStop { position: 0.25; color: "#ea580c" } // Burnt orange
                        GradientStop { position: 0.55; color: "#f59e0b" } // Warm amber / burnt copper
                        GradientStop { position: 0.80; color: "#84cc16" } // Lime
                        GradientStop { position: 1.00; color: "#10b981" } // Emerald green (cool)
                    }
                }

                // Meniscus glowing cap at the current surface level
                Rectangle {
                    id: meniscusGlow
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    height: Math.max(3, parent.width)
                    radius: height / 2
                    color: backend.currentColor

                    SequentialAnimation on opacity {
                        loops: Animation.Infinite
                        NumberAnimation { to: 0.65; duration: 2400; easing.type: Easing.InOutSine }
                        NumberAnimation { to: 1.00; duration: 2400; easing.type: Easing.InOutSine }
                    }
                }
            }
        }

        // Floating holographic readout pill adjacent to the current temperature level
        Item {
            id: readoutPill
            anchors.right: trackBackground.left
            anchors.rightMargin: 12
            // Align vertically with the meniscus level
            y: Math.max(0, Math.min(railContainer.height - height, (railContainer.height - fillClip.height) - height / 2))

            width: pillContent.width + 24
            height: pillContent.height + 16

            opacity: (isHovered || isDragging) ? 1.0 : 0.0
            visible: opacity > 0.001
            scale: (isHovered || isDragging) ? 1.0 : 0.85

            Behavior on opacity {
                NumberAnimation { duration: 200; easing.type: Easing.OutCubic }
            }
            Behavior on scale {
                NumberAnimation { duration: 220; easing.type: Easing.OutBack }
            }
            Behavior on y {
                NumberAnimation { duration: 300; easing.type: Easing.OutCubic }
            }

            // Frosted glass card backdrop
            Rectangle {
                anchors.fill: parent
                radius: 12
                color: "#0d1117ea"
                border.color: Qt.rgba(backend.currentColor.r, backend.currentColor.g, backend.currentColor.b, 0.45)
                border.width: 1

                // Subtle ambient glow shadow behind the card
                Rectangle {
                    anchors.fill: parent
                    anchors.margins: -1
                    radius: 13
                    color: "transparent"
                    border.color: Qt.rgba(backend.currentColor.r, backend.currentColor.g, backend.currentColor.b, 0.20)
                    border.width: 1
                    z: -1
                }
            }

            // Connecting hairline tick between pill and rail
            Rectangle {
                anchors.left: parent.right
                anchors.verticalCenter: parent.verticalCenter
                width: 12
                height: 1
                color: Qt.rgba(backend.currentColor.r, backend.currentColor.g, backend.currentColor.b, 0.6)
            }

            Column {
                id: pillContent
                anchors.centerIn: parent
                spacing: 3

                Row {
                    spacing: 7
                    anchors.horizontalCenter: parent.horizontalCenter

                    // Glowing status dot
                    Rectangle {
                        width: 8
                        height: 8
                        radius: 4
                        anchors.verticalCenter: parent.verticalCenter
                        color: backend.currentColor

                        Rectangle {
                            anchors.fill: parent
                            anchors.margins: -2
                            radius: 6
                            color: "transparent"
                            border.color: Qt.rgba(backend.currentColor.r, backend.currentColor.g, backend.currentColor.b, 0.4)
                            border.width: 1
                        }
                    }

                    // Bold numerical temperature readout
                    Text {
                        text: backend.formattedTemperature
                        color: "#f8fafc"
                        font.pixelSize: 16
                        font.weight: Font.DemiBold
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }

                // Sensor name / label
                Text {
                    text: backend.sensorLabel
                    color: "#94a3b8"
                    font.pixelSize: 10
                    font.weight: Font.Medium
                    anchors.horizontalCenter: parent.horizontalCenter
                    elide: Text.ElideRight
                    maximumLineCount: 1
                }

                // Hint for user
                Text {
                    text: isDragging ? qsTr("Moving position...") : qsTr("Drag to move • Click for settings")
                    color: Qt.rgba(theme.accentForeground === "white" ? 1 : 0.8, 0.8, 0.8, 0.5)
                    font.pixelSize: 9
                    anchors.horizontalCenter: parent.horizontalCenter
                }
            }

            // Mouse interaction directly on the holographic readout pill
            MouseArea {
                id: pillMouseArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: isDragging ? Qt.ClosedHandCursor : Qt.PointingHandCursor
                visible: readoutPill.opacity > 0.1

                drag.target: railContainer
                drag.axis: Drag.YAxis
                drag.minimumY: 28
                drag.maximumY: Math.max(28, thermoWindow.height - railContainer.height - 20)

                property real startY: 0
                property bool moved: false

                onPressed: (mouse) => {
                    startY = pillMouseArea.mouseY
                    moved = false
                }

                onPositionChanged: (mouse) => {
                    if (Math.abs(pillMouseArea.mouseY - startY) > 5) {
                        moved = true
                    }
                }

                onReleased: {
                    if (moved) {
                        backend.setRailY(railContainer.y)
                    }
                }

                onClicked: {
                    if (!moved) {
                        backend.openSettings()
                    }
                }
            }
        }

        // Mouse interaction for hover, dragging, and click-for-settings
        MouseArea {
            id: mouseArea
            anchors.fill: parent
            anchors.margins: -6 // Generous hit area around the thin strip
            hoverEnabled: true
            cursorShape: isDragging ? Qt.ClosedHandCursor : (isHovered ? Qt.PointingHandCursor : Qt.ArrowCursor)

            drag.target: railContainer
            drag.axis: Drag.YAxis
            drag.minimumY: 28
            drag.maximumY: Math.max(28, thermoWindow.height - railContainer.height - 20)

            property real startY: 0
            property bool moved: false

            onPressed: (mouse) => {
                startY = mouseArea.mouseY
                moved = false
            }

            onPositionChanged: (mouse) => {
                if (Math.abs(mouseArea.mouseY - startY) > 5) {
                    moved = true
                }
            }

            onReleased: {
                if (moved) {
                    backend.setRailY(railContainer.y)
                }
            }

            onClicked: {
                if (!moved) {
                    backend.openSettings()
                }
            }
        }
    }
}
