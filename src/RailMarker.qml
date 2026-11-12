import QtQuick
import QtQuick.Shapes

// Small vector symbols remain sharp at fractional and high-DPI scaling.
Item {
    id: marker
    width: 8
    height: 8
    property string symbol: "fan"
    property color ink: symbol === "fan" ? "#38bdf8" : "#fb923c"

    Rectangle {
        anchors.fill: parent
        anchors.margins: -1
        radius: width / 2
        color: "#dc141720"
    }
    Item {
        width: 12
        height: 12
        scale: marker.width / 12
        transformOrigin: Item.TopLeft
        Repeater {
            model: marker.symbol === "fan" ? 3 : 0
            Shape {
                required property int index
                width: 12
                height: 12
                rotation: index * 120
                ShapePath {
                    strokeColor: "transparent"
                    fillColor: marker.ink
                    PathSvg { path: "M6 5 C3 2 5 0 7 1 C10 2 8 4 6 5 Z" }
                }
            }
        }
        Rectangle {
            visible: marker.symbol === "fan"
            x: 4.5
            y: 4.5
            width: 3
            height: 3
            radius: 1.5
            color: marker.ink
        }
        Shape {
            visible: marker.symbol === "heat"
            width: 12
            height: 12
            ShapePath {
                strokeColor: "transparent"
                fillColor: marker.ink
                PathSvg { path: "M6 0 C6 3 10 4 10 7 C10 10 8 12 6 12 C3 12 2 10 2 8 C2 6 4 5 4 3 C5 4 5 5 5 6 C7 5 7 3 6 0 Z" }
            }
        }
    }
}
