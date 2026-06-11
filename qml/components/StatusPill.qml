import QtQuick
import QtQuick.Controls
import TestRequests 1.0

Rectangle {
    id: root
    property string text: ""

    radius: 999
    height: 22
    implicitWidth: label.implicitWidth + 16
    border.width: Style.borderWidth
    border.color: Qt.rgba(base.r, base.g, base.b, 0.4)

    readonly property color base: {
        if (text === "Published") return Style.accent
        if (text === "Sent") return Style.accent2
        if (text === "Done") return Style.ok
        if (text === "Pending") return Style.warn
        if (text === "Not in Inventory") return Style.bad
        return Style.accent2
    }

    color: Qt.rgba(base.r, base.g, base.b, 0.12)

    Label {
        id: label
        anchors.centerIn: parent
        text: root.text
        color: Style.isDark ? root.base : Qt.darker(root.base, 1.25)
        font.family: Style.fontSecondaryBold
        font.pixelSize: Style.fontSm
    }
}
