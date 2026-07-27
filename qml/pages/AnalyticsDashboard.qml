import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import TestRequests 1.0
import "../components"

Item {
    id: root
    property var viewModel

    ScrollView {
        id: scrollView
        anchors.fill: parent
        anchors.margins: Style.padLg
        clip: true
        contentWidth: availableWidth

        ColumnLayout {
            width: scrollView.availableWidth
            spacing: Style.padLg
            
            // Client Filter
            RowLayout {
                Layout.fillWidth: true
                spacing: Style.pad
                
                Label {
                    text: "Filter by Client:"
                    font.pixelSize: Style.fontMd
                    color: Style.subText
                }
                
                ComboBox {
                    id: clientFilter
                    Layout.preferredWidth: 300
                    textRole: "text"
                    valueRole: "value"
                    
                    model: ListModel { id: clientModel }
                    
                    Component.onCompleted: updateModel()
                    
                    Connections {
                        target: viewModel
                        function onClientsChanged() {
                            clientFilter.updateModel()
                        }
                    }
                    
                    function updateModel() {
                        clientModel.clear()
                        clientModel.append({ text: "All Clients", value: "" })
                        if (viewModel && viewModel.clients) {
                            for (let i = 0; i < viewModel.clients.length; i++) {
                                let c = viewModel.clients[i]
                                clientModel.append({ text: c.companyName + " (" + c.clientCode + ")", value: c.clientCode })
                            }
                        }
                    }
                    
                    onActivated: {
                        viewModel.setAnalyticsClient(currentValue)
                    }
                }
                
                Item { Layout.fillWidth: true }
            }

            // Stats Grid
            GridLayout {
                Layout.fillWidth: true
                columns: 3
                columnSpacing: Style.padLg
                
                // Active Contracts
                Rectangle {
                    Layout.fillWidth: true
                    height: 120
                    color: Style.panel
                    border.color: Style.border
                    border.width: 1
                    radius: Style.radius
                    ColumnLayout {
                        anchors.centerIn: parent
                        Label {
                            text: "Active Annualized Contracts"
                            color: Style.subText
                            font.pixelSize: Style.fontMd
                            Layout.alignment: Qt.AlignHCenter
                        }
                        Label {
                            text: viewModel ? viewModel.activeContracts : "0"
                            color: Style.text
                            font.pixelSize: Style.fontXxl
                            font.family: Style.fontPrimaryBold
                            Layout.alignment: Qt.AlignHCenter
                        }
                    }
                }
                
                // Total Pool
                Rectangle {
                    Layout.fillWidth: true
                    height: 120
                    color: Style.panel
                    border.color: Style.border
                    border.width: 1
                    radius: Style.radius
                    ColumnLayout {
                        anchors.centerIn: parent
                        Label {
                            text: "Total Monoplicates Pool"
                            color: Style.subText
                            font.pixelSize: Style.fontMd
                            Layout.alignment: Qt.AlignHCenter
                        }
                        Label {
                            text: viewModel ? viewModel.totalMonoplicatesPool : "0"
                            color: Style.teal
                            font.pixelSize: Style.fontXxl
                            font.family: Style.fontPrimaryBold
                            Layout.alignment: Qt.AlignHCenter
                        }
                    }
                }

                // Total Consumed
                Rectangle {
                    Layout.fillWidth: true
                    height: 120
                    color: Style.panel
                    border.color: Style.border
                    border.width: 1
                    radius: Style.radius
                    ColumnLayout {
                        anchors.centerIn: parent
                        Label {
                            text: "Total Monoplicates Consumed"
                            color: Style.subText
                            font.pixelSize: Style.fontMd
                            Layout.alignment: Qt.AlignHCenter
                        }
                        Label {
                            text: viewModel ? viewModel.totalMonoplicatesConsumed : "0"
                            color: Style.warn
                            font.pixelSize: Style.fontXxl
                            font.family: Style.fontPrimaryBold
                            Layout.alignment: Qt.AlignHCenter
                        }
                    }
                }
            }

            // Charts
            GridLayout {
                Layout.fillWidth: true
                columns: 2
                columnSpacing: Style.padLg
                rowSpacing: Style.padLg

                // Client Consumption Chart
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 400
                    color: Style.panel
                    border.color: Style.border
                    border.width: 1
                    radius: Style.radius

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: Style.padLg
                        Label {
                            text: "Client Consumption"
                            font.pixelSize: Style.fontLg
                            font.family: Style.fontPrimaryBold
                            color: Style.text
                        }
                        PieChart {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            dataModel: viewModel ? viewModel.clientConsumptionStats : []
                        }
                    }
                }

                // Assay Type Chart
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 400
                    color: Style.panel
                    border.color: Style.border
                    border.width: 1
                    radius: Style.radius

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: Style.padLg
                        Label {
                            text: "Assay Type Distribution"
                            font.pixelSize: Style.fontLg
                            font.family: Style.fontPrimaryBold
                            color: Style.text
                        }
                        PieChart {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            dataModel: viewModel ? viewModel.assayTypeStats : []
                        }
                    }
                }

                // Test Category Chart
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 400
                    color: Style.panel
                    border.color: Style.border
                    border.width: 1
                    radius: Style.radius

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: Style.padLg
                        Label {
                            text: "Test Categories (Click legend to filter)"
                            font.pixelSize: Style.fontLg
                            font.family: Style.fontPrimaryBold
                            color: Style.text
                        }
                        PieChart {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            dataModel: viewModel ? viewModel.testCategoryStats : []
                            onItemClicked: function(name) {
                                if (viewModel && name) viewModel.selectCategory(name)
                            }
                        }
                    }
                }

                // Test Distribution Chart
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 400
                    color: Style.panel
                    border.color: Style.border
                    border.width: 1
                    radius: Style.radius
                    visible: viewModel && viewModel.selectedCategory !== ""

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: Style.padLg
                        Label {
                            text: "Test Distribution: " + (viewModel ? viewModel.selectedCategory : "")
                            font.pixelSize: Style.fontLg
                            font.family: Style.fontPrimaryBold
                            color: Style.text
                        }
                        PieChart {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            dataModel: viewModel ? viewModel.testSpecificStats : []
                        }
                    }
                }
            }
            
            Item {
                Layout.preferredHeight: Style.padLg
                Layout.fillWidth: true
            }
        }
    }
}
