import QtQuick
import QtQuick.Controls
import QtQuick.Templates as T
import Qt5Compat.GraphicalEffects
import TestRequests 1.0

T.ToolBar {
    id: root
    implicitHeight: 54

    background: Item {
        anchors.fill: parent

        // Very soft shadow
        DropShadow {
            anchors.fill: bg
            source: bg
            verticalOffset: 2
            radius: 12
            samples: 25
            color: Qt.rgba(0, 0, 0, 0.12)
            opacity: 1
        }

        Rectangle {
            id: bg
            anchors.fill: parent
            color: Style.panel
            border.width: Style.borderWidth
            border.color: Style.border
        }

        // Bottom divider accent
        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 1
            color: Style.divider
        }
    }
}
