// qml/pages/WallboardPage.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import TestRequests 1.0
import "../components"

Page {
    id: root

    // Expect a VM that exposes:
    //  - dayModel (TwoWeekDayModel*), roles: day (date), iso (string), totalCount (int), eventsModel (CalendarEventsModel*)
    //  - errorMessage (string)
    //  - lastRefreshIso (string)
    //  - refresh() invokable
    //required property WallboardViewModel vm
    required property var vm

    // Optional: auto-refresh from QML too (VM may already poll internally)
    property int refreshIntervalMs: 60000

    Component.onCompleted: vm.refresh()

    Timer {
        interval: refreshIntervalMs
        running: true
        repeat: true
        onTriggered: vm.refresh()
    }

    header: ToolBar {
        contentHeight: 56
        RowLayout {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 12

            Label {
                text: "Test Planning • This Week + Next Week"
                font.pixelSize: 22
                font.bold: true
                Layout.fillWidth: true
                elide: Label.ElideRight
            }

            Label {
                text: vm.lastRefreshIso && vm.lastRefreshIso.length
                      ? ("Updated: " + vm.lastRefreshIso)
                      : ""
                opacity: 0.7
                font.pixelSize: 14
            }

            Button {
                text: "Refresh"
                onClicked: vm.refresh()
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 10

        Rectangle {
            visible: vm.errorMessage && vm.errorMessage.length > 0
            Layout.fillWidth: true
            radius: 10
            height: implicitHeight
            color: "#3a1f1f"
            border.color: "#b24a4a"

            Text {
                anchors.fill: parent
                anchors.margins: 10
                color: "white"
                wrapMode: Text.Wrap
                text: vm.errorMessage
                font.pixelSize: 14
            }
        }

        // Two columns (Week 1 / Week 2), 7 rows each
        GridLayout {
            id: grid
            Layout.fillWidth: true
            Layout.fillHeight: true
            columns: 2
            columnSpacing: 12
            rowSpacing: 12

            // Column headers
            Rectangle {
                Layout.column: 0
                Layout.row: 0
                Layout.fillWidth: true
                height: 44
                radius: 10
                color: "#202020"
                border.color: "#333333"

                Text {
                    anchors.centerIn: parent
                    color: "white"
                    font.pixelSize: 18
                    font.bold: true
                    text: "This Week"
                }
            }

            Rectangle {
                Layout.column: 1
                Layout.row: 0
                Layout.fillWidth: true
                height: 44
                radius: 10
                color: "#202020"
                border.color: "#333333"

                Text {
                    anchors.centerIn: parent
                    color: "white"
                    font.pixelSize: 18
                    font.bold: true
                    text: "Next Week"
                }
            }

            // Day tiles: 14 items -> week1 col0 rows 1..7, week2 col1 rows 1..7
            Repeater {
                model: vm.dayModel

                delegate: Rectangle {
                    required property date day
                    required property int totalCount
                    required property var eventsModel

                    radius: 12
                    color: "#1b1b1b"
                    border.color: "#333333"

                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    // GridLayout row 0 is headers, so days start at row 1
                    Layout.row: 1 + (index % 7)
                    Layout.column: (index < 7) ? 0 : 1

                    // Give each day roughly equal height (7 rows)
                    Layout.preferredHeight: Math.max(120, (grid.height - 44 - 6 * grid.rowSpacing) / 7)

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 8

                        // Day header
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            Label {
                                Layout.fillWidth: true
                                text: Qt.formatDate(day, "ddd dd MMM")
                                font.pixelSize: 18
                                font.bold: true
                                elide: Label.ElideRight
                            }

                            Rectangle {
                                radius: 999
                                color: totalCount > 0 ? "#2a3d2a" : "#2a2a2a"
                                border.color: "#3a3a3a"
                                height: 26
                                implicitWidth: badgeText.implicitWidth + 18

                                Text {
                                    id: badgeText
                                    anchors.centerIn: parent
                                    color: "white"
                                    font.pixelSize: 13
                                    text: totalCount > 0 ? (totalCount + " batches") : "—"
                                }
                            }
                        }

                        // Event list (read-only wallboard)
                        ListView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true
                            interactive: false
                            spacing: 6
                            model: eventsModel

                            delegate: Rectangle {
                                width: ListView.view.width
                                radius: 10
                                color: "#242424"
                                border.color: "#343434"
                                height: content.implicitHeight + 14

                                RowLayout {
                                    id: content
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    anchors.margins: 8
                                    spacing: 10

                                    // Simple status pill based on counts
                                    Rectangle {
                                        radius: 999
                                        border.color: "#3a3a3a"
                                        height: 24
                                        implicitWidth: pillText.implicitWidth + 18
                                        color: pendingCount > 0 ? "#3b2f1f" : "#24302a"

                                        Text {
                                            id: pillText
                                            anchors.centerIn: parent
                                            color: "white"
                                            font.pixelSize: 12
                                            text: pendingCount > 0 ? ("Pending " + pendingCount) : ("Sent " + sentCount)
                                        }
                                    }

                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        spacing: 2

                                        Label {
                                            Layout.fillWidth: true
                                            text: projectCode + " • " + testCode
                                            font.pixelSize: 14
                                            font.bold: true
                                            elide: Label.ElideRight
                                        }

                                        Label {
                                            Layout.fillWidth: true
                                            text: "Pending " + pendingCount + " / Sent " + sentCount
                                            font.pixelSize: 12
                                            opacity: 0.75
                                            elide: Label.ElideRight
                                        }
                                    }
                                }
                            }

                            footer: (totalCount === 0) ? emptyFooter : null

                            Component {
                                id: emptyFooter
                                Item {
                                    width: ListView.view ? ListView.view.width : 0
                                    height: 28
                                    Text {
                                        anchors.verticalCenter: parent.verticalCenter
                                        color: "#a0a0a0"
                                        font.pixelSize: 13
                                        text: "No scheduled items"
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
