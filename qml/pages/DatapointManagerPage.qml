import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import TestRequests 1.0
import "../components"
import "../controls"

Page {
    id: root
    
    property var viewModel: App.datapointManager
    
    Component.onCompleted: {
        viewModel.loadClients()
        viewModel.loadAnalytics()
    }
    
    Connections {
        target: viewModel
        function onSuccessMessage(msg) {
            console.log("Success:", msg)
        }
        function onErrorOccurred(msg) {
            console.error("Error:", msg)
        }
    }
    
    background: Rectangle { color: Style.bg }

    header: ColumnLayout {
        spacing: 0
        
        ToolBar {
            Layout.fillWidth: true
            
            RowLayout {
                anchors.fill: parent
                anchors.margins: Style.pad
                
                Label {
                    text: "Datapoint & Monoplicate Manager"
                    font.family: Style.fontPrimaryBold
                    font.pixelSize: Style.fontLg
                    color: Style.text
                }
                
                Item { Layout.fillWidth: true }
                
                Button {
                    text: "Refresh"
                    onClicked: {
                        viewModel.loadClients()
                        viewModel.loadAnalytics()
                    }
                }
            }
        }
        
        TabBar {
            id: tabBar
            Layout.fillWidth: true
            TabButton { text: "Clients & Overrides" }
            TabButton { text: "Analytics & Reports" }
        }
    }

    contentItem: Item {
        StackLayout {
            anchors.fill: parent
            currentIndex: tabBar.currentIndex
        
        // Tab 0: Clients
        RowLayout {
            anchors.margins: Style.padLg
            spacing: Style.padLg
        
        // Left Column: Client List
        Frame {
            Layout.fillHeight: true
            Layout.preferredWidth: 400
            padding: 0
            
            background: Rectangle {
                color: Style.panel
                border.color: Style.border
                radius: Style.radius
            }
            
            ColumnLayout {
                anchors.fill: parent
                
                Label {
                    text: "Clients"
                    font.family: Style.fontPrimaryBold
                    font.pixelSize: Style.fontMd
                    color: Style.text
                    Layout.margins: Style.pad
                }
                
                Rectangle { Layout.fillWidth: true; height: 1; color: Style.border }
                
                ListView {
                    id: clientListView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: ListModel { id: clientsModel; dynamicRoles: true }
                    
                    Connections {
                        target: viewModel
                        function onClientsChanged() {
                            clientsModel.clear()
                            if (viewModel && viewModel.clients) {
                                for (let i = 0; i < viewModel.clients.length; i++) {
                                    clientsModel.append(viewModel.clients[i])
                                }
                            }
                        }
                    }
                    
                    property var selectedClientData: null
                    
                    delegate: Rectangle {
                        width: ListView.view.width
                        height: 70
                        color: clientListView.currentIndex === index ? Style.panel2 : "transparent"
                        
                        Rectangle {
                            width: 3; height: parent.height
                            color: Style.teal
                            visible: clientListView.currentIndex === index
                        }
                        
                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                clientListView.currentIndex = index
                                clientListView.selectedClientData = {
                                    "clientCode": clientCode,
                                    "companyName": companyName,
                                    "isAnnualized": isAnnualized,
                                    "totalDatapoints": totalDatapoints,
                                    "usedDatapoints": usedDatapoints
                                }
                                viewModel.loadPricingOverrides(clientCode)
                            }
                        }
                        
                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: Style.pad
                            
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2
                                Label {
                                    text: companyName + " (" + clientCode + ")"
                                    font.family: Style.fontPrimaryBold
                                    color: Style.text
                                }
                                Label {
                                    text: isAnnualized ? "Annualized | " + totalDatapoints + " DP" : "Standard Contract"
                                    font.family: Style.fontSecondary
                                    font.pixelSize: Style.fontXs
                                    color: isAnnualized ? Style.ok : Style.subText
                                }
                            }
                        }
                        
                        Rectangle { width: parent.width; height: 1; color: Style.divider; anchors.bottom: parent.bottom }
                    }
                }
            }
        }
        
        // Right Column: Client Details & Overrides
        Frame {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: clientListView.selectedClientData !== null
            
            background: Rectangle {
                color: Style.panel
                border.color: Style.border
                radius: Style.radius
            }
            
            ColumnLayout {
                anchors.fill: parent
                spacing: Style.padLg
                
                // Client Header info
                RowLayout {
                    Layout.fillWidth: true
                    spacing: Style.pad
                    
                    Label {
                        text: clientListView.selectedClientData ? clientListView.selectedClientData.companyName : ""
                        font.family: Style.fontPrimaryBold
                        font.pixelSize: Style.fontXl
                        color: Style.text
                    }
                    
                    Item { Layout.fillWidth: true }
                    
                    Switch {
                        id: annualizeSwitch
                        text: "Is Annualized"
                        checked: clientListView.selectedClientData ? clientListView.selectedClientData.isAnnualized : false
                        onClicked: {
                            viewModel.toggleAnnualization(clientListView.selectedClientData.clientCode, checked)
                        }
                    }
                }
                
                Rectangle { Layout.fillWidth: true; height: 1; color: Style.border }
                
                // Datapoint Management
                GroupBox {
                    Layout.fillWidth: true
                    title: "Datapoint Management"
                    visible: clientListView.selectedClientData ? clientListView.selectedClientData.isAnnualized : false
                    
                    GridLayout {
                        columns: 2
                        rowSpacing: Style.pad
                        columnSpacing: Style.padLg
                        
                        Label { text: "Total Datapoints (Pool):"; color: Style.text }
                        SpinBox {
                            id: totalDpSpin
                            from: 0; to: 9999999
                            value: clientListView.selectedClientData ? clientListView.selectedClientData.totalDatapoints : 0
                        }
                        
                        Label { text: "Used Datapoints:"; color: Style.text }
                        SpinBox {
                            id: usedDpSpin
                            from: 0; to: 9999999
                            value: clientListView.selectedClientData ? clientListView.selectedClientData.usedDatapoints : 0
                        }
                        
                        Item { Layout.columnSpan: 2; height: Style.pad }
                        
                        Button {
                            Layout.columnSpan: 2
                            Layout.alignment: Qt.AlignRight
                            text: "Save Datapoints"
                            onClicked: {
                                viewModel.updateDatapoints(clientListView.selectedClientData.clientCode, totalDpSpin.value, usedDpSpin.value)
                            }
                        }
                    }
                }
                
                // Custom Pricing Overrides
                GroupBox {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    title: "Custom Test Pricing Overrides"
                    
                    ColumnLayout {
                        anchors.fill: parent
                        
                        ListView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true
                            model: ListModel { id: pricingModel; dynamicRoles: true }
                            
                            Connections {
                                target: viewModel
                                function onSelectedClientPricingChanged() {
                                    pricingModel.clear()
                                    if (viewModel && viewModel.selectedClientPricing) {
                                        for (let i = 0; i < viewModel.selectedClientPricing.length; i++) {
                                            pricingModel.append(viewModel.selectedClientPricing[i])
                                        }
                                    }
                                }
                            }
                            
                            header: RowLayout {
                                width: ListView.view.width
                                spacing: Style.pad
                                Label { text: "Test ID"; Layout.preferredWidth: 100; font.bold: true; color: Style.subText }
                                Label { text: "Ratio Pri"; Layout.preferredWidth: 80; font.bold: true; color: Style.subText }
                                Label { text: "Ratio Sec"; Layout.preferredWidth: 80; font.bold: true; color: Style.subText }
                                Label { text: "Fee"; Layout.preferredWidth: 60; font.bold: true; color: Style.subText }
                                Label { text: "Min/Req"; Layout.preferredWidth: 80; font.bold: true; color: Style.subText }
                            }
                            
                            delegate: RowLayout {
                                width: ListView.view.width
                                height: 40
                                spacing: Style.pad
                                
                                Label { text: testId; Layout.preferredWidth: 100; color: Style.text }
                                Label { text: Number(ratioPrimary).toFixed(2); Layout.preferredWidth: 80; color: Style.text }
                                Label { text: Number(ratioSecondary).toFixed(2); Layout.preferredWidth: 80; color: Style.text }
                                Label { text: testsetFee; Layout.preferredWidth: 60; color: Style.text }
                                Label { text: minPerRequest; Layout.preferredWidth: 80; color: Style.text }
                                
                                Item { Layout.fillWidth: true }
                                
                                Button {
                                    text: "Remove"
                                    flat: true
                                    onClicked: viewModel.deletePricingOverride(id, clientListView.selectedClientData.clientCode)
                                }
                            }
                        }
                        
                        Rectangle { Layout.fillWidth: true; height: 1; color: Style.border }
                        
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: Style.pad
                            
                            TextField { id: testIdField; placeholderText: "Test ID (e.g. M1)"; Layout.preferredWidth: 100 }
                            TextField { id: rPriField; placeholderText: "Pri Ratio"; Layout.preferredWidth: 80 }
                            TextField { id: rSecField; placeholderText: "Sec Ratio"; Layout.preferredWidth: 80 }
                            TextField { id: feeField; placeholderText: "Fee"; Layout.preferredWidth: 60 }
                            TextField { id: minField; placeholderText: "Min"; Layout.preferredWidth: 60 }
                            
                            Button {
                                text: "Add Override"
                                onClicked: {
                                    let pri = parseFloat(rPriField.text) || 1.0
                                    let sec = parseFloat(rSecField.text) || 1.0
                                    let fee = parseInt(feeField.text) || 0
                                    let min = parseInt(minField.text) || 0
                                    viewModel.savePricingOverride(0, clientListView.selectedClientData.clientCode, testIdField.text, pri, sec, fee, min)
                                    testIdField.text = ""
                                    rPriField.text = ""
                                    rSecField.text = ""
                                    feeField.text = ""
                                    minField.text = ""
                                }
                            }
                        }
                    }
                }
                }
            }
        }
        
        // Tab 1: Analytics
        AnalyticsDashboard {
            viewModel: root.viewModel
        }
        }
    }
}
