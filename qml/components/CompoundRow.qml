import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects
import TestRequests 1.0
import "../controls"
import "../components"

Item {
    id: root
    property string name: ""
    property string statusText: ""
    property bool selectable: true
    property bool checked: false

    signal toggled(bool checked)

    // Delegate footprint (ListView uses this)
    implicitHeight: 56
    width: 300

    property bool hovered: false

    // Inner card that we animate
    Item {
        id: lift
        anchors.fill: parent

        // animate the INNER item, not the delegate
        y: root.hovered ? -1 : 0
        Behavior on y { NumberAnimation { duration: 140; easing.type: Easing.OutCubic } }

        scale: root.hovered ? 1.005 : 1.0
        Behavior on scale { NumberAnimation { duration: 140; easing.type: Easing.OutCubic } }

        Rectangle {
            id: card
            anchors.fill: parent
            radius: Style.radiusLg
            color: Style.panel

            border.width: Style.borderWidth
            border.color: root.hovered
                          ? Qt.rgba(Style.focus.r, Style.focus.g, Style.focus.b, 0.30)
                          : Style.border
            Behavior on border.color { ColorAnimation { duration: 140 } }

            layer.enabled: true
            layer.effect: DropShadow {
                verticalOffset: root.hovered ? 4 : 2
                radius: root.hovered ? 16 : 12
                samples: 25
                color: Qt.rgba(0, 0, 0, root.hovered ? 0.16 : 0.12)
            }

            RowLayout {
                anchors.fill: parent
                anchors.margins: Style.padLg
                spacing: Style.padSm

                // Make checkbox not double-toggle with row click:
                CheckBox {
                    id: cb
                    enabled: root.selectable
                    opacity: root.selectable ? 1.0 : 0.4
                    checked: root.checked
                    onToggled: root.toggled(checked)
                    Layout.alignment: Qt.AlignVCenter

                    // prevent row MouseArea from also toggling when clicking checkbox
                    MouseArea {
                        anchors.fill: parent
                        acceptedButtons: Qt.LeftButton
                        onClicked: (mouse) => mouse.accepted = true
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2

                    Label {
                        text: root.name
                        color: Style.text
                        opacity: root.selectable ? 1.0 : 0.4
                        font.family: Style.fontSecondaryBold
                        font.pixelSize: Style.fontMd
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                    }

                    Label {
                        text: (root.statusText === "Pending" && !root.selectable)
                              ? "Inventory solutions required before sending"
                              : root.statusText
                        color: Style.subText
                        opacity: root.selectable ? 1.0 : 0.4
                        font.family: Style.fontSecondary
                        font.pixelSize: Style.fontSm
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                    }
                }

                StatusPill {
                    text: (root.statusText === "Pending" && !root.selectable)
                          ? "Not in Inventory"
                          : root.statusText
                    Layout.alignment: Qt.AlignVCenter
                    visible: root.statusText && root.statusText.length > 0
                }

                Text {
                    text: "chevron_right"
                    font.family: Style.iconFontFamily
                    font.pixelSize: 18
                    color: Style.subText
                    opacity: root.selectable ? 0.7 : 0.2
                    Layout.alignment: Qt.AlignVCenter
                }
            }

            MouseArea {
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: root.selectable ? Qt.PointingHandCursor : Qt.ArrowCursor

                onEntered: root.hovered = true
                onExited: root.hovered = false

                onClicked: {
                    if (!root.selectable) return
                    root.toggled(!root.checked)
                }
            }
        }
    }
}
