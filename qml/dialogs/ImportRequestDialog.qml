import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import TestRequests 1.0

Dialog {
    id: root
    modal: true
    title: ""
    standardButtons: Dialog.NoButton

    signal importFile(url fileUrl)

    property url selectedFileUrl: ""
    property string selectedFileName: ""

    width: 520
    implicitHeight: layout.implicitHeight + 112

    FileDialog {
        id: fileDialog
        title: "Select Excel file"
        nameFilters: ["Excel files (*.xlsx)"]
        onAccepted: {
            root.selectedFileUrl = selectedFile
            root.selectedFileName = ("" + selectedFile).split("/").pop()
        }
    }

    background: Rectangle {
        radius: Style.radiusLg
        color: Style.panel
        border.width: Style.borderWidth
        border.color: Style.border
    }

    header: Item {
        implicitHeight: 56

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: Style.padLg
            anchors.rightMargin: Style.padLg
            spacing: Style.padSm

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2

                Label {
                    text: "Add test request (Excel)"
                    font.family: Style.fontPrimaryBold
                    font.pixelSize: Style.fontLg
                    color: Style.text
                    Layout.fillWidth: true
                }
            }

            ToolButton {
                text: "close"
                font.family: Style.iconFontFamily
                font.pixelSize: 18
                onClicked: root.close()
            }
        }
    }

    ColumnLayout {
        id: layout
        anchors.fill: parent
        anchors.margins: Style.padLg
        spacing: 12

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 160
            radius: 10
            border.width: 1
            border.color: root.selectedFileUrl !== "" ? Style.accent : Style.border
            color: Style.panel2

            Text {
                anchors.centerIn: parent
                width: parent.width * 0.9
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                font.family: Style.fontSecondary
                font.pixelSize: Style.fontSm
                color: Style.text
                text: root.selectedFileName !== ""
                      ? ("Selected: " + root.selectedFileName)
                      : "Drag & drop an .xlsx file here\nor click Browse…"
            }

            DropArea {
                anchors.fill: parent
                onEntered: parent.border.color = Style.accent
                onExited: parent.border.color = root.selectedFileUrl !== "" ? Style.accent : Style.border
                onDropped: function(drop) {
                    parent.border.color = root.selectedFileUrl !== "" ? Style.accent : Style.border
                    if (!drop.hasUrls || drop.urls.length === 0) return
                    var u = drop.urls[0]
                    var s = "" + u
                    if (!s.toLowerCase().endsWith(".xlsx")) return
                    root.selectedFileUrl = u
                    root.selectedFileName = s.split("/").pop()
                }
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: fileDialog.open()
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Button {
                text: "Browse…"
                onClicked: fileDialog.open()
            }

            Item { Layout.fillWidth: true }

            Button {
                text: "Cancel"
                onClicked: root.close()
            }

            Button {
                text: "Import"
                enabled: root.selectedFileUrl !== "" && !App.dashboard.busy
                onClicked: {
                    root.importFile(root.selectedFileUrl)
                    root.close()
                }
            }
        }

        Text {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            font.family: Style.fontSecondary
            font.pixelSize: Style.fontXs
            color: Style.subText
            text: "The Excel must contain headers like: Client Code, Project N°, Sample ID, Invenesis Assay ID, Starting Dose (µM), Dilution Fold, Nb Replicates, Nb Dilutions."
        }
    }
}
