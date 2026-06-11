import QtQuick
import QtQuick.Templates as T
import QtQuick.Controls
import Qt5Compat.GraphicalEffects
import TestRequests 1.0

T.Button {
    id: root
    implicitHeight: 40
    implicitWidth: Math.max(120, label.implicitWidth + 34)

    // smoother feel
    hoverEnabled: true

    // animate press/hover
    property real _elev:  root.down ? 1 : (root.hovered ? 3 : 2)
    property real _scale: root.down ? 0.985 : 1.0

    scale: _scale
    Behavior on scale { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }

    background: Item {
        anchors.fill: parent

        // shadow (elevation)
        DropShadow {
            anchors.fill: bg
            source: bg
            verticalOffset: root._elev
            radius: 10 + root._elev * 2
            samples: 25
            color: Qt.rgba(0, 0, 0, root.enabled ? 0.20 : 0.10)
            opacity: root.enabled ? 1 : 0.7
        }

        Rectangle {
            id: bg
            anchors.fill: parent
            radius: 999

            // base color logic
            color: !root.enabled
                   ? Qt.rgba(Style.accent.r, Style.accent.g, Style.accent.b, 0.35)
                   : (root.down
                      ? Qt.rgba(Style.accent.r, Style.accent.g, Style.accent.b, 0.85)
                      : Style.accent)

            border.width: Style.borderWidth
            border.color: root.hovered
                          ? Qt.rgba(Style.focus.r, Style.focus.g, Style.focus.b, 0.85)
                          : Qt.rgba(0, 0, 0, 0.0)

            Behavior on color { ColorAnimation { duration: 160; easing.type: Easing.InOutQuad } }
            Behavior on border.color { ColorAnimation { duration: 160; easing.type: Easing.InOutQuad } }

            // glossy highlight (OnceUI-ish)
            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                height: parent.height * 0.55
                radius: 999
                color: Qt.rgba(1, 1, 1, root.enabled ? (root.down ? 0.06 : 0.10) : 0.04)
            }

            // subtle shimmer sweep on hover
            Item {
                anchors.fill: parent
                clip: true
                opacity: root.hovered && root.enabled ? 1 : 0
                Behavior on opacity { NumberAnimation { duration: 140 } }

                Rectangle {
                    id: sheen
                    width: parent.width * 0.45
                    height: parent.height * 2
                    radius: 999
                    color: Qt.rgba(1, 1, 1, 0.10)
                    rotation: 18
                    y: -parent.height * 0.5
                    x: root.hovered ? parent.width + width : -width

                    Behavior on x {
                        NumberAnimation { duration: 550; easing.type: Easing.InOutQuad }
                    }

                    onVisibleChanged: x = -width
                }
            }
        }
    }

    contentItem: Item {
        anchors.fill: parent

        Label {
            id: label
            anchors.centerIn: parent
            text: root.text
            font.family: Style.fontPrimaryBold
            font.pixelSize: Style.fontMd
            color: Style.panel
        }
    }
}
