import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtQuick.Window
import TestRequests 1.0

import "../components"
import "../controls"

Page {
    id: root
    background: null

    function pushPage(relativeQmlFile) {
        var w = root.Window.window
        if (w && w.appStack) w.appStack.push(Qt.resolvedUrl(relativeQmlFile))
        else console.error("DashboardPage: window/appStack not available")
    }

    Component.onCompleted: App.dashboard.refresh()

    // =======================
    // Import Excel Dialog
    // =======================
    Dialog {
        id: importDialog
        modal: true
        title: ""
        standardButtons: Dialog.NoButton
        width: 520
        implicitHeight: layout.implicitHeight + 112

        //center in the window
        x: (root.Window.window ? (root.Window.window.contentItem.width - width) / 2 : 0)
        y: (root.Window.window ? (root.Window.window.contentItem.height - height) / 2 : 0)

        Connections {
            target: root.Window.window ? root.Window.window.contentItem : null
            function onWidthChanged()  { importDialog.x = (target.width  - importDialog.width)  / 2 }
            function onHeightChanged() { importDialog.y = (target.height - importDialog.height) / 2 }
        }


        property url selectedFileUrl: ""
        property string selectedFileName: ""

        FileDialog {
            id: fileDialog
            title: "Select Excel file"
            nameFilters: ["Excel files (*.xlsx)"]
            onAccepted: {
                importDialog.selectedFileUrl = selectedFile
                importDialog.selectedFileName = ("" + selectedFile).split("/").pop()
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
                    onClicked: importDialog.close()
                }
            }
        }

        ColumnLayout {
            id: layout
            anchors.fill: parent
            anchors.margins: Style.padLg
            spacing: 12

            Rectangle {
                id: excelDropBox
                Layout.fillWidth: true
                Layout.preferredHeight: 160
                radius: 10
                border.width: 1
                border.color: importDialog.selectedFileUrl !== "" ? Style.accent : (excelMouseArea.containsMouse ? Style.accent2 : Style.border)
                color: Style.panel2

                Text {
                    anchors.centerIn: parent
                    width: parent.width * 0.9
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                    font.family: Style.fontSecondary
                    font.pixelSize: Style.fontSm
                    color: Style.text
                    text: importDialog.selectedFileName !== ""
                          ? ("Selected: " + importDialog.selectedFileName)
                          : "Drag & drop an .xlsx file here\nor click Browse…"
                }

                DropArea {
                    anchors.fill: parent
                    onEntered: excelDropBox.border.color = Style.accent
                    onExited: excelDropBox.border.color = importDialog.selectedFileUrl !== "" ? Style.accent : (excelMouseArea.containsMouse ? Style.accent2 : Style.border)
                    onDropped: function(drop) {
                        excelDropBox.border.color = importDialog.selectedFileUrl !== "" ? Style.accent : (excelMouseArea.containsMouse ? Style.accent2 : Style.border)
                        if (!drop.hasUrls || drop.urls.length === 0) return
                        var u = drop.urls[0]
                        var s = "" + u
                        if (!s.toLowerCase().endsWith(".xlsx")) return
                        importDialog.selectedFileUrl = u
                        importDialog.selectedFileName = s.split("/").pop()
                    }
                }

                MouseArea {
                    id: excelMouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: fileDialog.open()
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                Button {
                    text: "Browse…"
                    enabled: !App.dashboard.busy
                    onClicked: fileDialog.open()
                }

                Item { Layout.fillWidth: true }

                Button {
                    text: "Cancel"
                    onClicked: importDialog.close()
                }

                Button {
                    text: "Import"
                    enabled: importDialog.selectedFileUrl !== "" && !App.dashboard.busy
                    onClicked: {
                        // C++ side should accept QUrl
                        App.dashboard.importRequestExcel(importDialog.selectedFileUrl, App.session.username)
                        importDialog.close()
                        importDialog.selectedFileUrl = ""
                        importDialog.selectedFileName = ""
                    }
                }
            }

            Text {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                font.family: Style.fontSecondary
                font.pixelSize: Style.fontXs
                color: Style.subText
                text: "Expected headers: Client Code, Project N°, Sample ID, Invenesis Assay ID, Starting Dose (µM), Dilution Fold, Nb Replicates, Nb Dilutions."
            }
        }
    }

    // =======================
    // Page Layout
    // =======================
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Style.pad
        spacing: Style.pad
    
        DashboardHeader { busy: App.dashboard.busy }

        DashboardTopBar {
            username: App.session.username
            role: App.session.role
            busy: App.dashboard.busy

            onRefreshClicked: App.dashboard.refresh()
            onCalendarClicked: pushPage("CalendarPage.qml")
            onLogoutClicked: App.session.logout()

            onPublishResultsClicked: pushPage("PublishResultsPage.qml")

            // NEW
            onAddClicked: {
                importDialog.selectedFileUrl = ""
                importDialog.selectedFileName = ""
                importDialog.open()
            }
        }

        ErrorBanner {
            message: App.dashboard.errorMessage
            severity: "error"
        }

        DashboardCardsList {
            cardsModel: App.dashboard.testCardsModel
            onOpenBatch: function(batchId) {
                if (!batchId || batchId === "0") return
                App.dashboard.openBatch(batchId)
                pushPage("ProjectDetailPage.qml")
            }
        }
    }
}
