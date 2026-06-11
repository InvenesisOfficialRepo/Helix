import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
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
        else console.error("PublishResultsPage: window/appStack not available")
    }

    function popPage() {
        var w = root.Window.window
        if (w && w.appStack) w.appStack.pop()
        else console.error("PublishResultsPage: window/appStack not available")
    }

    Component.onCompleted: App.dashboard.refreshDone()

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Style.pad
        spacing: Style.pad

        RowLayout {
            Layout.fillWidth: true
            spacing: Style.pad

            Button {
                text: "Back"
                onClicked: popPage()
            }

            Label {
                text: "Analyze & Publish - Completed Batches"
                font.family: Style.fontPrimaryBold
                font.pixelSize: Style.fontLg
                color: Style.text
                Layout.fillWidth: true
            }

            Button {
                text: "Refresh"
                enabled: !App.dashboard.busy
                onClicked: App.dashboard.refreshDone()
            }
        }

        ErrorBanner {
            message: App.dashboard.errorMessage
            severity: "error"
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            ColumnLayout {
                width: parent.width
                spacing: Style.pad

                // Section 1: Batches Pending Analysis
                Label {
                    text: "Batches Pending Analysis"
                    font.family: Style.fontPrimaryBold
                    font.pixelSize: Style.fontLg
                    color: Style.text
                    visible: pendingRepeater.count > 0
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: Style.pad
                    visible: pendingRepeater.count > 0

                    Repeater {
                        id: pendingRepeater
                        model: App.dashboard.doneTestCardsModel
                        delegate: TestCard {
                            Layout.fillWidth: true
                            testCode: model.testCode
                            pendingCount: model.pendingCount
                            sentCount: model.sentCount
                            projectsModel: model.projectsModel
                            onOpenBatch: function(batchId) {
                                if (!batchId || batchId === "0") return
                                App.dashboard.openBatch(batchId)
                                pushPage("PublishDetailPage.qml")
                            }
                        }
                    }
                }

                // Divider (visible if both sections have items)
                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: Style.border
                    visible: pendingRepeater.count > 0 && archiveRepeater.count > 0
                }

                // Section 2: Archive (Published Batches)
                Label {
                    text: "Archive (Published Batches)"
                    font.family: Style.fontPrimaryBold
                    font.pixelSize: Style.fontLg
                    color: Style.text
                    visible: archiveRepeater.count > 0
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: Style.pad
                    visible: archiveRepeater.count > 0

                    Repeater {
                        id: archiveRepeater
                        model: App.dashboard.archiveTestCardsModel
                        delegate: TestCard {
                            Layout.fillWidth: true
                            testCode: model.testCode
                            pendingCount: model.pendingCount
                            sentCount: model.sentCount
                            projectsModel: model.projectsModel
                            onOpenBatch: function(batchId) {
                                if (!batchId || batchId === "0") return
                                App.dashboard.openBatch(batchId)
                                pushPage("PublishDetailPage.qml")
                            }
                        }
                    }
                }

                // Placeholder when both are empty
                Label {
                    text: "No completed or archived batches found for the current year."
                    font.family: Style.fontSecondary
                    font.pixelSize: Style.fontMd
                    color: Style.subText
                    visible: pendingRepeater.count === 0 && archiveRepeater.count === 0
                    horizontalAlignment: Text.AlignHCenter
                    Layout.fillWidth: true
                    Layout.margins: 40
                }
            }
        }
    }
}
