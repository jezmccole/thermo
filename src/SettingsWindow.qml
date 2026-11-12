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
    color: "#0e1116"

    Material.theme: Material.Dark
    Material.accent: theme.accent
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
                height: 130
                radius: Math.max(8, theme.radius)
                color: "#161b22"
                border.color: "#30363d"
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
                        color: "#0d1117"
                        border.color: Qt.rgba(backend.currentColor.r, backend.currentColor.g, backend.currentColor.b, 0.4)
                        border.width: 1

                        Text {
                            anchors.centerIn: parent
                            text: backend.formattedTemperature
                            color: backend.currentColor
                            font.pixelSize: 15
                            font.bold: true
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 3

                        Text {
                            text: qsTr("Thermo")
                            color: "#f0f6fc"
                            font.pixelSize: 18
                            font.bold: true
                        }

                        Text {
                            text: backend.sensorLabel
                            color: "#8b949e"
                            font.pixelSize: 12
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }

                        Text {
                            text: backend.temperatureWarning + (backend.temperatureStatus === "stale" ? "\n" + qsTr("Last read: %1").arg(backend.lastReadingAt) : "")
                            visible: backend.temperatureStatus !== "live"
                            color: "#fbbf24"
                            font.pixelSize: 11
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }

                        // Gradient preview bar
                        Rectangle {
                            Layout.fillWidth: true
                            height: 4
                            radius: 2
                            color: "#21262d"

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
                color: "#161b22"
                border.color: "#30363d"
                border.width: 1

                ColumnLayout {
                    id: sensorLayout
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 10

                    Text {
                        text: qsTr("Monitored Thermometer")
                        color: "#f0f6fc"
                        font.pixelSize: 14
                        font.bold: true
                    }

                    Text {
                        text: qsTr("Select which MacBook hardware sensor to display. Default is Central Processor Proximity (TC0P).")
                        color: "#8b949e"
                        font.pixelSize: 11
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
                                        color: "#f0f6fc"
                                        font.pixelSize: 12
                                        elide: Text.ElideRight
                                        Layout.fillWidth: true
                                    }
                                    Text {
                                        text: modelData.tempString
                                        color: theme.accent
                                        font.pixelSize: 11
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
                color: "#161b22"
                border.color: "#30363d"
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
                            color: "#f0f6fc"
                            font.pixelSize: 14
                            font.bold: true
                        }

                        Text {
                            text: qsTr("Start Thermo automatically in the background when your operating system loads.")
                            color: "#8b949e"
                            font.pixelSize: 11
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
                color: "#161b22"
                border.color: "#30363d"
                border.width: 1

                ColumnLayout {
                    id: displayLayout
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 14

                    Text {
                        text: qsTr("Display & Behavior")
                        color: "#f0f6fc"
                        font.pixelSize: 14
                        font.bold: true
                    }

                    // Unit Selection
                    RowLayout {
                        Layout.fillWidth: true
                        Text {
                            text: qsTr("Temperature Unit:")
                            color: "#c9d1d9"
                            font.pixelSize: 12
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
                                color: "#c9d1d9"
                                font.pixelSize: 12
                            }
                            Item { Layout.fillWidth: true }
                            Text {
                                text: backend.idleWidth + " px"
                                color: theme.accent
                                font.pixelSize: 12
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
                                color: "#c9d1d9"
                                font.pixelSize: 12
                            }
                            Item { Layout.fillWidth: true }
                            Text {
                                text: backend.barHeight + " px"
                                color: theme.accent
                                font.pixelSize: 12
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
                                color: "#c9d1d9"
                                font.pixelSize: 12
                            }
                            Item { Layout.fillWidth: true }
                            Text {
                                text: Math.round(backend.minTemperature) + "°C – " + Math.round(backend.maxTemperature) + "°C"
                                color: theme.accent
                                font.pixelSize: 12
                                font.bold: true
                            }
                        }
                        Text {
                            text: qsTr("Adjust minimum (blue) and maximum (red) threshold bounds.")
                            color: "#8b949e"
                            font.pixelSize: 11
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
                    color: "#f0f6fc"
                    font.pixelSize: 14
                    font.bold: true
                }
                Text {
                    text: qsTr("Created by Kairos Medical Informatics Ltd")
                    color: "#c9d1d9"
                    font.pixelSize: 12
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
                Text {
                    text: qsTr("Version %1 • MIT licence").arg(Qt.application.version)
                    color: "#8b949e"
                    font.pixelSize: 12
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
                    Material.background: "#21262d"
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
