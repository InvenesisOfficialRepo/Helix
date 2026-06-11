import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import TestRequests 1.0
import "../components"
import "../controls"

Page {
    id: root
    focus: true

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
        RowLayout {
            Layout.alignment: Qt.AlignHCenter
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
                            text: "" // Grid icon
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
        }

        Item { Layout.fillHeight: true } // bottom spacer
    }
}
