import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import TestRequests 1.0

Rectangle {
    id: root
    Layout.fillWidth: true

    radius: Style.radiusLg
    color: Style.panel
    border.color: Style.border
    border.width: Style.borderWidth

    property string batchId: ""
    property string projectCode: ""
    property string testCode: ""
    property string overallStatus: ""
    property var scheduledFor: null   // Date
    property string createdBy: ""

    implicitHeight: col.implicitHeight + Style.padLg * 2

    ColumnLayout {
        id: col
        anchors.fill: parent
        anchors.margins: Style.padLg
        spacing: 6

        Label {
            text: "Batch: " + root.batchId
            color: Style.subText
            font.family: Style.fontSecondary
            font.pixelSize: Style.fontSm
            Layout.fillWidth: true
            elide: Text.ElideRight
        }

        Label {
            text: root.projectCode
            color: Style.text
            font.family: Style.fontPrimaryBold
            font.pixelSize: Style.fontLg
            Layout.fillWidth: true
            elide: Text.ElideRight
        }

        Label {
            text: "Created by: " + (root.createdBy || "")
            color: Style.subText
            font.family: Style.fontSecondary
            font.pixelSize: Style.fontSm
            Layout.fillWidth: true
            elide: Text.ElideRight
        }

        Label {
            text: "Test: " + root.testCode
            color: Style.subText
            font.family: Style.fontSecondary
            font.pixelSize: Style.fontSm
            Layout.fillWidth: true
            elide: Text.ElideRight
        }

        Label {
            text: "Status: " + root.overallStatus
            color: Style.subText
            font.family: Style.fontSecondary
            font.pixelSize: Style.fontSm
            Layout.fillWidth: true
            elide: Text.ElideRight
        }

        Label {
            Layout.fillWidth: true
            color: Style.subText
            font.family: Style.fontSecondary
            font.pixelSize: Style.fontSm

            text: {
                var d = root.scheduledFor
                var ok = d && d.isValid && d.isValid()
                return "Scheduled for: " + (ok ? d.toString(Qt.ISODate) : "")
            }
            elide: Text.ElideRight
        }
    }
}
