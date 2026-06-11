import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"
import TestRequests 1.0

Frame {
    id: root

    property string testCode: ""
    property int pendingCount: 0
    property int sentCount: 0
    property var projectsModel: null // actually “batch rows” model

    signal openBatch(string batchId)

    padding: 16

    background: Rectangle {
        radius: Style.radiusLg
        color: Style.panel
        border.color: Style.border
        border.width: Style.borderWidth
    }

    ColumnLayout {
        width: parent.width
        spacing: 10

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            Label {
                text: root.testCode
                font.pixelSize: Style.fontLg
                font.family: Style.fontPrimaryBold
                color: Style.text
                Layout.fillWidth: true
            }

            Label {
                text: "Pending: " + root.pendingCount
                color: Style.subText
                font.family: Style.fontSecondary
                font.pixelSize: Style.fontSm
            }
            Label {
                text: "Sent: " + root.sentCount
                color: Style.subText
                font.family: Style.fontSecondary
                font.pixelSize: Style.fontSm
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: Style.divider
        }

        // Batch list
        ListView {
            Layout.fillWidth: true
            implicitHeight: Math.min(contentHeight, 260)
            clip: true
            spacing: 8
            model: root.projectsModel

            delegate: Rectangle {
                id: itemBg
                width: ListView.view.width
                radius: Style.radiusSm
                color: mouseArea.hovered ? Style.hover : Style.panel2
                border.width: 1
                border.color: mouseArea.hovered ? Style.accent : Style.border
                implicitHeight: row.implicitHeight + 12

                HoverHandler {
                    id: mouseArea
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.openBatch(model.batchId)
                }

                RowLayout {
                    id: row
                    anchors.margins: 8
                    anchors.fill: parent
                    spacing: 10

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2

                        Label {
                            text: model.projectCode
                            font.family: Style.fontPrimaryBold
                            font.pixelSize: Style.fontSm
                            color: Style.text
                        }

                        Label {
                            text: {
                                var s = model.subtitle
                                var sched = model.scheduledForText
                                if (sched && sched.length > 0)
                                    return "Scheduled: " + sched + " • " + s
                                return s + (model.createdAtText.length > 0 ? (" • Created: " + model.createdAtText) : "")
                            }
                            color: Style.subText
                            font.family: Style.fontSecondary
                            font.pixelSize: Style.fontXs
                            wrapMode: Text.Wrap
                            Layout.fillWidth: true
                        }
                    }

                    StatusPill { text: model.statusText }
                }
            }

            footer: Item { height: 2 }
        }

        Label {
            visible: root.projectsModel && root.projectsModel.rowCount && root.projectsModel.rowCount() === 0
            text: "No active batches."
            color: Style.subText
            font.family: Style.fontSecondary
            font.pixelSize: Style.fontSm
        }
    }
}
