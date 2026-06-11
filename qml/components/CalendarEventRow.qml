import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects
import TestRequests 1.0
import "../components"

Item {
    id: root
    width: 360
    implicitHeight: 56

    property string title: ""
    property string subtitle: ""
    property string batchId: ""
    signal clicked(string batchId)

    property bool hovered: false

    Item {
        anchors.fill: parent
        y: root.hovered ? -1 : 0
        Behavior on y { NumberAnimation { duration: 140; easing.type: Easing.OutCubic } }

        Rectangle {
            id: card
            anchors.fill: parent
            radius: Style.radiusLg
            color: Style.panel
            border.width: Style.borderWidth
            border.color: root.hovered ? Qt.rgba(Style.focus.r, Style.focus.g, Style.focus.b, 0.30) : Style.border
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

                Rectangle {
                    width: 8
                    Layout.fillHeight: true
                    radius: 4
                    color: Qt.rgba(Style.accent.r, Style.accent.g, Style.accent.b, 0.75)
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2

                    Label {
                        text: root.title
                        color: Style.text
                        font.family: Style.fontSecondaryBold
                        font.pixelSize: Style.fontMd
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                    }

                    Label {
                        visible: root.subtitle && root.subtitle.length > 0
                        text: root.subtitle
                        color: Style.subText
                        font.family: Style.fontSecondary
                        font.pixelSize: Style.fontSm
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                    }
                }

                Text {
                    text: "chevron_right"
                    font.family: Style.iconFontFamily
                    font.pixelSize: 18
                    color: Style.subText
                    opacity: 0.7
                }
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onEntered: root.hovered = true
        onExited: root.hovered = false
        onClicked: root.clicked(root.batchId)
    }
}
