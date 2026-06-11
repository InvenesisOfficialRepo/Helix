import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import TestRequests 1.0
import Qt5Compat.GraphicalEffects

Rectangle {
    id: root

    // Sizing like before (works well in RowLayout)
    radius: 999
    implicitHeight: 28
    implicitWidth: label.implicitWidth + 38

    border.width: Style.borderWidth
    border.color: Style.border

    // Base tint depends on DB state
    property color okTint:   Qt.rgba(Style.ok.r,   Style.ok.g,   Style.ok.b,   0.14)
    property color warnTint: Qt.rgba(Style.warn.r, Style.warn.g, Style.warn.b, 0.16)

    color: App.dbReady ? okTint : warnTint

    // Smooth transition when state flips
    Behavior on color { ColorAnimation { duration: 220; easing.type: Easing.InOutQuad } }

    // Optional: subtle scale pop on status change
    scale: 1.0
    Behavior on scale { NumberAnimation { duration: 180; easing.type: Easing.OutCubic } }

    onColorChanged: {
        // tiny "pop" when status changes
        root.scale = 1.04
        popBack.restart()
    }

    Timer {
        id: popBack
        interval: 140
        repeat: false
        onTriggered: root.scale = 1.0
    }

    // Content
    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 9
        anchors.rightMargin: 9
        spacing: 8

        // Small status dot
        Rectangle {
            id: dot
            width: 8
            height: 8
            radius: 4
            color: App.dbReady ? Qt.rgba(Style.ok.r, Style.ok.g, Style.ok.b, 0.95)
                               : Qt.rgba(Style.warn.r, Style.warn.g, Style.warn.b, 0.95)

            Behavior on color { ColorAnimation { duration: 220; easing.type: Easing.InOutQuad } }

            // Breathing glow when connected
            layer.enabled: true
            layer.effect: Glow {
                radius: App.dbReady ? 10 : 6
                samples: 21
                color: dot.color
                spread: 0.15
                transparentBorder: true
            }

            SequentialAnimation on opacity {
                running: App.dbReady
                loops: Animation.Infinite
                NumberAnimation { from: 0.75; to: 1.0; duration: 650; easing.type: Easing.InOutQuad }
                NumberAnimation { from: 1.0; to: 0.75; duration: 650; easing.type: Easing.InOutQuad }
            }
        }

        Label {
            id: label
            Layout.alignment: Qt.AlignVCenter
            text: App.dbReady ? "DB connected" : "DB not ready"
            font.family: Style.fontSecondary
            font.pixelSize: Style.fontSm
            color: Style.text
        }
    }

    // --- Shimmer sweep (subtle) ---
    // only really noticeable when connected; still present when not ready but weaker
    Item {
        id: shimmerClip
        anchors.fill: parent
        clip: true

        Rectangle {
            id: shimmer
            width: root.width * 0.55
            height: root.height * 2
            radius: 999

            // a soft white highlight; stronger when connected
            color: Qt.rgba(1, 1, 1, App.dbReady ? 0.10 : 0.06)

            // fake gradient via opacity fade using layer + shaderless trick:
            // just use a blurred glow to soften edges
            layer.enabled: true
            layer.effect: Glow {
                radius: 18
                samples: 25
                color: shimmer.color
                spread: 0.18
                transparentBorder: true
            }

            rotation: 18
            y: -root.height * 0.5
            x: -width

            SequentialAnimation on x {
                running: true
                loops: Animation.Infinite
                NumberAnimation {
                    from: -shimmer.width
                    to: root.width + shimmer.width
                    duration: App.dbReady ? 1700 : 2200
                    easing.type: Easing.InOutQuad
                }
                PauseAnimation { duration: App.dbReady ? 900 : 1400 }
            }
        }
    }
}
