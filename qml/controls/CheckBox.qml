import QtQuick
import QtQuick.Controls
import QtQuick.Templates as T
import QtQuick.Layouts
import TestRequests 1.0

T.CheckBox {
    id: root
    hoverEnabled: true

    // We will draw everything ourselves, so kill template's built-in layout pieces
    indicator: null
    background: null

    spacing: 10
    padding: 0
    leftPadding: 0
    rightPadding: 0
    topPadding: 0
    bottomPadding: 0

    // Keep good sizing for layouts
    TextMetrics {
        id: metrics
        text: root.text
        font.family: Style.fontSecondary
        font.pixelSize: Style.fontSm
    }
    implicitHeight: 22
    implicitWidth: 20 + spacing + Math.ceil(metrics.width)

    // Entire visual tree lives here (OnceUI style)
    contentItem: RowLayout {
        id: row
        spacing: root.spacing

        // Indicator square
        Item {
            Layout.preferredWidth: 20
            Layout.preferredHeight: 20

            Rectangle {
                id: box
                anchors.fill: parent
                radius: 6
                color: root.enabled ? Style.panel2 : Qt.rgba(Style.panel2.r, Style.panel2.g, Style.panel2.b, 0.55)

                border.width: Style.borderWidth
                border.color: root.activeFocus ? Style.focus
                             : (root.hovered ? Qt.rgba(Style.focus.r, Style.focus.g, Style.focus.b, 0.40)
                                             : Style.border)
                Behavior on border.color { ColorAnimation { duration: 140 } }

                // checked fill
                Rectangle {
                    anchors.fill: parent
                    radius: box.radius
                    color: Style.accent
                    opacity: root.checked ? 1 : 0
                    Behavior on opacity { NumberAnimation { duration: 120 } }
                }

                // check icon
                Text {
                    anchors.centerIn: parent
                    text: root.checked ? "check" : ""
                    font.family: Style.iconFontFamily
                    font.pixelSize: 16
                    color: Style.panel

                    opacity: root.checked ? 1 : 0
                    scale: root.checked ? 1 : 0.90
                    Behavior on opacity { NumberAnimation { duration: 120 } }
                    Behavior on scale { NumberAnimation { duration: 180; easing.type: Easing.OutBack } }
                }

                // subtle sheen
                Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    height: parent.height * 0.45
                    radius: box.radius
                    color: Qt.rgba(1, 1, 1, root.hovered && root.enabled ? 0.10 : 0.06)
                    Behavior on color { ColorAnimation { duration: 140 } }
                }

                // micro press animation
                scale: root.down ? 0.96 : 1.0
                Behavior on scale { NumberAnimation { duration: 90; easing.type: Easing.OutCubic } }
            }
        }

        // Label (cannot clip anymore because it's in our own layout)
        Label {
            text: root.text
            font.family: Style.fontSecondary
            font.pixelSize: Style.fontSm
            color: root.enabled ? Style.text : Qt.rgba(Style.text.r, Style.text.g, Style.text.b, 0.55)
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
            wrapMode: Text.NoWrap
            Layout.alignment: Qt.AlignVCenter
        }
    }
}
