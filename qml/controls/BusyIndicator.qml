import QtQuick
import QtQuick.Templates as T
import QtQuick.Controls
import TestRequests 1.0

T.BusyIndicator {
    id: root
    implicitWidth: 18
    implicitHeight: 18
    running: visible

    contentItem: Item {
        id: ring
        anchors.fill: parent
        opacity: root.running ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: 150 } }

        RotationAnimator {
            target: ring
            running: root.running
            from: 0
            to: 360
            loops: Animation.Infinite
            duration: 900
        }

        Repeater {
            model: 10
            Rectangle {
                width: Math.max(2, root.width * 0.12)
                height: width
                radius: width / 2
                color: Style.accent
                opacity: (index + 1) / 10

                x: ring.width / 2 - width / 2
                y: ring.height / 2 - height / 2

                transform: [
                    Translate { y: -((Math.min(ring.width, ring.height) * 0.45)) },
                    Rotation {
                        angle: (index / 10) * 360
                        origin.x: width / 2
                        origin.y: height / 2
                    }
                ]
            }
        }
    }
}
