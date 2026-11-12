import QtQuick
import QtQuick.Window
import org.kde.layershell as LayerShell

Window {
    id: thermoWindow
    property bool followTheme: backend.appearanceMode === "omarchy"
    readonly property bool lightRails: followTheme && theme.tokens.light
    readonly property color themeBackground: followTheme ? theme.tokens.background : "#141720"
    readonly property color railSurface: lightRails
        ? Qt.rgba(themeBackground.r, themeBackground.g, themeBackground.b, 0.72) : "#d0141720"
    readonly property color railOutline: lightRails ? "#48516070"
        : Qt.rgba(chromeColor.r, chromeColor.g, chromeColor.b, 0.45)
    readonly property color fanOutline: lightRails ? "#600369a1" : "#38bdf8"
    property real textScale: followTheme ? theme.tokens.baseSize / 12 : 1
    property color textColor: followTheme ? theme.tokens.foreground : "#f8fafc"
    property color mutedColor: followTheme ? theme.tokens.muted : "#94a3b8"
    property color warningColor: followTheme && theme.tokens.light ? "#92400e" : "#fbbf24"
    property color chromeColor: followTheme ? theme.tokens.border : "#38bdf8"
    property string textFamily: followTheme ? theme.tokens.fontFamily : Qt.application.font.family
    property bool leftEdge: backend.edge === "left"
    visible: false
    color: "transparent"
    flags: Qt.FramelessWindowHint

    LayerShell.Window.layer: LayerShell.Window.LayerBottom
    LayerShell.Window.anchors: (leftEdge ? LayerShell.Window.AnchorLeft : LayerShell.Window.AnchorRight) | LayerShell.Window.AnchorTop | LayerShell.Window.AnchorBottom
    LayerShell.Window.exclusionZone: 0
    LayerShell.Window.keyboardInteractivity: LayerShell.Window.KeyboardInteractivityNone
    LayerShell.Window.scope: "thermo-desktop-strip"

    // Both layer surfaces stay fixed in size, including during hover and drag.
    width: 48
    property int topHitMargin: backend.fanRailEnabled ? 20 : 8
    readonly property real railGap: Math.max(4, 12 - backend.idleWidth)
    readonly property real groupWidth: backend.fanRailEnabled ? backend.idleWidth * 2 + railGap : backend.idleWidth
    readonly property real groupLeft: leftEdge ? 2 : width - 2 - groupWidth
    readonly property real effectiveBarHeight: Math.min(backend.barHeight, Math.max(1, height - 48))
    readonly property real maximumRailY: Math.max(28, height - effectiveBarHeight - 20)
    function clampRailY(value) { return Math.max(28, Math.min(maximumRailY, value)) }
    property rect railRegion: Qt.rect(Math.max(0, groupLeft - 2), Math.max(0, railContainer.y - topHitMargin),
                                      groupWidth + 4, effectiveBarHeight + topHitMargin + 8)
    onRailRegionChanged: backend.setInputRegion(thermoWindow, [railRegion])
    Component.onCompleted: {
        backend.configureDesktopWindow(thermoWindow)
        backend.setInputRegion(thermoWindow, [railRegion])
        visible = true
    }

    property bool rawHover: mouseArea.containsMouse || (badgeWindow.visible && (pillMouseArea.containsMouse || corridorMouse.containsMouse))
    property bool isHovered: rawHover || hoverGraceTimer.running
    property bool isDragging: false
    property bool pressed: false
    property real pressY: 0
    property real originalY: 0
    readonly property bool badgeWanted: isHovered || pressed || backend.inspecting
    function beginPress(area, mouse) {
        const point = area.mapToGlobal(mouse.x, mouse.y)
        pressY = point.y
        originalY = railContainer.y
        pressed = true
        isDragging = false
    }
    function movePress(area, mouse) {
        if (!pressed) return
        const point = area.mapToGlobal(mouse.x, mouse.y)
        const dy = point.y - pressY
        if (!isDragging && Math.abs(dy) >= Qt.styleHints.startDragDistance) isDragging = true
        if (isDragging) railContainer.y = clampRailY(originalY + dy)
    }
    function finishPress(cancelled) {
        if (!pressed) return
        const dragged = isDragging
        if (dragged && !cancelled) backend.setRailY(Math.round(railContainer.y))
        railContainer.y = Qt.binding(function() { return thermoWindow.clampRailY(backend.railY) })
        if (dragged && !cancelled) badgeWindow.retainedY = readoutPill.y
        pressed = false
        isDragging = false
        if (!cancelled && !dragged) backend.openSettings()
        if (!rawHover) hoverGraceTimer.restart()
    }
    Timer { id: hoverGraceTimer; interval: 180 }
    onRawHoverChanged: {
        if (rawHover) hoverGraceTimer.stop()
        else if (!pressed) hoverGraceTimer.restart()
    }

    Item {
        id: railContainer
        objectName: "temperatureRail"
        x: thermoWindow.leftEdge ? 2 : thermoWindow.width - width - 2
        y: thermoWindow.clampRailY(backend.railY)

        // Fixed width: the thermometer scale NEVER moves or expands horizontally on hover
        width: backend.idleWidth
        height: thermoWindow.effectiveBarHeight

        RailMarker {
            objectName: "heatRailMarker"
            symbol: "heat"
            lightSurface: thermoWindow.lightRails
            visible: backend.fanRailEnabled
            x: (railContainer.width - width) / 2 + (thermoWindow.leftEdge ? 1 : -1) * Math.max(0, 3 - railContainer.width / 2)
            y: -15
        }

        // Light tracks stay quiet against wallpaper; thermal colour belongs to the fill.
        Rectangle {
            id: trackBackground
            anchors.fill: parent
            radius: width / 2
            color: thermoWindow.railSurface
            border.color: backend.temperatureStatus === "live" ? thermoWindow.railOutline : thermoWindow.warningColor
            border.width: 1

            // Liquid temperature column filling from bottom
            Item {
                id: fillClip
                objectName: "temperatureFill"
                visible: backend.temperatureStatus === "live"
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: Math.max(parent.width, Math.min(parent.height, parent.height * backend.normalizedTemperature))
                clip: true

                Behavior on height {
                    enabled: !backend.reducedMotion
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
                    height: thermoWindow.lightRails ? 2 : Math.max(3, parent.width)
                    radius: height / 2
                    color: backend.currentColor

                    SequentialAnimation on opacity {
                        objectName: "temperaturePulse"
                        running: !backend.reducedMotion
                        loops: Animation.Infinite
                        NumberAnimation { to: thermoWindow.lightRails ? 0.88 : 0.65; duration: 2400; easing.type: Easing.InOutSine }
                        NumberAnimation { to: 1.00; duration: 2400; easing.type: Easing.InOutSine }
                    }
                }
            }
        }

        // Optional companion fan rail. Shares the rail's position and input area.
        Rectangle {
            objectName: "fanRail"
            RailMarker {
                objectName: "fanRailMarker"
                symbol: "fan"
                lightSurface: thermoWindow.lightRails
                x: (parent.width - width) / 2 + (thermoWindow.leftEdge ? 1 : -1) * Math.max(0, 3 - parent.width / 2)
                y: -15
            }
            visible: backend.fanRailEnabled
            x: thermoWindow.leftEdge ? railContainer.width + thermoWindow.railGap : -width - thermoWindow.railGap
            width: backend.idleWidth
            height: parent.height
            radius: 3
            color: thermoWindow.railSurface
            border.width: 1
            border.color: backend.fanLevelAvailable ? thermoWindow.fanOutline : thermoWindow.warningColor
            Rectangle {
                objectName: "fanFill"
                visible: backend.fanLevelAvailable
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: parent.height * backend.fanLevel
                radius: 3
                gradient: Gradient {
                    GradientStop { position: 0; color: "#bae6fd" }
                    GradientStop { position: 0.5; color: "#38bdf8" }
                    GradientStop { position: 1; color: "#1e3a8a" }
                }
                Behavior on height { enabled: !backend.reducedMotion; NumberAnimation { duration: 650; easing.type: Easing.OutCubic } }
            }
        }

    }

    Window {
        id: badgeWindow
        objectName: "badgeWindow"
        transientParent: null
        width: 320
        color: "transparent"
        flags: Qt.FramelessWindowHint | Qt.WindowDoesNotAcceptFocus
        visible: false
        LayerShell.Window.layer: LayerShell.Window.LayerOverlay
        LayerShell.Window.anchors: (thermoWindow.leftEdge ? LayerShell.Window.AnchorLeft : LayerShell.Window.AnchorRight) | LayerShell.Window.AnchorTop | LayerShell.Window.AnchorBottom
        LayerShell.Window.exclusionZone: 0
        LayerShell.Window.keyboardInteractivity: LayerShell.Window.KeyboardInteractivityNone
        LayerShell.Window.scope: "thermo-readout"
        property real retainedHeight: 0
        property real retainedY: 0
        property real targetOpacity: 0
        property rect badgeRegion: Qt.rect(readoutPill.x, readoutPill.y, readoutPill.width, readoutPill.height)
        property rect corridorRegion: Qt.rect(thermoWindow.leftEdge ? 2 : readoutPill.x + readoutPill.width,
                                              readoutPill.y + readoutPill.height / 2 - 4,
                                              thermoWindow.groupWidth + 8, 8)
        function updateMask() { backend.setInputRegion(badgeWindow, [badgeRegion, corridorRegion]) }
        function reveal() {
            if (!visible) {
                retainedHeight = pillContent.height + 16
                retainedY = Math.max(4, Math.min(height - retainedHeight - 4, thermoWindow.y - badgeWindow.y + railContainer.y + (railContainer.height - retainedHeight) / 2))
            }
            updateMask()
            visible = true
            targetOpacity = 1
        }
        onBadgeRegionChanged: { if (visible) updateMask() }
        onCorridorRegionChanged: { if (visible) updateMask() }
        Component.onCompleted: {
            backend.configureDesktopWindow(badgeWindow)
            if (thermoWindow.badgeWanted) reveal()
        }
        Connections {
            target: thermoWindow
            function onBadgeWantedChanged() {
                if (thermoWindow.badgeWanted) badgeWindow.reveal()
                else badgeWindow.targetOpacity = 0
            }
        }
        Connections {
            target: pillContent
            function onHeightChanged() {
                if (badgeWindow.visible) badgeWindow.retainedHeight = Math.max(badgeWindow.retainedHeight, pillContent.height + 16)
            }
        }
        MouseArea {
            id: corridorMouse
            x: badgeWindow.corridorRegion.x; y: badgeWindow.corridorRegion.y
            width: badgeWindow.corridorRegion.width; height: badgeWindow.corridorRegion.height
            hoverEnabled: true
            onPressed: (mouse) => thermoWindow.beginPress(corridorMouse, mouse)
            onPositionChanged: (mouse) => thermoWindow.movePress(corridorMouse, mouse)
            onReleased: thermoWindow.finishPress(!thermoWindow.isDragging && !containsMouse)
            onCanceled: thermoWindow.finishPress(true)
        }
        Item {
            id: readoutPill
            objectName: "readoutBadge"
            x: thermoWindow.leftEdge ? 2 + thermoWindow.groupWidth + 8 : badgeWindow.width - 2 - thermoWindow.groupWidth - 8 - width
            y: Math.max(4, Math.min(badgeWindow.height - height - 4, badgeWindow.retainedY + (thermoWindow.pressed ? railContainer.y - thermoWindow.originalY : 0)))
            width: pillContent.width + 24
            height: Math.max(pillContent.height + 16, badgeWindow.retainedHeight)
            opacity: badgeWindow.targetOpacity
            onOpacityChanged: {
                if (opacity === 0 && !thermoWindow.badgeWanted) {
                    // Unmap first: an empty Qt mask means the whole surface.
                    badgeWindow.visible = false
                    backend.setInputRegion(badgeWindow, [])
                }
            }
            Behavior on opacity {
                enabled: !backend.reducedMotion
                NumberAnimation { duration: 180; easing.type: Easing.OutCubic }
            }

            // Frosted glass card backdrop with ice cold blue outline
            Rectangle {
                anchors.fill: parent
                radius: thermoWindow.followTheme ? Math.min(24, theme.radius) : 12
                // Neutral smoked glass: a bright upper bevel over a dense base.
                gradient: Gradient {
                    GradientStop { position: 0; color: thermoWindow.followTheme ? Qt.lighter(theme.tokens.background, 1.4) : "#f2596069" }
                    GradientStop { position: 0.32; color: thermoWindow.followTheme ? theme.tokens.background : "#f0353b43" }
                    GradientStop { position: 1; color: thermoWindow.followTheme ? Qt.darker(theme.tokens.background, 1.1) : "#f21c2128" }
                }

                Rectangle {
                    anchors.fill: parent
                    anchors.margins: 2
                    radius: thermoWindow.followTheme ? Math.max(0, Math.min(24, theme.radius) - 2) : 10
                    color: "transparent"
                    border.width: 1
                    border.color: thermoWindow.followTheme && theme.tokens.light ? "#25000000" : "#35ffffff"
                }
                Rectangle {
                    anchors.top: parent.top
                    anchors.topMargin: 3
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: parent.width - 28
                    height: 1
                    color: thermoWindow.followTheme && theme.tokens.light ? "#35000000" : "#75ffffff"
                }
                border.color: Qt.rgba(thermoWindow.chromeColor.r, thermoWindow.chromeColor.g, thermoWindow.chromeColor.b, 0.45)
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
                x: thermoWindow.leftEdge ? -8 : parent.width
                anchors.verticalCenter: parent.verticalCenter
                width: 8
                height: 1
                color: Qt.rgba(thermoWindow.chromeColor.r, thermoWindow.chromeColor.g, thermoWindow.chromeColor.b, 0.55)
            }

            Column {
                id: pillContent
                width: 224
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
                        text: backend.formattedTemperature + (backend.temperatureStatus === "stale" ? " *" : "")
                        color: thermoWindow.textColor
                        font.family: thermoWindow.textFamily
                        font.pixelSize: Math.round(16 * thermoWindow.textScale)
                        font.weight: Font.DemiBold
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }

                Text {
                    text: backend.temperatureWarning
                    visible: text.length > 0
                    width: 224
                    wrapMode: Text.WordWrap
                    horizontalAlignment: Text.AlignHCenter
                    color: thermoWindow.warningColor
                    font.family: thermoWindow.textFamily
                    font.pixelSize: Math.round(11 * thermoWindow.textScale)
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                // Sensor name / label
                Text {
                    text: backend.sensorLabel
                    width: 224
                    horizontalAlignment: Text.AlignHCenter
                    color: thermoWindow.mutedColor
                    font.family: thermoWindow.textFamily
                    font.pixelSize: Math.round(10 * thermoWindow.textScale)
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
                        color: thermoWindow.textColor
                        font.family: thermoWindow.textFamily
                        font.pixelSize: Math.round(11 * thermoWindow.textScale)
                        font.features: { "tnum": 1 }
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                }
                Text {
                    visible: backend.fanSpeeds.length === 0
                    text: qsTr("Fan speed unavailable")
                    color: thermoWindow.mutedColor
                    font.family: thermoWindow.textFamily
                    font.pixelSize: Math.round(11 * thermoWindow.textScale)
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                Text {
                    visible: backend.fanRailEnabled && !backend.fanLevelAvailable
                    text: backend.fanRailDescription
                    width: 224
                    wrapMode: Text.WordWrap
                    horizontalAlignment: Text.AlignHCenter
                    color: thermoWindow.warningColor
                    font.family: thermoWindow.textFamily
                    font.pixelSize: Math.round(10 * thermoWindow.textScale)
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                // Hint for user in cold blue accent
                Text {
                    width: 224
                    elide: Text.ElideRight
                    horizontalAlignment: Text.AlignHCenter
                    text: thermoWindow.isDragging ? qsTr("Moving position...") : qsTr("Drag to move • Click for settings")
                    color: Qt.rgba(thermoWindow.chromeColor.r, thermoWindow.chromeColor.g, thermoWindow.chromeColor.b, 0.7)
                    font.family: thermoWindow.textFamily
                    font.pixelSize: Math.round(9 * thermoWindow.textScale)
                    anchors.horizontalCenter: parent.horizontalCenter
                }
            }

            MouseArea {
                id: pillMouseArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: thermoWindow.isDragging ? Qt.ClosedHandCursor : Qt.PointingHandCursor
                onPressed: (mouse) => thermoWindow.beginPress(pillMouseArea, mouse)
                onPositionChanged: (mouse) => thermoWindow.movePress(pillMouseArea, mouse)
                onReleased: thermoWindow.finishPress(!thermoWindow.isDragging && !containsMouse)
                onCanceled: thermoWindow.finishPress(true)
            }
        }
    }
    MouseArea {
        id: mouseArea
        objectName: "railMouseArea"
        x: thermoWindow.railRegion.x; y: thermoWindow.railRegion.y
        width: thermoWindow.railRegion.width; height: thermoWindow.railRegion.height
        hoverEnabled: true
        preventStealing: true
        cursorShape: thermoWindow.isDragging ? Qt.ClosedHandCursor : Qt.PointingHandCursor
        onPressed: (mouse) => thermoWindow.beginPress(mouseArea, mouse)
        onPositionChanged: (mouse) => thermoWindow.movePress(mouseArea, mouse)
        onReleased: thermoWindow.finishPress(!thermoWindow.isDragging && !containsMouse)
        onCanceled: thermoWindow.finishPress(true)
    }
}
