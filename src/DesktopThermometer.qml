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

    // Fixed surface width: prevents Wayland layer shell resize configure oscillations
    width: 280

    // A window mask also defines the Wayland input region. Expand it before
    // showing the badge, and retain it through the fade-out.
    property rect inputRect: Qt.rect(
        readoutPill.visible || isHovered || isDragging ? Math.floor(railContainer.x + readoutPill.x - 5) : railContainer.x - 12,
        Math.max(0, railContainer.y - 8),
        readoutPill.visible || isHovered || isDragging ? Math.ceil(readoutPill.width + 32 + railContainer.width) : railContainer.width + 16,
        railContainer.height + 16)
    onInputRectChanged: backend.setInputRegion(thermoWindow, inputRect)
    Component.onCompleted: backend.setInputRegion(thermoWindow, inputRect)

    property bool rawHover: mouseArea.containsMouse || (readoutPill.visible && pillMouseArea.containsMouse)
    property bool isHovered: rawHover || hoverGraceTimer.running
    property bool isDragging: mouseArea.drag.active || pillMouseArea.drag.active

    // Brief grace timer so moving smoothly between the rail and readout badge never flickers
    Timer {
        id: hoverGraceTimer
        interval: 180
        repeat: false
    }

    onRawHoverChanged: {
        if (!rawHover && !isDragging) {
            hoverGraceTimer.restart()
        } else if (rawHover) {
            hoverGraceTimer.stop()
        }
    }

    Item {
        id: railContainer
        anchors.right: parent.right
        anchors.rightMargin: 2
        y: backend.railY

        // Fixed width: the thermometer scale NEVER moves or expands horizontally on hover
        width: backend.idleWidth
        height: backend.barHeight

        // Frosted glass background rail track with ice cold blue outline
        Rectangle {
            id: trackBackground
            anchors.fill: parent
            radius: width / 2
            color: "#d0141720"
            border.color: Qt.rgba(backend.coldColor.r, backend.coldColor.g, backend.coldColor.b, 0.45) // Ice cold blue outline
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

                    // Continuous cold blue to crimson red spectrum (no green elements)
                    gradient: Gradient {
                        orientation: Gradient.Vertical
                        GradientStop { position: 0.00; color: "#dc2626" } // Crimson Red (hot)
                        GradientStop { position: 0.28; color: "#ea580c" } // Hot orange
                        GradientStop { position: 0.55; color: "#94a3b8" } // Neutral midpoint
                        GradientStop { position: 1.00; color: "#38bdf8" } // Arctic Cold Blue
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

        // Floating holographic readout pill anchored adjacent to current temperature level
        Item {
            id: readoutPill
            anchors.right: trackBackground.left
            anchors.rightMargin: 12
            // Centered on the rail so sensor updates cannot move the hover target.
            y: (railContainer.height - height) / 2

            width: pillContent.width + 24
            height: pillContent.height + 16

            opacity: (isHovered || isDragging) ? 1.0 : 0.0
            visible: opacity > 0.001

            Behavior on opacity {
                NumberAnimation { duration: 180; easing.type: Easing.OutCubic }
            }

            // Frosted glass card backdrop with ice cold blue outline
            Rectangle {
                anchors.fill: parent
                radius: 12
                // Neutral smoked glass: a bright upper bevel over a dense base.
                gradient: Gradient {
                    GradientStop { position: 0; color: "#f2596069" }
                    GradientStop { position: 0.32; color: "#f0353b43" }
                    GradientStop { position: 1; color: "#f21c2128" }
                }

                Rectangle {
                    anchors.fill: parent
                    anchors.margins: 2
                    radius: 10
                    color: "transparent"
                    border.width: 1
                    border.color: "#35ffffff"
                }
                Rectangle {
                    anchors.top: parent.top
                    anchors.topMargin: 3
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: parent.width - 28
                    height: 1
                    color: "#75ffffff"
                }
                border.color: Qt.rgba(backend.coldColor.r, backend.coldColor.g, backend.coldColor.b, 0.45)
                border.width: 1

                // Subtle ambient glow shadow behind the card matching current temperature
                Rectangle {
                    anchors.fill: parent
                    anchors.margins: -1
                    radius: 13
                    color: "transparent"
                    border.color: Qt.rgba(backend.currentColor.r, backend.currentColor.g, backend.currentColor.b, 0.25)
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
                color: Qt.rgba(backend.coldColor.r, backend.coldColor.g, backend.coldColor.b, 0.55)
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

                Repeater {
                    model: backend.fanSpeeds
                    Text {
                        required property var modelData
                        width: 210
                        horizontalAlignment: Text.AlignHCenter
                        elide: Text.ElideRight
                        text: modelData.label + ": " + (modelData.rpm < 0 ? qsTr("Unavailable") : modelData.rpm + " RPM")
                        color: "#e2e8f0"
                        font.pixelSize: 11
                        font.features: { "tnum": 1 }
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                }
                Text {
                    visible: backend.fanSpeeds.length === 0
                    text: qsTr("Fan speed unavailable")
                    color: "#cbd5e1"
                    font.pixelSize: 11
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                // Hint for user in cold blue accent
                Text {
                    text: isDragging ? qsTr("Moving position...") : qsTr("Drag to move • Click for settings")
                    color: Qt.rgba(backend.coldColor.r, backend.coldColor.g, backend.coldColor.b, 0.7)
                    font.pixelSize: 9
                    anchors.horizontalCenter: parent.horizontalCenter
                }
            }

            // Mouse interaction directly on the holographic readout pill
            // Right margin extends across the 12px gap to ensure seamless overlap
            MouseArea {
                id: pillMouseArea
                anchors.fill: parent
                anchors.rightMargin: -16
                anchors.margins: -4
                hoverEnabled: true
                cursorShape: isDragging ? Qt.ClosedHandCursor : Qt.PointingHandCursor
                enabled: readoutPill.opacity > 0.05

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
                    if (pillMouseArea.drag.active || Math.abs(pillMouseArea.mouseY - startY) > 5) {
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

        // Mouse interaction for hover, dragging, and click-for-settings on the rail
        MouseArea {
            id: mouseArea
            anchors.fill: parent
            anchors.leftMargin: -12
            anchors.rightMargin: -4
            anchors.topMargin: -8
            anchors.bottomMargin: -8
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
                if (mouseArea.drag.active || Math.abs(mouseArea.mouseY - startY) > 5) {
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
