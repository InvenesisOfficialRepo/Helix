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

    function popPage() {
        var w = root.Window.window
        if (w && w.appStack) w.appStack.pop()
        else console.error("CalendarPage: window/appStack not available")
    }

    function pushProjectDetail(batchId) {
        App.project.loadBatch(batchId)
        var w = root.Window.window
        if (w && w.appStack) w.appStack.push(Qt.resolvedUrl("ProjectDetailPage.qml"))
    }

    Component.onCompleted: {
        App.calendar.setVisibleMonth(cal.shownYear, cal.shownMonth)
        App.calendar.reloadForSelectedDate()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Style.pad
        spacing: Style.pad

        CalendarHeader {
            busy: App.calendar.busy
            onBackClicked: popPage()
        }

        ErrorBanner {
            message: App.calendar.errorMessage
            severity: "error"
        }

        // Calendar card
        PlanningCalendar {
            id: cal
            Layout.fillWidth: true
            dayCounts: App.calendar.monthDayCounts
            selectedDate: App.calendar.selectedDate

            onMonthChanged: function(y, m0) {
                App.calendar.setVisibleMonth(y, m0)
            }

            onDateClicked: function(d) {
                App.calendar.selectedDate = d
                App.calendar.reloadForSelectedDate()
            }
        }

        // Selected date header
        RowLayout {
            Layout.fillWidth: true
            spacing: Style.padSm

            Label {
                text: Qt.formatDate(App.calendar.selectedDate, "dddd, dd MMM yyyy")
                color: Style.text
                font.family: Style.fontSecondaryBold
                font.pixelSize: Style.fontMd
                Layout.fillWidth: true
                elide: Text.ElideRight
            }

            Label {
                text: (eventsView.count > 0) ? (eventsView.count + " item(s)") : "No items"
                color: Style.subText
                font.family: Style.fontSecondary
                font.pixelSize: Style.fontSm
            }
        }

        // Events list
        ListView {
            id: eventsView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: Style.padSm

            // nice breathing room for shadows
            header: Item { height: 6 }
            footer: Item { height: 8 }

            model: App.calendar.eventsModel

            delegate: CalendarEventRow {
                width: eventsView.width - 12
                title: model.title
                // if you have extra roles, plug them here later
                subtitle: ""
                batchId: model.batchId

                onClicked: (batchId) => pushProjectDetail(batchId)
            }
        }

        Label {
            Layout.fillWidth: true
            visible: eventsView.count === 0 && !App.calendar.busy
            text: "No scheduled items for this day."
            color: Style.subText
            font.family: Style.fontSecondary
            font.pixelSize: Style.fontSm
            horizontalAlignment: Text.AlignHCenter
        }
    }
}
