import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import TestRequests 1.0

Item {
    id: root

    // API
    property string message: App.dbError
    property string severity: "error" // "warn" or "error"

    // Layout integration
    Layout.fillWidth: true

    // Internal state
    readonly property bool hasMessage: message && message.length > 0

    // We animate height/opacity, so keep the wrapper always in layout,
    // but collapse it to 0 when no message.
    implicitHeight: banner.implicitHeight
    height: hasMessage ? banner.implicitHeight : 0

    // Animate collapse/expand
    Behavior on height { NumberAnimation { duration: 180; easing.type: Easing.InOutQuad } }

    Rectangle {
        id: banner
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top

        radius: Style.radiusSm
        border.width: Style.borderWidth

        readonly property color baseColor: {
            if (root.severity === "warn") return Style.warn
            return (Style.dbError !== undefined) ? Style.dbError : Style.bad
        }

        color: Qt.rgba(baseColor.r, baseColor.g, baseColor.b, 0.14)
        border.color: Qt.rgba(baseColor.r, baseColor.g, baseColor.b, 0.55)

        // Content-driven height (same as your original)
        implicitHeight: msgText.implicitHeight + Style.padSm * 2

        // Fade in/out
        opacity: root.hasMessage ? 1.0 : 0.0
        Behavior on opacity { NumberAnimation { duration: 140; easing.type: Easing.InOutQuad } }

        // A tiny upward motion feels nice
        y: root.hasMessage ? 0 : -4
        Behavior on y { NumberAnimation { duration: 180; easing.type: Easing.OutCubic } }

        // Prevent clicks when hidden (optional)
        enabled: root.hasMessage

        Text {
            id: msgText
            anchors.fill: parent
            anchors.margins: Style.padSm
            text: root.message
            wrapMode: Text.Wrap
            color: Style.text
            font.family: Style.fontSecondary
            font.pixelSize: Style.fontSm
        }
    }
}
