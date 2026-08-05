import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects
import TestRequests 1.0
import "../components"
import "../controls"
import "../dialogs"

Page {
    id: root

    property var viewModel: App.tecanPlanner

    Component.onCompleted: {
        if (viewModel) {
            viewModel.refreshTodaySummary()
            viewModel.reload()
        }
    }

    background: Rectangle {
        color: Style.bg

        // Subtle gradient background overlay for workstation feel
        Rectangle {
            anchors.fill: parent
            opacity: 0.10
            gradient: Gradient {
                GradientStop { position: 0.0; color: Style.teal }
                GradientStop { position: 1.0; color: "transparent" }
            }
        }
    }

    // =========================================================================
    // Header & ToolBar
    // =========================================================================
    header: ToolBar {
        ColumnLayout {
            anchors.fill: parent
            anchors.leftMargin: Style.padLg
            anchors.rightMargin: Style.padLg
            anchors.topMargin: Style.padSm
            anchors.bottomMargin: Style.padSm
            spacing: Style.padSm

            RowLayout {
                Layout.fillWidth: true
                spacing: Style.pad

                // Back Button (if navigated from stack)
                Rectangle {
                    width: 34
                    height: 32
                    radius: Style.radiusSm
                    color: backMouse.containsMouse ? Style.surfaceRaised : Style.surfacePanel
                    border.color: backMouse.containsMouse ? Style.accent : Style.border
                    border.width: 1
                    visible: (root.Window.window && root.Window.window.appStack && root.Window.window.appStack.depth > 1) ? true : false

                    Label {
                        anchors.centerIn: parent
                        text: "arrow_back"
                        font.family: Style.iconFontFamily
                        font.pixelSize: Style.fontLg
                        color: Style.text
                    }

                    MouseArea {
                        id: backMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            if (root.Window.window && root.Window.window.appStack) {
                                root.Window.window.appStack.pop()
                            }
                        }
                    }
                }

                // Module Title & Icon
                Rectangle {
                    width: 38
                    height: 38
                    radius: Style.radiusMd
                    color: Qt.rgba(Style.accent.r, Style.accent.g, Style.accent.b, 0.15)
                    border.color: Qt.rgba(Style.accent.r, Style.accent.g, Style.accent.b, 0.3)
                    border.width: 1

                    Label {
                        anchors.centerIn: parent
                        text: "biotech"
                        font.family: Style.iconFontFamily
                        font.pixelSize: Style.fontXl
                        color: Style.accent
                    }
                }

                ColumnLayout {
                    spacing: 2

                    Label {
                        text: "Daily Tecan Planner"
                        font.family: Style.fontPrimaryBold
                        font.pixelSize: Style.fontLg
                        color: Style.text
                    }

                    Label {
                        text: "Robotic assay batch scheduler • Single assay, single solvent, single species runs"
                        font.family: Style.fontSecondary
                        font.pixelSize: Style.fontXs
                        color: Style.subText
                    }
                }

                Item { Layout.fillWidth: true }

                // Refresh Button
                Rectangle {
                    implicitWidth: refreshRow.implicitWidth + 24
                    implicitHeight: 34
                    radius: Style.radiusSm
                    color: refreshMouse.containsMouse ? Style.surfaceRaised : Style.surfacePanel
                    border.color: refreshMouse.containsMouse ? Style.accent : Style.border
                    border.width: 1

                    RowLayout {
                        id: refreshRow
                        anchors.centerIn: parent
                        spacing: Style.padSm

                        Label {
                            text: "refresh"
                            font.family: Style.iconFontFamily
                            font.pixelSize: Style.fontMd
                            color: Style.text
                        }

                        Label {
                            text: "Refresh"
                            font.family: Style.fontSecondaryBold
                            font.pixelSize: Style.fontSm
                            color: Style.text
                        }
                    }

                    MouseArea {
                        id: refreshMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            if (viewModel) {
                                viewModel.refreshTodaySummary()
                                viewModel.reload()
                            }
                        }
                    }
                }
            }
        }
    }

    // =========================================================================
    // Content Layout
    // =========================================================================
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Style.padLg
        spacing: Style.pad

        // ---------------------------------------------------------------------
        // Date Navigator & KPI Ribbon
        // ---------------------------------------------------------------------
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: navigatorRow.implicitHeight + Style.pad * 2
            color: Style.surfacePanel
            border.color: Style.border
            border.width: Style.borderWidth
            radius: Style.radiusLg

            RowLayout {
                id: navigatorRow
                anchors.fill: parent
                anchors.margins: Style.pad
                spacing: Style.padLg

                // Date Navigation Controls
                RowLayout {
                    spacing: Style.padSm

                    // Previous Day Button
                    Rectangle {
                        width: 32
                        height: 32
                        radius: Style.radiusSm
                        color: prevMouse.containsMouse ? Style.surfaceRaised : Style.surfacePanel
                        border.color: prevMouse.containsMouse ? Style.accent : Style.border
                        border.width: 1

                        Label {
                            anchors.centerIn: parent
                            text: "chevron_left"
                            font.family: Style.iconFontFamily
                            font.pixelSize: Style.fontLg
                            color: Style.text
                        }

                        MouseArea {
                            id: prevMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: if (viewModel) viewModel.prevDay()
                        }
                    }

                    // Today Quick-Select Button
                    Rectangle {
                        implicitWidth: todayLabel.implicitWidth + 24
                        height: 32
                        radius: Style.radiusSm
                        color: todayMouse.containsMouse ? Qt.rgba(Style.accent.r, Style.accent.g, Style.accent.b, 0.2) : Qt.rgba(Style.accent.r, Style.accent.g, Style.accent.b, 0.1)
                        border.color: todayMouse.containsMouse ? Style.accent : Qt.rgba(Style.accent.r, Style.accent.g, Style.accent.b, 0.3)
                        border.width: 1

                        Label {
                            id: todayLabel
                            anchors.centerIn: parent
                            text: "Today"
                            font.family: Style.fontSecondaryBold
                            font.pixelSize: Style.fontSm
                            color: Style.accent
                        }

                        MouseArea {
                            id: todayMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: if (viewModel) viewModel.setToday()
                        }
                    }

                    // Next Day Button
                    Rectangle {
                        width: 32
                        height: 32
                        radius: Style.radiusSm
                        color: nextMouse.containsMouse ? Style.surfaceRaised : Style.surfacePanel
                        border.color: nextMouse.containsMouse ? Style.accent : Style.border
                        border.width: 1

                        Label {
                            anchors.centerIn: parent
                            text: "chevron_right"
                            font.family: Style.iconFontFamily
                            font.pixelSize: Style.fontLg
                            color: Style.text
                        }

                        MouseArea {
                            id: nextMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: if (viewModel) viewModel.nextDay()
                        }
                    }

                    // Selected Date Display & Picker trigger
                    Rectangle {
                        implicitHeight: 32
                        implicitWidth: dateLabelRow.implicitWidth + Style.pad * 2
                        color: datePickMouse.containsMouse ? Qt.rgba(Style.accent2.r, Style.accent2.g, Style.accent2.b, 0.2) : Qt.rgba(Style.accent2.r, Style.accent2.g, Style.accent2.b, 0.1)
                        border.color: datePickMouse.containsMouse ? Style.accent : Style.border
                        border.width: 1
                        radius: Style.radiusSm

                        RowLayout {
                            id: dateLabelRow
                            anchors.centerIn: parent
                            spacing: Style.padSm

                            Label {
                                text: "calendar_today"
                                font.family: Style.iconFontFamily
                                font.pixelSize: Style.fontMd
                                color: Style.accent
                            }

                            Label {
                                text: {
                                    if (!viewModel || !viewModel.selectedDate) return "Select Date"
                                    return Qt.formatDate(viewModel.selectedDate, "dddd, MMMM d, yyyy")
                                }
                                font.family: Style.fontSecondaryBold
                                font.pixelSize: Style.fontSm
                                color: Style.text
                            }
                        }

                        MouseArea {
                            id: datePickMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                if (viewModel && viewModel.selectedDate) {
                                    datePicker.initialDate = viewModel.selectedDate
                                }
                                datePicker.open()
                            }
                        }
                    }
                }

                Item { Layout.fillWidth: true }

                // KPIs Ribbon
                RowLayout {
                    spacing: Style.padLg

                    // Today Summary Pill
                    Rectangle {
                        implicitHeight: 34
                        implicitWidth: todaySummaryLayout.implicitWidth + 24
                        color: Qt.rgba(Style.ok.r, Style.ok.g, Style.ok.b, 0.12)
                        border.color: Qt.rgba(Style.ok.r, Style.ok.g, Style.ok.b, 0.3)
                        border.width: 1
                        radius: Style.radiusSm

                        RowLayout {
                            id: todaySummaryLayout
                            anchors.centerIn: parent
                            spacing: Style.padSm

                            Label {
                                text: "today"
                                font.family: Style.iconFontFamily
                                font.pixelSize: Style.fontMd
                                color: Style.ok
                            }

                            Label {
                                text: "Today's Queue:"
                                font.family: Style.fontPrimary
                                font.pixelSize: Style.fontXs
                                color: Style.subText
                            }

                            Label {
                                text: (viewModel ? viewModel.todayBatchCount : 0) + " batches (" + (viewModel ? viewModel.todayCompoundCount : 0) + " cpds)"
                                font.family: Style.fontSecondaryBold
                                font.pixelSize: Style.fontSm
                                color: Style.ok
                            }
                        }
                    }

                    // Selected Date Summary Pill
                    Rectangle {
                        implicitHeight: 34
                        implicitWidth: selectedSummaryLayout.implicitWidth + 24
                        color: Qt.rgba(Style.accent.r, Style.accent.g, Style.accent.b, 0.12)
                        border.color: Qt.rgba(Style.accent.r, Style.accent.g, Style.accent.b, 0.3)
                        border.width: 1
                        radius: Style.radiusSm

                        RowLayout {
                            id: selectedSummaryLayout
                            anchors.centerIn: parent
                            spacing: Style.padSm

                            Label {
                                text: "schedule"
                                font.family: Style.iconFontFamily
                                font.pixelSize: Style.fontMd
                                color: Style.accent
                            }

                            Label {
                                text: "Selected Date:"
                                font.family: Style.fontPrimary
                                font.pixelSize: Style.fontXs
                                color: Style.subText
                            }

                            Label {
                                text: (viewModel ? viewModel.batchCount : 0) + " batches (" + (viewModel ? viewModel.totalCompounds : 0) + " cpds)"
                                font.family: Style.fontSecondaryBold
                                font.pixelSize: Style.fontSm
                                color: Style.accent
                            }
                        }
                    }
                }
            }
        }

        // ---------------------------------------------------------------------
        // Error Banner (if any)
        // ---------------------------------------------------------------------
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 40
            color: Qt.rgba(Style.bad.r, Style.bad.g, Style.bad.b, 0.15)
            border.color: Style.bad
            border.width: 1
            radius: Style.radiusMd
            visible: viewModel && viewModel.errorMessage !== ""

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Style.pad
                anchors.rightMargin: Style.pad
                spacing: Style.padSm

                Label {
                    text: "warning"
                    font.family: Style.iconFontFamily
                    font.pixelSize: Style.fontLg
                    color: Style.bad
                }

                Label {
                    text: viewModel ? viewModel.errorMessage : ""
                    font.family: Style.fontSecondary
                    font.pixelSize: Style.fontSm
                    color: Style.text
                    Layout.fillWidth: true
                }

                Button {
                    text: "Retry"
                    implicitHeight: 28
                    implicitWidth: 80
                    onClicked: if (viewModel) viewModel.reload()
                }
            }
        }

        // ---------------------------------------------------------------------
        // Main Batch List / Empty State
        // ---------------------------------------------------------------------
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            // Loading Indicator
            BusyIndicator {
                anchors.centerIn: parent
                running: viewModel ? viewModel.busy : false
                visible: (viewModel ? viewModel.busy : false) && (viewModel ? viewModel.batchCount === 0 : true)
            }

            // Empty State (No Batches for selected date)
            Rectangle {
                anchors.centerIn: parent
                width: Math.min(540, parent.width - 40)
                height: 240
                color: Style.surfacePanel
                border.color: Style.border
                border.width: Style.borderWidth
                radius: Style.radiusLg
                visible: (!viewModel || !viewModel.busy) && (viewModel ? viewModel.batchCount === 0 : true)

                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: Style.pad
                    width: parent.width - Style.padXl * 2

                    Rectangle {
                        Layout.alignment: Qt.AlignHCenter
                        width: 48
                        height: 48
                        radius: 24
                        color: Qt.rgba(Style.accent2.r, Style.accent2.g, Style.accent2.b, 0.15)
                        border.color: Style.border
                        border.width: 1

                        Label {
                            anchors.centerIn: parent
                            text: "event_busy"
                            font.family: Style.iconFontFamily
                            font.pixelSize: Style.fontXxl
                            color: Style.subText
                        }
                    }

                    ColumnLayout {
                        Layout.alignment: Qt.AlignHCenter
                        spacing: 2

                        Label {
                            text: "No Test Requests Scheduled"
                            font.family: Style.fontPrimaryBold
                            font.pixelSize: Style.fontLg
                            color: Style.text
                            Layout.alignment: Qt.AlignHCenter
                        }

                        Label {
                            text: "There are no pending test requests scheduled for this date."
                            font.family: Style.fontSecondary
                            font.pixelSize: Style.fontSm
                            color: Style.subText
                            Layout.alignment: Qt.AlignHCenter
                        }
                    }

                    RowLayout {
                        Layout.alignment: Qt.AlignHCenter
                        spacing: Style.padSm

                        Button {
                            text: "Go to Today"
                            implicitHeight: 34
                            implicitWidth: 120
                            onClicked: if (viewModel) viewModel.setToday()
                        }

                        Button {
                            text: "Pick Date"
                            implicitHeight: 34
                            implicitWidth: 120
                            onClicked: {
                                if (viewModel && viewModel.selectedDate) {
                                    datePicker.initialDate = viewModel.selectedDate
                                }
                                datePicker.open()
                            }
                        }
                    }
                }
            }

            // Batch Cards Scroll View
            ScrollView {
                id: batchScrollView
                anchors.fill: parent
                clip: true
                visible: viewModel && viewModel.batchCount > 0

                ListView {
                    id: batchListView
                    width: batchScrollView.availableWidth
                    model: viewModel ? viewModel.batches : []
                    spacing: Style.pad

                    delegate: Rectangle {
                        id: batchCard
                        width: batchListView.width
                        implicitHeight: batchCardLayout.implicitHeight + Style.padLg * 2
                        color: Style.surfacePanel
                        border.color: mouseHover.containsMouse ? Style.accent : Style.border
                        border.width: mouseHover.containsMouse ? 2 : Style.borderWidth
                        radius: Style.radiusLg

                        Behavior on border.color { ColorAnimation { duration: 120 } }

                        MouseArea {
                            id: mouseHover
                            anchors.fill: parent
                            hoverEnabled: true
                            acceptedButtons: Qt.NoButton
                        }

                        ColumnLayout {
                            id: batchCardLayout
                            anchors.fill: parent
                            anchors.margins: Style.padLg
                            spacing: Style.padSm

                            // Top Header Row: Primary Identification & Action Button
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: Style.padSm

                                // Assay Code Badge (Prominent)
                                Rectangle {
                                    implicitHeight: 32
                                    implicitWidth: assayCodeInnerRow.implicitWidth + 20
                                    color: Qt.rgba(Style.accent.r, Style.accent.g, Style.accent.b, 0.15)
                                    border.color: Style.accent
                                    border.width: 1
                                    radius: Style.radiusSm

                                    RowLayout {
                                        id: assayCodeInnerRow
                                        anchors.centerIn: parent
                                        spacing: Style.padXs

                                        Label {
                                            text: "science"
                                            font.family: Style.iconFontFamily
                                            font.pixelSize: Style.fontSm
                                            color: Style.accent
                                        }

                                        Label {
                                            text: modelData.testCode || "Unknown Assay"
                                            font.family: Style.fontSecondaryBold
                                            font.pixelSize: Style.fontMd
                                            color: Style.accent
                                        }
                                    }
                                }

                                // Compound Count Pill
                                Rectangle {
                                    implicitHeight: 32
                                    implicitWidth: countInnerLabel.implicitWidth + 20
                                    color: Qt.rgba(Style.ok.r, Style.ok.g, Style.ok.b, 0.12)
                                    border.color: Qt.rgba(Style.ok.r, Style.ok.g, Style.ok.b, 0.4)
                                    border.width: 1
                                    radius: Style.radiusSm

                                    Label {
                                        id: countInnerLabel
                                        anchors.centerIn: parent
                                        text: modelData.compoundCount + " Compounds"
                                        font.family: Style.fontSecondaryBold
                                        font.pixelSize: Style.fontSm
                                        color: Style.ok
                                    }
                                }

                                Item { Layout.fillWidth: true }

                                // Primary Action: Launch in Tecan
                                Button {
                                    text: "Prepare in Tecan"
                                    implicitHeight: 34
                                    implicitWidth: 150
                                    onClicked: {
                                        if (modelData.requestIds && modelData.requestIds.length > 0) {
                                            App.launchTecanWithRequests(modelData.requestIds)
                                        }
                                    }
                                }
                            }

                            // Metadata Tags Row: Solvent, Species, Project
                            Flow {
                                Layout.fillWidth: true
                                spacing: Style.padSm

                                // Solvent Badge
                                Rectangle {
                                    implicitHeight: 28
                                    implicitWidth: solventInnerRow.implicitWidth + 16
                                    color: Qt.rgba(Style.s2.r, Style.s2.g, Style.s2.b, 0.12)
                                    border.color: Qt.rgba(Style.s2.r, Style.s2.g, Style.s2.b, 0.4)
                                    border.width: 1
                                    radius: Style.radiusSm

                                    RowLayout {
                                        id: solventInnerRow
                                        anchors.centerIn: parent
                                        spacing: Style.padXs

                                        Label {
                                            text: "opacity"
                                            font.family: Style.iconFontFamily
                                            font.pixelSize: Style.fontSm
                                            color: Style.s2
                                        }

                                        Label {
                                            text: "Solvent: " + (modelData.solvent || "N/A")
                                            font.family: Style.fontSecondaryBold
                                            font.pixelSize: Style.fontXs
                                            color: Style.isDark ? Style.s2 : Qt.darker(Style.s2, 1.2)
                                        }
                                    }
                                }

                                // Species Badge
                                Rectangle {
                                    implicitHeight: 28
                                    implicitWidth: speciesInnerRow.implicitWidth + 16
                                    color: Qt.rgba(Style.s3.r, Style.s3.g, Style.s3.b, 0.12)
                                    border.color: Qt.rgba(Style.s3.r, Style.s3.g, Style.s3.b, 0.4)
                                    border.width: 1
                                    radius: Style.radiusSm

                                    RowLayout {
                                        id: speciesInnerRow
                                        anchors.centerIn: parent
                                        spacing: Style.padXs

                                        Label {
                                            text: "bug_report"
                                            font.family: Style.iconFontFamily
                                            font.pixelSize: Style.fontSm
                                            color: Style.s3
                                        }

                                        Label {
                                            text: "Species: " + (modelData.species || "N/A")
                                            font.family: Style.fontSecondaryBold
                                            font.pixelSize: Style.fontXs
                                            color: Style.isDark ? Style.s3 : Qt.darker(Style.s3, 1.2)
                                        }
                                    }
                                }

                                // Project Badge (if present)
                                Rectangle {
                                    implicitHeight: 28
                                    implicitWidth: prjInnerRow.implicitWidth + 16
                                    color: Qt.rgba(Style.accent2.r, Style.accent2.g, Style.accent2.b, 0.12)
                                    border.color: Style.border
                                    border.width: 1
                                    radius: Style.radiusSm
                                    visible: Boolean(modelData.projectCode && modelData.projectCode !== "")

                                    RowLayout {
                                        id: prjInnerRow
                                        anchors.centerIn: parent
                                        spacing: Style.padXs

                                        Label {
                                            text: "folder"
                                            font.family: Style.iconFontFamily
                                            font.pixelSize: Style.fontSm
                                            color: Style.subText
                                        }

                                        Label {
                                            text: "Project: " + (modelData.projectCode || "")
                                            font.family: Style.fontSecondary
                                            font.pixelSize: Style.fontXs
                                            color: Style.text
                                        }
                                    }
                                }
                            }

                            // Divider
                            Rectangle {
                                Layout.fillWidth: true
                                height: 1
                                color: Style.border
                            }

                            // Compound Chips Area
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: Style.padXs

                                Label {
                                    text: "Compounds in this batch queue (" + (modelData.compoundNames ? modelData.compoundNames.length : 0) + "):"
                                    font.family: Style.fontPrimary
                                    font.pixelSize: Style.fontXs
                                    color: Style.subText
                                }

                                Flow {
                                    Layout.fillWidth: true
                                    spacing: Style.padXs

                                    Repeater {
                                        model: modelData.compoundNames || []

                                        Rectangle {
                                            implicitHeight: 24
                                            implicitWidth: cpdNameText.implicitWidth + 16
                                            color: Style.surfaceRaised
                                            border.color: Style.border
                                            border.width: 1
                                            radius: 4

                                            Label {
                                                id: cpdNameText
                                                anchors.centerIn: parent
                                                text: modelData
                                                font.family: Style.fontSecondary
                                                font.pixelSize: Style.fontXs
                                                color: Style.text
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
    }

    // =========================================================================
    // Embedded Date Picker Dialog
    // =========================================================================
    DatePickerDialog {
        id: datePicker
        x: (root.width - width) / 2
        y: (root.height - height) / 2

        onAcceptedDate: function(d) {
            if (viewModel) {
                viewModel.setSelectedDate(d)
            }
        }
    }
}
