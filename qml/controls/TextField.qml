import QtQuick
import QtQuick.Controls
import QtQuick.Templates as T
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects
import TestRequests 1.0

T.TextField {
    id: root

    implicitHeight: 42
    implicitWidth: 220

    hoverEnabled: true
    selectByMouse: true

    leftPadding: 14
    rightPadding: clearBtn.visible ? 40 : 14
    topPadding: 10
    bottomPadding: 10

    font.family: Style.fontPrimary
    font.pixelSize: Style.fontMd
    color: Style.text
    placeholderTextColor: Style.subText

    readonly property color _bg: Style.panel2
    readonly property color _borderIdle: Style.border
    readonly property color _borderHover: Qt.rgba(Style.focus.r, Style.focus.g, Style.focus.b, 0.40)
    readonly property color _borderFocus: Style.focus

    // smooth "ring intensity"
    property real _ring: activeFocus ? 1.0 : (hovered ? 0.55 : 0.0)
    Behavior on _ring { NumberAnimation { duration: 160; easing.type: Easing.InOutQuad } }

    background: Item {
        anchors.fill: parent

        // subtle shadow
        DropShadow {
            anchors.fill: bg
            source: bg
            verticalOffset: root.activeFocus ? 3 : 2
            radius: root.activeFocus ? 12 : 10
            samples: 25
            color: Qt.rgba(0, 0, 0, root.activeFocus ? 0.16 : 0.10)
            opacity: root.enabled ? 1 : 0.55
        }

        Rectangle {
            id: bg
            anchors.fill: parent
            radius: Style.radiusSm

            color: root.enabled ? root._bg : Qt.rgba(root._bg.r, root._bg.g, root._bg.b, 0.55)

            border.width: Style.borderWidth
            border.color: root.activeFocus ? root._borderFocus
                         : (root.hovered ? root._borderHover : root._borderIdle)
            Behavior on border.color { ColorAnimation { duration: 160; easing.type: Easing.InOutQuad } }

            // focus ring
            Rectangle {
                anchors.fill: parent
                radius: bg.radius
                color: "transparent"
                border.width: 2
                border.color: Qt.rgba(Style.focus.r, Style.focus.g, Style.focus.b, 0.55 * root._ring)
                visible: root._ring > 0.01
                opacity: root._ring
                Behavior on opacity { NumberAnimation { duration: 160 } }
            }

            // top highlight (glass)
            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                height: parent.height * 0.45
                radius: bg.radius
                color: Qt.rgba(1, 1, 1, root.enabled ? 0.10 : 0.06)
            }
        }
    }

    // Placeholder overlay (restores placeholder for template-based field)
    Label {
        id: placeholder
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: root.leftPadding
        anchors.rightMargin: root.rightPadding
        anchors.verticalCenter: parent.verticalCenter

        text: root.placeholderText
        visible: root.text.length === 0 && !root.activeFocus
                 && (!root.inputMethodComposing) // safe
        color: root.placeholderTextColor
        font.family: root.font.family
        font.pixelSize: root.font.pixelSize
        elide: Text.ElideRight
    }


    // Clear button overlay (doesn't require contentItem override)
    Rectangle {
        id: clearBtn
        width: 22
        height: 22
        radius: 11
        anchors.right: parent.right
        anchors.rightMargin: 10
        anchors.verticalCenter: parent.verticalCenter

        visible: root.enabled && !root.readOnly && root.text.length > 0 && (root.activeFocus || root.hovered)

        color: Qt.rgba(Style.text.r, Style.text.g, Style.text.b, clearMouse.pressed ? 0.12 : 0.06)
        border.width: 1
        border.color: Qt.rgba(Style.text.r, Style.text.g, Style.text.b, 0.10)

        opacity: visible ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: 120 } }
        scale: clearMouse.pressed ? 0.96 : 1.0
        Behavior on scale { NumberAnimation { duration: 90; easing.type: Easing.OutCubic } }

        Text {
            anchors.centerIn: parent
            text: "×"
            font.family: Style.fontPrimaryBold
            font.pixelSize: 16
            color: Qt.rgba(Style.text.r, Style.text.g, Style.text.b, 0.60)
        }

        MouseArea {
            id: clearMouse
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: root.clear()
        }
    }
}
