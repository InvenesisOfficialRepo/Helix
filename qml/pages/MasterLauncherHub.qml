import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects
import TestRequests 1.0
import "../components"
import "../controls"

Page {
    id: root
    focus: true

    Component.onCompleted: {
        UpdateManager.checkForUpdates()
    }

    Connections {
        target: UpdateManager

        function onUpdateAvailable(latestVersion, notes) {
            updateOverlay.latestVersion = latestVersion
            updateOverlay.releaseNotes = notes
            updateOverlay.visible = true
        }

        function onErrorOccurred(errorMessage) {
            console.error("Helix Update Error:", errorMessage)
        }
    }

    background: Rectangle {
        color: Style.bg
        
        // Add subtle radial/gradient background overlay for premium clinical style
        Rectangle {
            anchors.fill: parent
            opacity: 0.15
            gradient: Gradient {
                GradientStop { position: 0.0; color: Style.teal }
                GradientStop { position: 1.0; color: "transparent" }
            }
        }
    }

    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            spacing: Style.pad
            Layout.topMargin: Style.padXs

            Label {
                text: "Invenesis Hub"
                font.family: Style.fontPrimaryBold
                font.pixelSize: Style.fontLg
                color: Style.text
                Layout.leftMargin: Style.padLg
            }

            Item { Layout.fillWidth: true }

            RowLayout {
                spacing: Style.pad
                Layout.rightMargin: Style.padLg

                // Connection telemetry indicator
                Rectangle {
                    color: Qt.rgba(Style.accent2.r, Style.accent2.g, Style.accent2.b, 0.15)
                    border.color: Style.border
                    border.width: 1
                    radius: Style.radiusSm
                    implicitHeight: 28
                    implicitWidth: telemetryLabel.implicitWidth + Style.pad * 2

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: Style.padXs
                        spacing: Style.padXs

                        Rectangle {
                            width: 8; height: 8; radius: 4
                            color: Style.ok
                        }

                        Label {
                            id: telemetryLabel
                            text: "Active DB: Swiss Screening | User: " + App.session.username + " (" + App.session.role + ")"
                            font.family: Style.fontSecondary
                            font.pixelSize: Style.fontSm - 1
                            color: Style.text
                        }
                    }
                }

                Button {
                    text: "Logout"
                    font.family: Style.fontSecondaryBold
                    font.pixelSize: Style.fontSm
                    onClicked: App.session.logout()
                    implicitHeight: 28
                }
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Style.padXl
        spacing: Style.padXl

        Item { Layout.fillHeight: true } // top spacer

        ColumnLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: Style.padXs

            Label {
                text: "Welcome to Invenesis Master Hub"
                font.family: Style.fontPrimaryBold
                font.pixelSize: Style.fontXxl
                color: Style.text
                Layout.alignment: Qt.AlignHCenter
            }

            Label {
                text: "Swiss quality data • Choose application module"
                font.family: Style.fontSecondary
                font.pixelSize: Style.fontMd
                color: Style.subText
                Layout.alignment: Qt.AlignHCenter
            }
        }

        Item { height: Style.pad } // spacing gap

        // Premium Cards Layout
        Flickable {
            id: cardCarousel
            Layout.fillWidth: true
            Layout.preferredHeight: 380
            contentWidth: cardRow.implicitWidth + 40
            contentHeight: 380
            boundsBehavior: Flickable.StopAtBounds
            clip: true


            RowLayout {
                id: cardRow
                y: (parent.height - implicitHeight) / 2
                x: 20
                spacing: Style.padXl

            // Card 1: Screening Database Manager (Widgets App)
            Frame {
                id: cardWidgets
                Layout.preferredWidth: 380
                Layout.preferredHeight: 320
                padding: Style.padLg
                
                scale: mouseWidgets.containsMouse ? 1.03 : 1.0
                Behavior on scale { NumberAnimation { duration: 150; easing.type: Easing.OutCubic } }

                background: Rectangle {
                    radius: Style.radiusLg
                    color: Style.panel
                    border.color: mouseWidgets.containsMouse ? Style.teal : Style.border
                    border.width: mouseWidgets.containsMouse ? 2 : Style.borderWidth

                    // Glassmorphism subtle glow
                    Rectangle {
                        anchors.fill: parent
                        radius: parent.radius
                        color: "transparent"
                        border.color: Qt.rgba(Style.teal.r, Style.teal.g, Style.teal.b, 0.2)
                        border.width: 1
                        visible: mouseWidgets.containsMouse
                    }
                }

                ColumnLayout {
                    anchors.fill: parent
                    spacing: Style.pad

                    // Card header
                    RowLayout {
                        spacing: Style.pad
                        
                        Label {
                            text: "grid_view" // Grid icon
                            font.family: Style.iconFontFamily
                            font.pixelSize: Style.fontXxl + 6
                            color: Style.teal
                        }

                        ColumnLayout {
                            spacing: 2
                            Label {
                                text: "Screening Database"
                                font.family: Style.fontPrimaryBold
                                font.pixelSize: Style.fontLg
                                color: Style.text
                            }
                            Label {
                                text: "Qt Widgets Desktop client"
                                font.family: Style.fontSecondary
                                font.pixelSize: Style.fontXs
                                color: Style.subText
                            }
                        }
                    }

                    // Card Body
                    Text {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        text: "Direct integration with local laboratory databases. Includes compound registration models, plate mappings, microplate readers (Tecan integration), and manual database view operations."
                        wrapMode: Text.Wrap
                        color: Style.subText
                        font.family: Style.fontSecondary
                        font.pixelSize: Style.fontSm + 1
                        lineHeight: 1.25
                    }

                    // Tech Tags
                    RowLayout {
                        spacing: Style.padXs
                        Layout.fillWidth: true

                        Repeater {
                            model: ["Widgets", "C++ Core", "Tecan Logs", "Raw DB Access"]
                            Rectangle {
                                color: Qt.rgba(Style.accent2.r, Style.accent2.g, Style.accent2.b, 0.15)
                                border.color: Style.border
                                radius: 4
                                implicitWidth: tagText.implicitWidth + 12
                                implicitHeight: tagText.implicitHeight + 6

                                Label {
                                    id: tagText
                                    anchors.centerIn: parent
                                    text: modelData
                                    font.family: Style.fontSecondary
                                    font.pixelSize: Style.fontXs - 1
                                    color: Style.text
                                }
                            }
                        }
                    }

                    Button {
                        Layout.fillWidth: true
                        text: "Launch Screening DB"
                        onClicked: App.launchWidgetsApp()
                    }
                }

                MouseArea {
                    id: mouseWidgets
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: App.launchWidgetsApp()
                }
            }

            // Card 2: Test Requests Tracking (QML App)
            Frame {
                id: cardQml
                Layout.preferredWidth: 380
                Layout.preferredHeight: 320
                padding: Style.padLg
                
                scale: mouseQml.containsMouse ? 1.03 : 1.0
                Behavior on scale { NumberAnimation { duration: 150; easing.type: Easing.OutCubic } }

                background: Rectangle {
                    radius: Style.radiusLg
                    color: Style.panel
                    border.color: mouseQml.containsMouse ? Style.teal : Style.border
                    border.width: mouseQml.containsMouse ? 2 : Style.borderWidth

                    // Glassmorphism subtle glow
                    Rectangle {
                        anchors.fill: parent
                        radius: parent.radius
                        color: "transparent"
                        border.color: Qt.rgba(Style.teal.r, Style.teal.g, Style.teal.b, 0.2)
                        border.width: 1
                        visible: mouseQml.containsMouse
                    }
                }

                ColumnLayout {
                    anchors.fill: parent
                    spacing: Style.pad

                    // Card header
                    RowLayout {
                        spacing: Style.pad
                        
                        Label {
                            text: "" // Chart/Dashboard icon
                            font.family: Style.iconFontFamily
                            font.pixelSize: Style.fontXxl + 6
                            color: Style.teal
                        }

                        ColumnLayout {
                            spacing: 2
                            Label {
                                text: "Request Dashboard"
                                font.family: Style.fontPrimaryBold
                                font.pixelSize: Style.fontLg
                                color: Style.text
                            }
                            Label {
                                text: "Asynchronous QML client"
                                font.family: Style.fontSecondary
                                font.pixelSize: Style.fontXs
                                color: Style.subText
                            }
                        }
                    }

                    // Card Body
                    Text {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        text: "Highly personalized tracking dashboard. Offers interactive charts of pending batches, a detailed scientific calendar for schedules, asynchronous done-status synchronization, and automated PDF export reports."
                        wrapMode: Text.Wrap
                        color: Style.subText
                        font.family: Style.fontSecondary
                        font.pixelSize: Style.fontSm + 1
                        lineHeight: 1.25
                    }

                    // Tech Tags
                    RowLayout {
                        spacing: Style.padXs
                        Layout.fillWidth: true

                        Repeater {
                            model: ["QML Quick", "Async Workers", "Excel Import", "Reports"]
                            Rectangle {
                                color: Qt.rgba(Style.accent2.r, Style.accent2.g, Style.accent2.b, 0.15)
                                border.color: Style.border
                                radius: 4
                                implicitWidth: tagText2.implicitWidth + 12
                                implicitHeight: tagText2.implicitHeight + 6

                                Label {
                                    id: tagText2
                                    anchors.centerIn: parent
                                    text: modelData
                                    font.family: Style.fontSecondary
                                    font.pixelSize: Style.fontXs - 1
                                    color: Style.text
                                }
                            }
                        }
                    }

                    Button {
                        Layout.fillWidth: true
                        text: "Launch Dashboard"
                        onClicked: win.launchDashboardWindow()
                    }
                }

                MouseArea {
                    id: mouseQml
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: win.launchDashboardWindow()
                }
            }
            // Card 3: Standalone Offline Analysis
            Frame {
                id: cardStandalone
                Layout.preferredWidth: 380
                Layout.preferredHeight: 320
                padding: Style.padLg
                
                scale: mouseStandalone.containsMouse ? 1.03 : 1.0
                Behavior on scale { NumberAnimation { duration: 150; easing.type: Easing.OutCubic } }

                background: Rectangle {
                    radius: Style.radiusLg
                    color: Style.panel
                    border.color: mouseStandalone.containsMouse ? Style.teal : Style.border
                    border.width: mouseStandalone.containsMouse ? 2 : Style.borderWidth

                    // Glassmorphism subtle glow
                    Rectangle {
                        anchors.fill: parent
                        radius: parent.radius
                        color: "transparent"
                        border.color: Qt.rgba(Style.teal.r, Style.teal.g, Style.teal.b, 0.2)
                        border.width: 1
                        visible: mouseStandalone.containsMouse
                    }
                }

                ColumnLayout {
                    anchors.fill: parent
                    spacing: Style.pad

                    // Card header
                    RowLayout {
                        spacing: Style.pad
                        
                        Label {
                            text: "" // Analytics/Science icon
                            font.family: Style.iconFontFamily
                            font.pixelSize: Style.fontXxl + 6
                            color: Style.teal
                        }

                        ColumnLayout {
                            spacing: 2
                            Label {
                                text: "Offline Analysis"
                                font.family: Style.fontPrimaryBold
                                font.pixelSize: Style.fontLg
                                color: Style.text
                            }
                            Label {
                                text: "Standalone math engine"
                                font.family: Style.fontSecondary
                                font.pixelSize: Style.fontXs
                                color: Style.subText
                            }
                        }
                    }

                    // Card Body
                    Text {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        text: "Run analysis locally without a database connection. Load raw instrument data and plate layouts manually, perform standard or custom efficacy calculations, and export results and graphs directly to your computer."
                        wrapMode: Text.Wrap
                        color: Style.subText
                        font.family: Style.fontSecondary
                        font.pixelSize: Style.fontSm + 1
                        lineHeight: 1.25
                    }

                    // Tech Tags
                    RowLayout {
                        spacing: Style.padXs
                        Layout.fillWidth: true

                        Repeater {
                            model: ["Local Only", "CSV Export", "Math Engine", "Plots"]
                            Rectangle {
                                color: Qt.rgba(Style.accent2.r, Style.accent2.g, Style.accent2.b, 0.15)
                                border.color: Style.border
                                radius: 4
                                implicitWidth: tagText3.implicitWidth + 12
                                implicitHeight: tagText3.implicitHeight + 6

                                Label {
                                    id: tagText3
                                    anchors.centerIn: parent
                                    text: modelData
                                    font.family: Style.fontSecondary
                                    font.pixelSize: Style.fontXs - 1
                                    color: Style.text
                                }
                            }
                        }
                    }

                    Button {
                        Layout.fillWidth: true
                        text: "Launch Offline Analysis"
                        onClicked: win.launchStandaloneAnalysis()
                    }
                }

                MouseArea {
                    id: mouseStandalone
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: win.launchStandaloneAnalysis()
                }
            }

            // Card 4: Datapoint & Monoplicate Manager
            Frame {
                id: cardDatapoint
                Layout.preferredWidth: 380
                Layout.preferredHeight: 320
                padding: Style.padLg
                
                scale: mouseDatapoint.containsMouse ? 1.03 : 1.0
                Behavior on scale { NumberAnimation { duration: 150; easing.type: Easing.OutCubic } }

                background: Rectangle {
                    radius: Style.radiusLg
                    color: Style.panel
                    border.color: mouseDatapoint.containsMouse ? Style.teal : Style.border
                    border.width: mouseDatapoint.containsMouse ? 2 : Style.borderWidth

                    // Glassmorphism subtle glow
                    Rectangle {
                        anchors.fill: parent
                        radius: parent.radius
                        color: "transparent"
                        border.color: Qt.rgba(Style.teal.r, Style.teal.g, Style.teal.b, 0.2)
                        border.width: 1
                        visible: mouseDatapoint.containsMouse
                    }
                }

                ColumnLayout {
                    anchors.fill: parent
                    spacing: Style.pad

                    // Card header
                    RowLayout {
                        spacing: Style.pad
                        
                        Label {
                            text: "" // Database/Money icon (approximate)
                            font.family: Style.iconFontFamily
                            font.pixelSize: Style.fontXxl + 6
                            color: Style.teal
                        }

                        ColumnLayout {
                            spacing: 2
                            Label {
                                text: "Datapoint Manager"
                                font.family: Style.fontPrimaryBold
                                font.pixelSize: Style.fontLg
                                color: Style.text
                            }
                            Label {
                                text: "Contract & Pricing grids"
                                font.family: Style.fontSecondary
                                font.pixelSize: Style.fontXs
                                color: Style.subText
                            }
                        }
                    }

                    // Card Body
                    Text {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        text: "Manage annualized client contracts, track monoplicate consumption, configure custom pricing overrides, and monitor overall datapoint pools for all your testing partners."
                        wrapMode: Text.Wrap
                        color: Style.subText
                        font.family: Style.fontSecondary
                        font.pixelSize: Style.fontSm + 1
                        lineHeight: 1.25
                    }

                    // Tech Tags
                    RowLayout {
                        spacing: Style.padXs
                        Layout.fillWidth: true

                        Repeater {
                            model: ["Contracts", "Pricing", "Monoplicates"]
                            Rectangle {
                                color: Qt.rgba(Style.accent2.r, Style.accent2.g, Style.accent2.b, 0.15)
                                border.color: Style.border
                                radius: 4
                                implicitWidth: tagText4.implicitWidth + 12
                                implicitHeight: tagText4.implicitHeight + 6

                                Label {
                                    id: tagText4
                                    anchors.centerIn: parent
                                    text: modelData
                                    font.family: Style.fontSecondary
                                    font.pixelSize: Style.fontXs - 1
                                    color: Style.text
                                }
                            }
                        }
                    }

                    Button {
                        Layout.fillWidth: true
                        text: "Launch Datapoint Manager"
                        onClicked: win.launchDatapointManager()
                    }
                }

                MouseArea {
                    id: mouseDatapoint
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: win.launchDatapointManager()
                }
            }
        }

        }

        PageIndicator {
            id: pageIndicator
            Layout.alignment: Qt.AlignHCenter
            count: 4
            currentIndex: Math.max(0, Math.min(count - 1, Math.round(cardCarousel.contentX / (cardRow.implicitWidth / count))))
            
            delegate: Rectangle {
                implicitWidth: 8
                implicitHeight: 8
                radius: width / 2
                color: index === pageIndicator.currentIndex ? Style.teal : Style.subText
                opacity: index === pageIndicator.currentIndex ? 1.0 : 0.4
                Behavior on color { ColorAnimation { duration: 150 } }
                Behavior on opacity { NumberAnimation { duration: 150 } }
            }
        }
        
        Item { Layout.fillHeight: true } // bottom spacer
    }

    // ---------------- Update overlay ----------------
    Rectangle {
        id: updateOverlay
        anchors.fill: parent
        color: Qt.rgba(0, 0, 0, 0.75) // dark semi-transparent backdrop
        visible: false // controlled by signals
        z: 9999 // ensure it sits on top of everything

        property string latestVersion: ""
        property string releaseNotes: ""

        // Prevent mouse events from leaking to items underneath
        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            acceptedButtons: Qt.AllButtons
        }

        // The Glassmorphic Modal Box
        Rectangle {
            anchors.centerIn: parent
            width: 550
            height: 420
            radius: Style.radiusLg
            color: Style.panel
            border.color: Style.teal
            border.width: 1

            // Subtle glow
            layer.enabled: true
            layer.effect: DropShadow {
                transparentBorder: true
                horizontalOffset: 0
                verticalOffset: 4
                radius: 20
                samples: 25
                color: Qt.rgba(Style.teal.r, Style.teal.g, Style.teal.b, 0.25)
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Style.padXl
                spacing: Style.padLg

                // Header
                RowLayout {
                    Layout.fillWidth: true
                    spacing: Style.pad

                    Label {
                        text: "\ue89c" // Download/Update arrow icon (Material Symbols hex)
                        font.family: Style.iconFontFamily
                        font.pixelSize: Style.fontXl + 4
                        color: Style.teal
                    }

                    ColumnLayout {
                        spacing: 2
                        Label {
                            text: "New Update Available"
                            font.family: Style.fontPrimaryBold
                            font.pixelSize: Style.fontLg
                            color: Style.text
                        }
                        Label {
                            text: "Version " + updateOverlay.latestVersion + " is ready for installation"
                            font.family: Style.fontSecondary
                            font.pixelSize: Style.fontXs
                            color: Style.subText
                        }
                    }
                }

                // Divider
                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: Style.divider
                }

                // Release Notes Scrollable Area
                ScrollView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true

                    TextArea {
                        text: updateOverlay.releaseNotes
                        textFormat: TextEdit.RichText
                        readOnly: true
                        selectByMouse: true
                        wrapMode: TextEdit.Wrap
                        color: Style.text
                        font.family: Style.fontSecondary
                        font.pixelSize: Style.fontSm + 1
                        background: null
                        leftPadding: 0
                        topPadding: 0
                    }
                }

                // Divider
                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: Style.divider
                }

                // Download Progress (visible when downloading)
                ColumnLayout {
                    Layout.fillWidth: true
                    visible: UpdateManager.isDownloading
                    spacing: Style.padXs

                    // Custom styled progress bar
                    Rectangle {
                        Layout.fillWidth: true
                        height: 6
                        color: Style.panel2
                        radius: 3

                        Rectangle {
                            height: parent.height
                            width: parent.width * UpdateManager.downloadProgress
                            color: Style.teal
                            radius: 3
                            
                            Behavior on width {
                                NumberAnimation { duration: 150 }
                            }
                        }
                    }

                    Label {
                        text: "Downloading update... " + Math.round(UpdateManager.downloadProgress * 100) + "%"
                        font.family: Style.fontSecondary
                        font.pixelSize: Style.fontXs
                        color: Style.subText
                        Layout.alignment: Qt.AlignHCenter
                    }
                }

                // Action Buttons
                RowLayout {
                    Layout.fillWidth: true
                    spacing: Style.padLg
                    visible: !UpdateManager.isDownloading

                    // Custom Secondary button using standard Rectangle + MouseArea
                    Rectangle {
                        id: skipBtn
                        implicitWidth: 120
                        implicitHeight: 38
                        radius: 19
                        color: skipMouse.containsMouse ? Style.panel2 : "transparent"
                        border.color: Style.border
                        border.width: 1

                        Label {
                            anchors.centerIn: parent
                            text: "Skip for Now"
                            font.family: Style.fontPrimaryBold
                            font.pixelSize: Style.fontSm + 1
                            color: Style.text
                        }

                        MouseArea {
                            id: skipMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: updateOverlay.visible = false
                        }
                    }

                    Item { Layout.fillWidth: true } // spacer

                    // Primary Action Button (Using app's custom Button)
                    Button {
                        id: updateBtn
                        text: UpdateManager.downloadProgress === 1.0 ? "Restart & Install" : "Update Now"
                        implicitHeight: 38
                        implicitWidth: 160
                        onClicked: {
                            if (UpdateManager.downloadProgress === 1.0) {
                                UpdateManager.installAndExit()
                            } else {
                                UpdateManager.startDownload()
                            }
                        }
                    }
                }
            }
        }
    }
}
