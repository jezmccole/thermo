import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

ApplicationWindow {
    id: settingsWin
    width: 600
    height: 640
    minimumWidth: 520
    minimumHeight: 560
    visible: backend.settingsVisible
    title: qsTr("Thermo — Settings")
    property bool followTheme: backend.appearanceMode === "omarchy"
    property real textScale: followTheme ? theme.tokens.baseSize / 12 : 1
    property color textColor: followTheme ? theme.tokens.foreground : "#f0f6fc"
    property color mutedColor: followTheme ? theme.tokens.muted : "#8b949e"
    property color panelColor: followTheme ? theme.tokens.panel : "#161b22"
    property color outlineColor: followTheme ? theme.tokens.border : "#30363d"
    property color warningColor: followTheme && theme.tokens.light ? "#92400e" : "#fbbf24"
    property color accentColor: followTheme ? theme.tokens.accent : "#38bdf8"
    color: followTheme ? theme.tokens.background : "#0e1116"
    font.family: followTheme ? theme.tokens.fontFamily : Qt.application.font.family

    Material.theme: followTheme && theme.tokens.light ? Material.Light : Material.Dark
    Material.accent: accentColor
    Material.foreground: textColor
    Material.background: panelColor
    Material.roundedScale: theme.radius

    onClosing: (close) => {
        close.accepted = false
        backend.closeSettings()
    }

    Shortcut { sequence: "Esc"; onActivated: backend.closeSettings() }
    Shortcut { sequence: "Q"; onActivated: backend.closeSettings() }

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true

        ColumnLayout {
            width: parent.width
            spacing: 16
            Layout.margins: 24

            // Top Header Banner
            Rectangle {
                Layout.fillWidth: true
                height: headerContent.implicitHeight + 36
                radius: Math.max(8, theme.radius)
                color: panelColor
                border.color: outlineColor
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 18
                    spacing: 16

                    // Glowing temperature badge
                    Rectangle {
                        width: 64
                        height: 64
                        radius: Math.max(8, theme.radius)
                        color: settingsWin.color
                        border.color: Qt.rgba(backend.currentColor.r, backend.currentColor.g, backend.currentColor.b, 0.4)
                        border.width: 1

                        Text {
                            anchors.centerIn: parent
                            text: backend.formattedTemperature
                            color: backend.currentColor
                            font.family: settingsWin.font.family
                            font.pixelSize: Math.round(15 * settingsWin.textScale)
                            font.bold: true
                        }
                    }

                    ColumnLayout {
                        id: headerContent
                        Layout.fillWidth: true
                        spacing: 3

                        Text {
                            text: qsTr("Thermo")
                            color: textColor
                            font.family: settingsWin.font.family
                            font.pixelSize: Math.round(18 * settingsWin.textScale)
                            font.bold: true
                        }

                        Text {
                            text: backend.sensorLabel
                            color: mutedColor
                            font.family: settingsWin.font.family
                            font.pixelSize: Math.round(12 * settingsWin.textScale)
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }

                        Text {
                            text: backend.temperatureWarning + (backend.temperatureStatus === "stale" ? "\n" + qsTr("Last read: %1").arg(backend.lastReadingAt) : "")
                            visible: backend.temperatureStatus !== "live"
                            color: settingsWin.warningColor
                            font.family: settingsWin.font.family
                            font.pixelSize: Math.round(11 * settingsWin.textScale)
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }

                        // Gradient preview bar
                        Rectangle {
                            Layout.fillWidth: true
                            height: 4
                            radius: 2
                            color: panelColor

                            Rectangle {
                                visible: backend.temperatureStatus === "live"
                                width: Math.max(4, parent.width * backend.normalizedTemperature)
                                height: parent.height
                                radius: 2
                                color: backend.currentColor

                                Behavior on width {
                                    NumberAnimation { duration: 400; easing.type: Easing.OutCubic }
                                }
                            }
                        }
                    }
                }
            }

            // Card 1: Sensor Selection
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: sensorLayout.implicitHeight + 32
                radius: Math.max(8, theme.radius)
                color: panelColor
                border.color: outlineColor
                border.width: 1

                ColumnLayout {
                    id: sensorLayout
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 10

                    Text {
                        text: qsTr("Monitored Thermometer")
                        color: textColor
                        font.family: settingsWin.font.family
                        font.pixelSize: Math.round(14 * settingsWin.textScale)
                        font.bold: true
                    }

                    Text {
                        text: qsTr("Select which MacBook hardware sensor to display. Default is Central Processor Proximity (TC0P).")
                        color: mutedColor
                        font.family: settingsWin.font.family
                        font.pixelSize: Math.round(11 * settingsWin.textScale)
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        ComboBox {
                            id: sensorCombo
                            Layout.fillWidth: true
                            model: backend.availableSensors
                            textRole: "label"

                            Component.onCompleted: {
                                updateSelectedIndex()
                            }

                            function updateSelectedIndex() {
                                currentIndex = -1
                                for (var i = 0; i < model.length; ++i) {
                                    if (model[i].id === backend.sensorId) {
                                        currentIndex = i
                                        break
                                    }
                                }
                            }

                            Connections {
                                target: backend
                                function onSensorChanged() { sensorCombo.updateSelectedIndex() }
                                function onSensorsUpdated() { sensorCombo.updateSelectedIndex() }
                            }

                            onActivated: (index) => {
                                if (index >= 0 && index < model.length) {
                                    backend.setSensorId(model[index].id)
                                }
                            }

                            delegate: ItemDelegate {
                                width: sensorCombo.width
                                contentItem: RowLayout {
                                    spacing: 8
                                    Text {
                                        text: modelData.label
                                        color: textColor
                                        font.family: settingsWin.font.family
                                        font.pixelSize: Math.round(12 * settingsWin.textScale)
                                        elide: Text.ElideRight
                                        Layout.fillWidth: true
                                    }
                                    Text {
                                        text: modelData.tempString
                                        color: accentColor
                                        font.family: settingsWin.font.family
                                        font.pixelSize: Math.round(11 * settingsWin.textScale)
                                        font.bold: true
                                    }
                                }
                                highlighted: sensorCombo.highlightedIndex === index
                            }
                        }

                        Button {
                            text: qsTr("Refresh")
                            onClicked: backend.refreshSensors()
                        }
                    }
                }
            }

            // Card 2: System Autostart
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: autostartLayout.implicitHeight + 32
                radius: Math.max(8, theme.radius)
                color: panelColor
                border.color: outlineColor
                border.width: 1

                RowLayout {
                    id: autostartLayout
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 16

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 4

                        Text {
                            text: qsTr("Launch on Startup")
                            color: textColor
                            font.family: settingsWin.font.family
                            font.pixelSize: Math.round(14 * settingsWin.textScale)
                            font.bold: true
                        }

                        Text {
                            text: qsTr("Start Thermo automatically in the background when your operating system loads.")
                            color: mutedColor
                            font.family: settingsWin.font.family
                            font.pixelSize: Math.round(11 * settingsWin.textScale)
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }
                    }

                    Switch {
                        checked: backend.autostart
                        onToggled: backend.setAutostart(checked)
                    }
                }
            }

            // Card 3: Appearance & Dimensions
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: displayLayout.implicitHeight + 32
                radius: Math.max(8, theme.radius)
                color: panelColor
                border.color: outlineColor
                border.width: 1

                ColumnLayout {
                    id: displayLayout
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 14

                    Text {
                        text: qsTr("Display & Behavior")
                        color: textColor
                        font.family: settingsWin.font.family
                        font.pixelSize: Math.round(14 * settingsWin.textScale)
                        font.bold: true
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Text {
                            text: qsTr("Appearance:")
                            color: textColor
                            Layout.fillWidth: true
                        }
                        ComboBox {
                            implicitWidth: 190
                            model: [qsTr("Follow Omarchy"), qsTr("Classic Glass")]
                            currentIndex: settingsWin.followTheme ? 0 : 1
                            onActivated: (index) => backend.setAppearanceMode(index === 0 ? "omarchy" : "classic")
                        }
                    }
                    Switch {
                        text: qsTr("Show companion fan rail")
                        checked: backend.fanRailEnabled
                        onToggled: backend.setFanRailEnabled(checked)
                    }
                    Text {
                        text: backend.fanRailDescription
                        visible: backend.fanRailEnabled
                        color: backend.fanLevelAvailable ? mutedColor : settingsWin.warningColor
                        font.family: settingsWin.font.family
                        font.pixelSize: Math.round(11 * settingsWin.textScale)
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }

                    // Unit Selection
                    RowLayout {
                        Layout.fillWidth: true
                        Text {
                            text: qsTr("Temperature Unit:")
                            color: textColor
                            font.family: settingsWin.font.family
                            font.pixelSize: Math.round(12 * settingsWin.textScale)
                            Layout.fillWidth: true
                        }
                        RadioButton {
                            text: "Celsius (°C)"
                            checked: backend.unit === "C"
                            onClicked: backend.setUnit("C")
                        }
                        RadioButton {
                            text: "Fahrenheit (°F)"
                            checked: backend.unit === "F"
                            onClicked: backend.setUnit("F")
                        }
                    }

                    // Idle Width
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2
                        RowLayout {
                            Layout.fillWidth: true
                            Text {
                                text: qsTr("Resting Width:")
                                color: textColor
                                font.family: settingsWin.font.family
                                font.pixelSize: Math.round(12 * settingsWin.textScale)
                            }
                            Item { Layout.fillWidth: true }
                            Text {
                                text: backend.idleWidth + " px"
                                color: accentColor
                                font.family: settingsWin.font.family
                                font.pixelSize: Math.round(12 * settingsWin.textScale)
                                font.bold: true
                            }
                        }
                        Slider {
                            Layout.fillWidth: true
                            from: 2
                            to: 10
                            stepSize: 1
                            value: backend.idleWidth
                            onMoved: backend.setIdleWidth(value)
                        }
                    }

                    // Bar Height
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2
                        RowLayout {
                            Layout.fillWidth: true
                            Text {
                                text: qsTr("Thermometer Height:")
                                color: textColor
                                font.family: settingsWin.font.family
                                font.pixelSize: Math.round(12 * settingsWin.textScale)
                            }
                            Item { Layout.fillWidth: true }
                            Text {
                                text: backend.barHeight + " px"
                                color: accentColor
                                font.family: settingsWin.font.family
                                font.pixelSize: Math.round(12 * settingsWin.textScale)
                                font.bold: true
                            }
                        }
                        Slider {
                            Layout.fillWidth: true
                            from: 80
                            to: 650
                            stepSize: 1
                            value: backend.barHeight
                            onMoved: backend.setBarHeight(value)
                        }
                    }

                    // Temperature scale range calibration
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2
                        RowLayout {
                            Layout.fillWidth: true
                            Text {
                                text: qsTr("Temperature Scale Range:")
                                color: textColor
                                font.family: settingsWin.font.family
                                font.pixelSize: Math.round(12 * settingsWin.textScale)
                            }
                            Item { Layout.fillWidth: true }
                            Text {
                                text: Math.round(backend.minTemperature) + "°C – " + Math.round(backend.maxTemperature) + "°C"
                                color: accentColor
                                font.family: settingsWin.font.family
                                font.pixelSize: Math.round(12 * settingsWin.textScale)
                                font.bold: true
                            }
                        }
                        Text {
                            text: qsTr("Adjust minimum (blue) and maximum (red) threshold bounds.")
                            color: mutedColor
                            font.family: settingsWin.font.family
                            font.pixelSize: Math.round(11 * settingsWin.textScale)
                        }
                    }
                }
            }

            // Product attribution and project information.
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 6
                Text {
                    text: qsTr("About Thermo")
                    color: textColor
                    font.family: settingsWin.font.family
                    font.pixelSize: Math.round(14 * settingsWin.textScale)
                    font.bold: true
                }
                Text {
                    text: qsTr("Created by Kairos Medical Informatics Ltd")
                    color: textColor
                    font.family: settingsWin.font.family
                    font.pixelSize: Math.round(12 * settingsWin.textScale)
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
                Text {
                    text: qsTr("Version %1 • MIT licence").arg(Qt.application.version)
                    color: mutedColor
                    font.family: settingsWin.font.family
                    font.pixelSize: Math.round(12 * settingsWin.textScale)
                }
                Button {
                    text: qsTr("Project and contributions on GitHub")
                    onClicked: Qt.openUrlExternally("https://github.com/jezmccole/thermo")
                }
            }

            // Bottom Buttons
            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                Button {
                    text: qsTr("Quit Thermo")
                    Material.background: panelColor
                    onClicked: backend.quitApp()
                }

                Item { Layout.fillWidth: true }

                Button {
                    text: qsTr("Close")
                    highlighted: true
                    onClicked: backend.closeSettings()
                }
            }
        }
    }
}
