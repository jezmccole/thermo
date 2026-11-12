import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

ApplicationWindow {
    id: settingsWin
    width: 640
    height: 720
    minimumWidth: 520
    minimumHeight: 480
    visible: backend.settingsVisible
    title: qsTr("Thermo — Settings")
    property bool followTheme: backend.appearanceMode === "omarchy"
    property real textScale: followTheme ? theme.tokens.baseSize / 12 : 1
    readonly property int pagePadding: Math.round(24 * Math.min(textScale, 1.25))
    readonly property int cardPadding: Math.round(18 * Math.min(textScale, 1.25))
    readonly property int fieldSpacing: Math.round(14 * Math.min(textScale, 1.25))
    readonly property int cardRadius: followTheme ? Math.max(0, Math.min(24, theme.radius)) : 12
    property color textColor: followTheme ? theme.tokens.foreground : "#f0f6fc"
    property color mutedColor: followTheme ? theme.tokens.muted : "#8b949e"
    property color panelColor: followTheme ? theme.tokens.panel : "#161b22"
    property color outlineColor: followTheme ? theme.tokens.border : "#30363d"
    property color warningColor: followTheme && theme.tokens.light ? "#92400e" : "#fbbf24"
    property color accentColor: followTheme ? theme.tokens.accent : "#38bdf8"
    color: followTheme ? theme.tokens.background : "#0e1116"
    font.family: followTheme ? theme.tokens.fontFamily : Qt.application.font.family
    font.pixelSize: Math.round(12 * textScale)
    Material.theme: followTheme && theme.tokens.light ? Material.Light : Material.Dark
    Material.accent: accentColor
    Material.foreground: textColor
    Material.background: panelColor
    Material.roundedScale: theme.radius

    onClosing: (close) => { close.accepted = false; backend.closeSettings() }
    Shortcut { sequence: "Esc"; onActivated: backend.closeSettings() }
    Shortcut { sequence: "Ctrl+W"; onActivated: backend.closeSettings() }
    Shortcut { sequence: "Ctrl+Q"; onActivated: backend.quitApp() }
    Shortcut { sequence: "Ctrl+I"; onActivated: backend.inspect() }
    Connections {
        target: backend
        function onSettingsRequested() { settingsWin.raise(); settingsWin.requestActivate() }
    }
    // Only page descendants participate: popup delegates and footer buttons do
    // not belong to the scrolling page, even when they own keyboard focus.
    onActiveFocusItemChanged: Qt.callLater(ensureFocusVisible)
    function ensureFocusVisible() {
        const item = activeFocusItem
        if (!item) return
        let ancestor = item
        while (ancestor && ancestor !== settingsPage) ancestor = ancestor.parent
        if (!ancestor) return
        const flick = scroll.contentItem
        const pos = item.mapToItem(settingsPage, 0, 0)
        const margin = 12
        if (pos.y < flick.contentY + margin) flick.contentY = Math.max(0, pos.y - margin)
        else if (pos.y + item.height > flick.contentY + flick.height - margin)
            flick.contentY = Math.min(Math.max(0, flick.contentHeight - flick.height), pos.y + item.height - flick.height + margin)
    }

    component Section: SettingsSection {
        style: settingsWin
        Layout.fillWidth: true
    }
    component Label: Text {
        color: settingsWin.textColor
        font.family: settingsWin.font.family
        font.pixelSize: Math.round(12 * settingsWin.textScale)
        wrapMode: Text.WordWrap
        Layout.fillWidth: true
    }
    component Description: Label {
        color: settingsWin.mutedColor
        font.pixelSize: Math.round(11 * settingsWin.textScale)
    }
    component Selector: ComboBox {
        Layout.fillWidth: true
        implicitHeight: Math.max(44, implicitContentHeight + topPadding + bottomPadding)
        // Keep this explicit: wheel events over a closed selector scroll the page.
        wheelEnabled: false
    }
    component ToggleRow: RowLayout {
        id: toggleRow
        property string label
        property bool checked
        signal toggled(bool value)
        spacing: 12
        Layout.fillWidth: true
        Label { text: toggleRow.label }
        Switch {
            checked: toggleRow.checked
            Accessible.name: toggleRow.label
            onToggled: toggleRow.toggled(checked)
        }
    }
    component Dimension: ColumnLayout {
        id: dimension
        property string label
        property int value
        property int minimumValue
        property int maximumValue
        signal moved(int value)
        spacing: 4
        Layout.fillWidth: true
        RowLayout {
            Layout.fillWidth: true
            Label { text: dimension.label }
            Text {
                text: dimension.value + " px"
                color: settingsWin.accentColor
                font.family: settingsWin.font.family
                font.pixelSize: settingsWin.font.pixelSize
                font.weight: Font.DemiBold
            }
        }
        Slider {
            from: dimension.minimumValue
            to: dimension.maximumValue
            stepSize: 1
            value: dimension.value
            Layout.fillWidth: true
            Accessible.name: dimension.label
            onMoved: dimension.moved(Math.round(value))
        }
    }

    ScrollView {
        id: scroll
        objectName: "settingsScroll"
        anchors.fill: parent
        contentWidth: availableWidth
        contentHeight: settingsPage.implicitHeight
        clip: true
        ScrollBar.vertical.policy: ScrollBar.AsNeeded

        Item {
            id: settingsPage
            width: scroll.availableWidth
            implicitHeight: settingsColumn.implicitHeight + 2 * settingsWin.pagePadding

            ColumnLayout {
                id: settingsColumn
                objectName: "settingsColumn"
                x: Math.max(settingsWin.pagePadding, (settingsPage.width - 8 - width) / 2)
                y: settingsWin.pagePadding
                width: Math.min(760, settingsPage.width - 2 * settingsWin.pagePadding - 8)
                spacing: Math.round(20 * Math.min(settingsWin.textScale, 1.25))

                Section {
                    objectName: "summarySection"
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: settingsWin.cardPadding
                        Rectangle {
                            Layout.preferredWidth: Math.max(92, headerTemperature.implicitWidth + 20)
                            Layout.preferredHeight: 72
                            color: settingsWin.color
                            radius: settingsWin.cardRadius
                            border.color: backend.currentColor
                            Text {
                                id: headerTemperature
                                anchors.centerIn: parent
                                text: backend.formattedTemperature
                                color: backend.currentColor
                                font.family: settingsWin.font.family
                                font.pixelSize: Math.round(15 * settingsWin.textScale)
                                font.weight: Font.DemiBold
                            }
                        }
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 6
                            Label {
                                text: qsTr("Thermo")
                                font.pixelSize: Math.round(20 * settingsWin.textScale)
                                font.weight: Font.DemiBold
                            }
                            Description { text: backend.sensorLabel }
                            Description {
                                text: backend.temperatureWarning + (backend.temperatureStatus === "stale" ? "\n" + qsTr("Last read: %1").arg(backend.lastReadingAt) : "")
                                visible: backend.temperatureStatus !== "live"
                                color: settingsWin.warningColor
                            }
                        }
                    }
                }

                Section {
                    title: qsTr("Temperature sensor")
                    Description { text: qsTr("Choose the hardware sensor to display. Central Processor Proximity (TC0P) is preferred when available.") }
                    Selector {
                        id: sensorCombo
                        objectName: "sensorSelector"
                        property var choices: []
                        model: choices
                        function refreshChoices() {
                            if (popup.visible) return
                            choices = backend.availableSensors
                            updateSelectedIndex()
                        }
                        Connections {
                            target: sensorCombo.popup
                            function onClosed() { Qt.callLater(sensorCombo.refreshChoices) }
                        }
                        textRole: "label"
                        Accessible.name: qsTr("Temperature sensor")
                        function updateSelectedIndex() {
                            for (var i = 0; i < model.length; ++i) {
                                if (model[i].id === backend.sensorId) { currentIndex = i; return }
                            }
                            currentIndex = -1
                        }
                        Component.onCompleted: refreshChoices()
                        Connections {
                            target: backend
                            function onSensorChanged() { sensorCombo.refreshChoices() }
                            function onSensorsUpdated() { sensorCombo.refreshChoices() }
                        }
                        onActivated: (index) => { if (index >= 0) backend.setSensorId(model[index].id) }
                        delegate: ItemDelegate {
                            width: sensorCombo.width
                            highlighted: sensorCombo.highlightedIndex === index
                            contentItem: RowLayout {
                                spacing: 12
                                Label { text: modelData.label; elide: Text.ElideRight; wrapMode: Text.NoWrap }
                                Text { text: modelData.tempString; color: settingsWin.accentColor; font: settingsWin.font }
                            }
                        }
                    }
                    Button { text: qsTr("Refresh sensors"); onClicked: backend.refreshSensors() }
                }

                Section {
                    title: qsTr("Desktop placement")
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 6
                        Label { text: qsTr("Monitor") }
                        Selector {
                            id: screenCombo
                            objectName: "screenSelector"
                            property var choices: []
                            model: choices
                            function refreshChoices() {
                                if (popup.visible) return
                                choices = backend.availableScreens
                                updateSelectedIndex()
                            }
                            Connections {
                                target: backend
                                function onScreensChanged() { screenCombo.refreshChoices() }
                            }
                            Connections {
                                target: screenCombo.popup
                                function onClosed() { Qt.callLater(screenCombo.refreshChoices) }
                            }
                            textRole: "label"
                            Accessible.name: qsTr("Monitor")
                            function updateSelectedIndex() {
                                for (var i = 0; i < model.length; ++i) {
                                    if (model[i].name === backend.screenName) { currentIndex = i; return }
                                }
                                currentIndex = 0
                            }
                            Component.onCompleted: refreshChoices()
                            onActivated: (index) => { if (index >= 0) backend.setScreenName(model[index].name) }
                        }
                    }
                    Description { text: backend.screenWarning; visible: text.length > 0; color: settingsWin.warningColor }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 6
                        Label { text: qsTr("Screen edge") }
                        Selector {
                            model: [qsTr("Right edge"), qsTr("Left edge")]
                            currentIndex: backend.edge === "left" ? 1 : 0
                            Accessible.name: qsTr("Screen edge")
                            onActivated: (index) => backend.setEdge(index === 1 ? "left" : "right")
                        }
                    }
                    Description { text: qsTr("Drag either rail or the badge vertically. The rails and readout stay inside the selected display when its size changes.") }
                }

                Section {
                    title: qsTr("Appearance and behaviour")
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 6
                        Label { text: qsTr("Appearance") }
                        Selector {
                            model: [qsTr("Follow Omarchy"), qsTr("Classic Glass")]
                            currentIndex: settingsWin.followTheme ? 0 : 1
                            Accessible.name: qsTr("Appearance")
                            onActivated: (index) => backend.setAppearanceMode(index === 0 ? "omarchy" : "classic")
                        }
                    }
                    ToggleRow {
                        label: qsTr("Reduce motion")
                        checked: backend.reducedMotion
                        onToggled: (value) => backend.setReducedMotion(value)
                    }
                    ToggleRow {
                        label: qsTr("Show companion fan rail")
                        checked: backend.fanRailEnabled
                        onToggled: (value) => backend.setFanRailEnabled(value)
                    }
                    Description {
                        text: backend.fanRailDescription
                        visible: backend.fanRailEnabled
                        color: backend.fanLevelAvailable ? settingsWin.mutedColor : settingsWin.warningColor
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 6
                        Label { text: qsTr("Temperature unit") }
                        Selector {
                            model: [qsTr("Celsius (°C)"), qsTr("Fahrenheit (°F)")]
                            currentIndex: backend.unit === "F" ? 1 : 0
                            Accessible.name: qsTr("Temperature unit")
                            onActivated: (index) => backend.setUnit(index === 1 ? "F" : "C")
                        }
                    }
                }

                Section {
                    title: qsTr("Size")
                    Dimension {
                        label: qsTr("Rail width")
                        value: backend.idleWidth
                        minimumValue: 2
                        maximumValue: 12
                        onMoved: (value) => backend.setIdleWidth(value)
                    }
                    Dimension {
                        label: qsTr("Rail height")
                        value: backend.barHeight
                        minimumValue: 80
                        maximumValue: 750
                        onMoved: (value) => backend.setBarHeight(value)
                    }
                    Description { text: qsTr("Sizes are logical pixels, so display scaling is applied automatically. Height is limited to the available space without changing your saved preference.") }
                    RowLayout {
                        Layout.fillWidth: true
                        Label { text: qsTr("Temperature scale") }
                        Text {
                            objectName: "temperatureScaleLabel"
                            text: backend.unit === "F"
                                ? Math.round(backend.minTemperature * 9 / 5 + 32) + "°F – " + Math.round(backend.maxTemperature * 9 / 5 + 32) + "°F"
                                : Math.round(backend.minTemperature) + "°C – " + Math.round(backend.maxTemperature) + "°C"
                            color: settingsWin.accentColor
                            font: settingsWin.font
                        }
                    }
                    Description { text: qsTr("The colour scale runs from cold blue to hot red.") }
                }

                Section {
                    title: qsTr("Startup")
                    ToggleRow {
                        label: qsTr("Launch on startup")
                        checked: backend.autostart
                        onToggled: (value) => backend.setAutostart(value)
                    }
                    Description { text: qsTr("Start quietly in the background when you log in.") }
                }

                Section {
                    title: qsTr("About Thermo")
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: settingsWin.cardPadding
                        Rectangle {
                            Layout.preferredWidth: 112
                            Layout.preferredHeight: 112
                            Layout.alignment: Qt.AlignTop
                            color: "white"
                            radius: 6
                            Image {
                                objectName: "companyLogo"
                                anchors.fill: parent
                                anchors.margins: 6
                                source: "qrc:/assets/kairos-logo.png"
                                fillMode: Image.PreserveAspectFit
                                smooth: true
                                Accessible.role: Accessible.Graphic
                                Accessible.name: qsTr("Kairos Medical Informatics logo")
                            }
                        }
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 6
                            Label { text: qsTr("Created by Kairos Medical Informatics Ltd") }
                            Description { text: qsTr("Version %1 • MIT licence").arg(Qt.application.version) }
                            Button {
                                objectName: "companyWebsiteLink"
                                text: "kairosmedical.co.uk"
                                Accessible.name: qsTr("Visit the Kairos Medical Informatics website")
                                onClicked: Qt.openUrlExternally("https://kairosmedical.co.uk/")
                            }
                        }
                    }
                    Button { text: qsTr("View project on GitHub"); onClicked: Qt.openUrlExternally("https://github.com/jezmccole/thermo") }
                }
            }
        }
    }

    footer: Pane {
        objectName: "settingsFooter"
        leftPadding: Math.max(settingsWin.pagePadding, (settingsWin.width - 8 - 760) / 2)
        rightPadding: leftPadding + 8
        topPadding: 12
        bottomPadding: 12
        background: Rectangle { color: settingsWin.color }
        contentItem: RowLayout {
            spacing: 12
            Button { text: qsTr("Quit Thermo"); onClicked: backend.quitApp() }
            Button { objectName: "inspectButton"; text: qsTr("Inspect readout"); onClicked: backend.inspect() }
            Item { Layout.fillWidth: true }
            Button { text: qsTr("Close"); highlighted: true; onClicked: backend.closeSettings() }
        }
    }
}
