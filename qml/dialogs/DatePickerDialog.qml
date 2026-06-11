import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import TestRequests 1.0
import "../components"
import "../controls"

Dialog {
    id: root
    modal: true
    focus: true
    title: ""
    standardButtons: Dialog.NoButton

    signal acceptedDate(date d)

    property date pickedDate: new Date()
    property date initialDate: new Date()
    property real _openT: 0.0

    implicitWidth: 640
    implicitHeight: 720

    // -------- Keyboard handling (robust for Popup/Dialog) --------
    // Works even when focus is inside child items (ListView / calendar).
    Shortcut {
        enabled: root.visible && scheduleBtn.enabled
        sequences: ["Return", "Enter"]       // Enter + keypad Enter
        context: Qt.WindowShortcut
        onActivated: root.doAccept()
    }

    Shortcut {
        enabled: root.visible
        sequence: "Escape"
        context: Qt.WindowShortcut
        onActivated: root.close()
    }

    Overlay.modal: Rectangle {
        color: Qt.rgba(0, 0, 0, 0.35 * root._openT)
        Behavior on color { ColorAnimation { duration: 160 } }
    }

    background: Rectangle {
        id: surface
        radius: Style.radiusLg
        color: Style.panel
        border.width: Style.borderWidth
        border.color: Style.border

        opacity: root._openT
        scale: 0.96 + 0.04 * root._openT
        transformOrigin: Item.Center

        Behavior on opacity { NumberAnimation { duration: 160; easing.type: Easing.OutCubic } }
        Behavior on scale   { NumberAnimation { duration: 220; easing.type: Easing.OutBack } }
    }

    onAboutToShow: root._openT = 0.0

    onOpened: {
        pickedDate = initialDate ? initialDate : new Date()

        cal.shownMonth = pickedDate.getMonth()
        cal.shownYear  = pickedDate.getFullYear()
        cal.selectedDate = pickedDate

        App.calendar.setVisibleMonth(cal.shownYear, cal.shownMonth)
        App.calendar.selectedDate = pickedDate
        App.calendar.reloadForSelectedDate()

        root._openT = 1.0
        root.forceActiveFocus()
    }

    function doAccept() {
        // normalize to a pure day (no time surprises)
        const d = new Date(pickedDate.getFullYear(), pickedDate.getMonth(), pickedDate.getDate())
        root.acceptedDate(d)
        root.close()
    }

    // Header (always visible)
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
                    text: "Schedule date"
                    font.family: Style.fontPrimaryBold
                    font.pixelSize: Style.fontLg
                    color: Style.text
                    Layout.fillWidth: true
                }

                Label {
                    text: "Select a date and review existing scheduled tests."
                    font.family: Style.fontSecondary
                    font.pixelSize: Style.fontSm
                    color: Style.subText
                    Layout.fillWidth: true
                }
            }

            BusyIndicator {
                running: App.calendar.busy
                visible: App.calendar.busy
                Layout.preferredWidth: 18
                Layout.preferredHeight: 18
            }

            ToolButton {
                text: "close"
                font.family: Style.iconFontFamily
                font.pixelSize: 18
                onClicked: root.close()
            }
        }
    }

    // Footer actions (always visible)
    footer: Item {
        implicitHeight: 64

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: Style.padLg
            anchors.rightMargin: Style.padLg
            spacing: Style.padSm

            Item { Layout.fillWidth: true }

            Button {
                text: "Cancel"
                onClicked: root.close()
            }

            Button {
                id: scheduleBtn
                text: "Schedule"
                // recommended: prevent scheduling while loading
                enabled: !App.calendar.busy
                onClicked: root.doAccept()
            }
        }
    }

    contentItem: ColumnLayout {
        anchors.fill: parent
        anchors.margins: Style.padLg
        spacing: Style.padSm

        ErrorBanner {
            message: App.calendar.errorMessage
            severity: "error"
        }

        PlanningCalendar {
            id: cal
            Layout.fillWidth: true
            dayCounts: App.calendar.monthDayCounts
            selectedDate: root.pickedDate

            onMonthChanged: function(y, m0) {
                App.calendar.setVisibleMonth(y, m0)
            }

            onDateClicked: function(d) {
                root.pickedDate = d
                cal.selectedDate = d

                App.calendar.selectedDate = d
                App.calendar.reloadForSelectedDate()
            }
        }

        // Selected date line
        RowLayout {
            Layout.fillWidth: true
            spacing: Style.padSm

            Label {
                Layout.fillWidth: true
                text: "Already scheduled on " + Qt.formatDate(root.pickedDate, "dddd, dd MMM yyyy")
                color: Style.subText
                font.family: Style.fontSecondary
                font.pixelSize: Style.fontSm
                elide: Text.ElideRight
            }

            Label {
                text: (scheduledList.count > 0) ? (scheduledList.count + " item(s)") : "No items"
                color: Style.subText
                font.family: Style.fontSecondary
                font.pixelSize: Style.fontSm
            }
        }

        // Events list
        ListView {
            id: scheduledList
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 160
            clip: true
            spacing: Style.padSm

            header: Item { height: 6 }
            footer: Item { height: 8 }

            model: App.calendar.eventsModel

            delegate: CalendarEventRow {
                width: scheduledList.width - 12
                title: model.title
                subtitle: ""
                batchId: model.batchId
                onClicked: {}
            }
        }

        Label {
            Layout.fillWidth: true
            visible: scheduledList.count === 0 && !App.calendar.busy
            text: "No scheduled items for this day."
            color: Style.subText
            font.family: Style.fontSecondary
            font.pixelSize: Style.fontSm
            horizontalAlignment: Text.AlignHCenter
        }
    }
}
