import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Control {
    id: section
    required property var style
    property string title: ""
    default property alias contents: fields.data
    padding: style.cardPadding
    implicitHeight: implicitContentHeight + topPadding + bottomPadding
    background: Rectangle {
        color: section.style.panelColor
        radius: section.style.cardRadius
        border.color: section.style.outlineColor
        border.width: 1
    }
    contentItem: ColumnLayout {
        spacing: section.style.fieldSpacing
        Text {
            visible: section.title.length > 0
            text: section.title
            color: section.style.textColor
            font.family: section.style.font.family
            font.pixelSize: Math.round(14 * section.style.textScale)
            font.weight: Font.DemiBold
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
        ColumnLayout {
            id: fields
            spacing: section.style.fieldSpacing
            Layout.fillWidth: true
        }
    }
}
