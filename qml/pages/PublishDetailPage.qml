import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtQuick.Window
import TestRequests 1.0
import QtGraphs

import "../components"
import "../controls"
import "../dialogs"

Page {
    id: root
    background: null

    // Input URLs
    property var layoutUrls: []
    property var rawDataUrls: []
    property var feedingDataUrls: []
    property url expJsonUrl: ""
    property string expJsonName: ""
    property string selectedTimepoint: "24h"
    
    // PDF Report URL for publishing
    property url selectedPdfUrl: ""
    property string selectedPdfName: ""

    // Results state
    property string selectedCompoundName: ""
    property int selectedPlateNum: 1
    property var currentPlateWells: ({})

    readonly property bool isDone: App.project.overallStatus === "Done"
    readonly property bool hasReport: App.project.hasReport

    // Custom Graph Styling Properties
    property color graphBgColor: Style.panel2
    property color graphPlotAreaColor: "transparent"
    property real curveWidth: 3.5
    property real pointSize: 10.0
    property real controlCurveWidth: 2.5
    property real controlPointSize: 8.0
    
    property color testColor: "#ec4899"
    property color standardColor: "#3b82f6"
    property color qcColor: "#f97316"
    
    property real axisXMinVal: -2.0
    property real axisXMaxVal: 1.5
    property real axisYMinVal: -10.0
    property real axisYMaxVal: 110.0

    function popPage() {
        var w = root.Window.window
        if (w && w.appStack) w.appStack.pop()
        else console.error("PublishDetailPage: window/appStack not available")
    }

    // Helper to convert URL to local file path
    function urlToLocalPath(url) {
        var s = url.toString();
        if (s.startsWith("file:///")) {
            s = s.substring(8);
        } else if (s.startsWith("file://")) {
            s = s.substring(7);
        }
        return decodeURIComponent(s);
    }

    // Helper to format a QML color to an HTML hex string (#RRGGBB)
    function colorToHex(c) {
        var r = Math.round(c.r * 255).toString(16);
        var g = Math.round(c.g * 255).toString(16);
        var b = Math.round(c.b * 255).toString(16);
        if (r.length < 2) r = "0" + r;
        if (g.length < 2) g = "0" + g;
        if (b.length < 2) b = "0" + b;
        return "#" + r + g + b;
    }

    // Asynchronous loop to grab images of all compounds and save to folder
    function exportAllGraphs(folderPath) {
        var compounds = App.project.analysisResults.compounds || [];
        if (compounds.length === 0) {
            customizeButton.visible = true;
            exportOverlay.visible = false;
            exportSuccessDialog.open();
            return;
        }

        // Save original tab index and selection
        var originalTab = inspectTabs.currentIndex;
        var originalSelection = root.selectedCompoundName;
        
        // Hide customize button during export so it doesn't appear in the grabbed image
        customizeButton.visible = false;
        
        // Force switch to Dose-Response tab (Index 0) to ensure rendering
        inspectTabs.currentIndex = 0;
        
        function processIndex(idx) {
            if (idx >= compounds.length) {
                // Restore original selections
                root.selectedCompoundName = originalSelection;
                root.updateCurves(root.selectedCompoundName);
                inspectTabs.currentIndex = originalTab;
                
                // Show customize button again
                customizeButton.visible = true;
                
                exportOverlay.visible = false;
                exportSuccessDialog.open();
                return;
            }
            
            var c = compounds[idx];
            if (!c || !c.compound_name || c.is_standard || c.is_qc || c.compound_name === "" || c.compound_name === "DMSO") {
                processIndex(idx + 1);
                return;
            }
            
            root.selectedCompoundName = c.compound_name;
            root.updateCurves(root.selectedCompoundName);
            
            var timer = Qt.createQmlObject("import QtQuick; Timer { interval: 150; repeat: false; running: true }", root);
            timer.triggered.connect(function() {
                chartContainer.grabToImage(function(result) {
                    var cleanName = c.compound_name.replace(/[^a-zA-Z0-9_-]/g, "_");
                    var filePath = folderPath + "/" + cleanName + "_curve.png";
                    result.saveToFile(filePath);
                    timer.destroy();
                    processIndex(idx + 1);
                });
            });
        }

        processIndex(0);
    }

    // Color gradient for the Tecan plate heatmap
    function getEfficacyColor(efficacy) {
        var t = Math.max(0.0, Math.min(100.0, efficacy)) / 100.0;
        var r, g, b;
        if (Style.isDark) {
            // Dark mode gradient: Neutral Slate (#1e293b) to Vibrant Purple (#8b5cf6)
            r = Math.round(30 + t * (139 - 30));
            g = Math.round(41 + t * (92 - 41));
            b = Math.round(59 + t * (246 - 59));
        } else {
            // Light mode gradient: Light Slate (#e2e8f0) to Vibrant Purple (#8b5cf6)
            r = Math.round(226 + t * (139 - 226));
            g = Math.round(232 + t * (92 - 232));
            b = Math.round(240 + t * (246 - 240));
        }
        return Qt.rgba(r / 255.0, g / 255.0, b / 255.0, 1.0);
    }

    // Update the local plate wells map for fast O(1) rendering lookups
    function updatePlateWells(plateNum) {
        var wells = App.project.analysisResults.wells || [];
        var map = {};
        for (var i = 0; i < wells.length; ++i) {
            var w = wells[i];
            if (w.plate === plateNum) {
                map[w.well] = w;
            }
        }
        currentPlateWells = map;
    }

    // Populate the 2D LineSeries and ScatterSeries for Standard, QC and selected compound
    function updateCurves(selectedCompoundName) {
        standardSeries.clear();
        qcSeries.clear();
        testSeries.clear();
        standardPoints.clear();
        qcPoints.clear();
        testPoints.clear();
        
        var curves = App.project.analysisResults.curves || {};
        var compounds = App.project.analysisResults.compounds || [];
        
        var standardKey = "";
        var qcKey = "";
        var testKey = selectedCompoundName;
        
        // Find compound objects to get replicate points and curve keys
        var standardComp = null;
        var qcComp = null;
        var testComp = null;
        for (var idx = 0; idx < compounds.length; ++idx) {
            var c = compounds[idx];
            if (c.compound_name === selectedCompoundName) {
                testComp = c;
            }
            if (c.is_standard) {
                standardComp = c;
                standardKey = c.compound_name;
            }
            if (c.is_qc) {
                qcComp = c;
                qcKey = c.compound_name;
            }
        }
        
        // Helper to populate ScatterSeries
        function populateScatter(series, comp) {
            series.clear();
            if (comp && comp.replicate_points) {
                var pts = comp.replicate_points;
                for (var j = 0; j < pts.length; ++j) {
                    var dose = pts[j].x;
                    if (dose > 0) {
                        series.append(Math.log(dose) / Math.LN10, pts[j].y);
                    }
                }
            }
        }
        
        // Populate standard curves & points
        if (standardKey && curves[standardKey]) {
            var pts = curves[standardKey];
            for (var i = 0; i < pts.length; ++i) {
                var dose = pts[i].x;
                if (dose > 0) {
                    standardSeries.append(Math.log(dose) / Math.LN10, pts[i].y);
                }
            }
        }
        populateScatter(standardPoints, standardComp);
        
        // Populate QC curves & points
        if (qcKey && curves[qcKey]) {
            var pts = curves[qcKey];
            for (var i = 0; i < pts.length; ++i) {
                var dose = pts[i].x;
                if (dose > 0) {
                    qcSeries.append(Math.log(dose) / Math.LN10, pts[i].y);
                }
            }
        }
        populateScatter(qcPoints, qcComp);
        
        // Populate test compound curves & points
        if (testKey && curves[testKey] && !testKey.toLowerCase().includes("standard") && !testKey.toLowerCase().includes("qc")) {
            var pts = curves[testKey];
            for (var i = 0; i < pts.length; ++i) {
                var dose = pts[i].x;
                if (dose > 0) {
                    testSeries.append(Math.log(dose) / Math.LN10, pts[i].y);
                }
            }
        }
        if (testComp && !testComp.is_standard && !testComp.is_qc) {
            populateScatter(testPoints, testComp);
        }
    }

    Connections {
        target: App.project
        
        function onPublishFinished(success, errorMsg) {
            if (success) {
                root.layoutUrls = []
                root.rawDataUrls = []
                root.feedingDataUrls = []
                root.expJsonUrl = ""
                root.expJsonName = ""
                root.selectedPdfUrl = ""
                root.selectedPdfName = ""
                popPage()
            }
        }

        function onAnalysisResultsChanged() {
            if (App.project.hasResults) {
                var cmps = App.project.analysisResults.compounds || [];
                if (cmps.length > 0) {
                    root.selectedCompoundName = cmps[0].compound_name;
                    root.updateCurves(root.selectedCompoundName);
                }
                root.selectedPlateNum = 1;
                root.updatePlateWells(root.selectedPlateNum);
            }
        }
    }

    // File Dialogs
    FileDialog {
        id: layoutDialog
        title: "Select Layout CSV Files"
        nameFilters: ["CSV files (*.csv)"]
        fileMode: FileDialog.OpenFiles
        onAccepted: {
            root.layoutUrls = selectedFiles
        }
    }

    FileDialog {
        id: rawDataDialog
        title: "Select Raw Data CSV Files"
        nameFilters: ["CSV files (*.csv)"]
        fileMode: FileDialog.OpenFiles
        onAccepted: {
            root.rawDataUrls = selectedFiles
        }
    }

    FileDialog {
        id: expJsonDialog
        title: "Select Experiment JSON File"
        nameFilters: ["JSON files (*.json)"]
        onAccepted: {
            root.expJsonUrl = selectedFile
            root.expJsonName = ("" + selectedFile).split("/").pop()
        }
    }

    FileDialog {
        id: pdfDialog
        title: "Select Bioactivity PDF Report"
        nameFilters: ["PDF reports (*.pdf)"]
        onAccepted: {
            root.selectedPdfUrl = selectedFile
            root.selectedPdfName = ("" + selectedFile).split("/").pop()
        }
    }

    FeedingAnalysisWindow {
        id: feedingWindow
        layoutUrls: root.layoutUrls
        onAnalysisApplied: function(rawPaths) {
            root.feedingDataUrls = rawPaths;
        }
    }

    FolderDialog {
        id: exportFolderDialog
        title: "Select Folder to Export Analysis"
        onAccepted: {
            exportOverlay.visible = true
            var ok = App.project.exportAnalysis(selectedFolder)
            if (ok) {
                var folderPath = root.urlToLocalPath(selectedFolder)
                root.exportAllGraphs(folderPath)
            } else {
                exportOverlay.visible = false
                exportErrorDialog.open()
            }
        }
    }

    MessageDialog {
        id: exportSuccessDialog
        title: "Export Successful"
        text: "The analysis files (merged CSVs, summary, and JSON data) were successfully exported to the selected folder."
        buttons: MessageDialog.Ok
    }

    MessageDialog {
        id: exportErrorDialog
        title: "Export Failed"
        text: "Failed to export analysis. Make sure the folder is writable and results are loaded."
        buttons: MessageDialog.Ok
    }

    ColorDialog {
        id: activeCompoundColorDialog
        title: "Select Active Compound Color"
        selectedColor: root.testColor
        onAccepted: {
            root.testColor = selectedColor
        }
    }

    ColorDialog {
        id: standardColorDialog
        title: "Select Standard Control Color"
        selectedColor: root.standardColor
        onAccepted: {
            root.standardColor = selectedColor
        }
    }

    ColorDialog {
        id: qcColorDialog
        title: "Select QC Control Color"
        selectedColor: root.qcColor
        onAccepted: {
            root.qcColor = selectedColor
        }
    }

    // Helper component for Wizard step indicator
    component StepIndicator : RowLayout {
        property int stepNumber: 1
        property string title: ""
        property bool active: false
        property bool completed: false
        
        spacing: 8
        
        Rectangle {
            width: 28
            height: 28
            radius: 14
            color: completed ? Style.accent : (active ? Style.panel2 : "transparent")
            border.color: completed || active ? Style.accent : Style.border
            border.width: 2
            
            Text {
                anchors.centerIn: parent
                text: completed ? "✓" : stepNumber
                font.family: Style.fontPrimaryBold
                font.pixelSize: Style.fontSm
                color: completed ? "#ffffff" : (active ? Style.accent : Style.subText)
            }
        }
        
        Text {
            text: title
            font.family: Style.fontPrimaryBold
            font.pixelSize: Style.fontSm
            color: active || completed ? Style.text : Style.subText
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Style.pad
        spacing: Style.pad

        ProjectHeader {
            title: "Analyze & Publish Batch"
            onBackClicked: popPage()
        }

        ProjectInfoCard {
            batchId: App.project.batchId
            projectCode: "Project: " + App.project.projectCode
            testCode: App.project.testCode
            overallStatus: App.project.overallStatus
            scheduledFor: App.project.scheduledFor
            createdBy: App.project.createdBy
        }

        // Beautiful Wizard Progress Header
        Rectangle {
            Layout.fillWidth: true
            height: 48
            radius: 8
            color: Style.panel
            border.color: Style.border
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 16

                StepIndicator {
                    stepNumber: 1
                    title: "Load Inputs"
                    active: !root.hasReport && !App.project.hasResults
                    completed: root.hasReport || App.project.hasResults
                }
                Rectangle { height: 2; Layout.fillWidth: true; color: (root.hasReport || App.project.hasResults) ? Style.accent : Style.border }
                StepIndicator {
                    stepNumber: 2
                    title: "Run Analysis"
                    active: !root.hasReport && App.project.busy
                    completed: root.hasReport || App.project.hasResults
                }
                Rectangle { height: 2; Layout.fillWidth: true; color: (root.hasReport || App.project.hasResults) ? Style.accent : Style.border }
                StepIndicator {
                    stepNumber: 3
                    title: "Inspect Results"
                    active: !root.hasReport && App.project.hasResults && root.selectedPdfUrl === ""
                    completed: root.hasReport || (App.project.hasResults && root.selectedPdfUrl !== "")
                }
                Rectangle { height: 2; Layout.fillWidth: true; color: (root.hasReport || root.selectedPdfUrl !== "") ? Style.accent : Style.border }
                StepIndicator {
                    stepNumber: 4
                    title: "Publish"
                    active: !root.hasReport && App.project.hasResults && root.selectedPdfUrl !== ""
                    completed: root.hasReport
                }
            }
        }

        ErrorBanner {
            message: App.project.errorMessage
            severity: "error"
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: Style.padLg

            // Left Panel: File uploads & Analysis Trigger
            ColumnLayout {
                Layout.preferredWidth: App.project.hasResults ? 360 : -1
                Layout.fillWidth: !App.project.hasResults
                Layout.fillHeight: true
                spacing: Style.pad

                // Step 1: Input Selectors
                Rectangle {
                    visible: !root.hasReport
                    Layout.fillWidth: true
                    Layout.preferredHeight: 330
                    radius: 12
                    color: Style.panel
                    border.color: Style.border
                    border.width: 1

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 12

                        Label {
                            text: "1. Load Input Files"
                            font.family: Style.fontPrimaryBold
                            font.pixelSize: Style.fontMd
                            color: Style.text
                        }

                        // Layout CSV Selector
                        Rectangle {
                            Layout.fillWidth: true
                            height: 48
                            color: Style.panel2
                            border.color: root.layoutUrls.length > 0 ? Style.accent : Style.border
                            radius: 6
                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 8
                                Text {
                                    text: root.layoutUrls.length > 0 ? root.layoutUrls.length + " layout CSV(s) selected" : "Select Layout CSVs"
                                    font.family: Style.fontSecondary
                                    font.pixelSize: Style.fontXs
                                    color: root.layoutUrls.length > 0 ? Style.text : Style.subText
                                    elide: Text.ElideMiddle
                                    Layout.fillWidth: true
                                }
                                Button {
                                    text: "Browse"
                                    onClicked: layoutDialog.open()
                                }
                            }
                        }

                        // Raw Data / Video Selector
                        Rectangle {
                            Layout.fillWidth: true
                            height: 48
                            color: Style.panel2
                            border.color: (root.rawDataUrls.length > 0 || root.feedingDataUrls.length > 0) ? Style.accent : Style.border
                            radius: 6
                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 8
                                Text {
                                    text: {
                                        var msgs = []
                                        if (root.rawDataUrls.length > 0) {
                                            msgs.push(root.rawDataUrls.length + " raw motility CSV(s)")
                                        }
                                        if (root.feedingDataUrls.length > 0) {
                                            msgs.push(root.feedingDataUrls.length + " feeding plate(s)")
                                        }
                                        if (msgs.length > 0) {
                                            return msgs.join(" + ") + " selected"
                                        }
                                        if (App.project.testCode === "INV-T-009") {
                                            return "Flea Feeding Video / CSV Raw Data"
                                        }
                                        return "Select Raw Data CSVs"
                                    }
                                    font.family: Style.fontSecondary
                                    font.pixelSize: Style.fontXs
                                    color: (root.rawDataUrls.length > 0 || root.feedingDataUrls.length > 0) ? Style.text : Style.subText
                                    elide: Text.ElideMiddle
                                    Layout.fillWidth: true
                                }
                                Button {
                                    text: "Browse"
                                    onClicked: rawDataDialog.open()
                                }
                                Button {
                                    visible: App.project.testCode === "INV-T-009"
                                    text: "Analyze Videos"
                                    onClicked: feedingWindow.show()
                                }
                            }
                        }

                        // Experiment JSON Selector
                        Rectangle {
                            Layout.fillWidth: true
                            height: 48
                            color: Style.panel2
                            border.color: root.expJsonUrl !== "" ? Style.accent : Style.border
                            radius: 6
                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 8
                                Text {
                                    text: root.expJsonName !== "" ? root.expJsonName : "Select experiment.json (Optional)"
                                    font.family: Style.fontSecondary
                                    font.pixelSize: Style.fontXs
                                    color: root.expJsonUrl !== "" ? Style.text : Style.subText
                                    elide: Text.ElideMiddle
                                    Layout.fillWidth: true
                                }
                                Button {
                                    text: "Browse"
                                    onClicked: expJsonDialog.open()
                                }
                            }
                        }

                        // Timepoint Selector Row
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8
                            Label {
                                text: "Timepoint:"
                                font.family: Style.fontPrimaryBold
                                font.pixelSize: Style.fontXs
                                color: Style.text
                            }
                            ComboBox {
                                id: timepointCombo
                                Layout.fillWidth: true
                                model: ["6h", "24h", "48h", "72h", "96h"]
                                currentIndex: 1 // default 24h
                                onCurrentIndexChanged: {
                                    root.selectedTimepoint = currentText;
                                }
                            }
                        }
                    }
                }

                // Step 2 & 4: Pipeline Trigger & PDF Publisher
                Rectangle {
                    visible: !root.hasReport
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    radius: 12
                    color: Style.panel
                    border.color: Style.border
                    border.width: 1

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 12

                        Label {
                            text: "2. Process & Publish"
                            font.family: Style.fontPrimaryBold
                            font.pixelSize: Style.fontMd
                            color: Style.text
                        }

                        Button {
                            text: "Run Analysis Pipeline"
                            Layout.fillWidth: true
                            enabled: root.layoutUrls.length > 0 && root.rawDataUrls.length > 0 && !App.project.busy
                            onClicked: {
                                App.project.runAnalysis(root.layoutUrls, root.rawDataUrls, root.expJsonUrl, root.selectedTimepoint, root.feedingDataUrls)
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            height: 1
                            color: Style.border
                        }

                        Label {
                            text: "3. Choose Bioactivity PDF Report"
                            font.family: Style.fontPrimaryBold
                            font.pixelSize: Style.fontSm
                            color: Style.text
                            visible: App.project.hasResults
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            height: 48
                            color: Style.panel2
                            border.color: root.selectedPdfUrl !== "" ? Style.accent : Style.border
                            radius: 6
                            visible: App.project.hasResults
                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 8
                                Text {
                                    text: root.selectedPdfName !== "" ? root.selectedPdfName : "Select Bioactivity PDF"
                                    font.family: Style.fontSecondary
                                    font.pixelSize: Style.fontXs
                                    color: root.selectedPdfUrl !== "" ? Style.text : Style.subText
                                    elide: Text.ElideMiddle
                                    Layout.fillWidth: true
                                }
                                Button {
                                    text: "Browse"
                                    onClicked: pdfDialog.open()
                                }
                            }
                        }

                        Item { Layout.fillHeight: true }

                        Button {
                            text: "Publish Results"
                            Layout.fillWidth: true
                            highlighted: true
                            enabled: App.project.hasResults && root.selectedPdfUrl !== "" && !App.project.busy
                            onClicked: {
                                // Publishes result with generated plate CSVs (from pipeline) and the PDF report
                                App.project.publishResults(root.selectedPdfUrl, App.project.analysisResults.merged_files)
                            }
                        }
                    }
                }

                // Already Published View
                Rectangle {
                    visible: root.hasReport
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    radius: 12
                    color: Style.panel
                    border.color: Style.border
                    border.width: 1
                    
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 24
                        spacing: 16
                        
                        Item { Layout.fillHeight: true }
                        
                        Rectangle {
                            width: 64
                            height: 64
                            radius: 32
                            color: Style.wash(0.1, 0.15)
                            border.color: Style.accent
                            border.width: 2
                            Layout.alignment: Qt.AlignHCenter
                            
                            Text {
                                anchors.centerIn: parent
                                text: "check"
                                font.family: Style.iconFontFamily
                                font.pixelSize: 32
                                color: Style.accent
                            }
                        }
                        
                        Label {
                            text: "Analysis Already Published"
                            font.family: Style.fontPrimaryBold
                            font.pixelSize: Style.fontLg
                            color: Style.text
                            Layout.alignment: Qt.AlignHCenter
                        }
                        
                        Text {
                            text: "This batch has been successfully analyzed and its results are published to the client portal.\n\nIf the data contains errors or you need to perform a new test run, you can reject the current results. This will delete all published efficacies, plate layout files, and the PDF report, and reset the batch status back to active."
                            font.family: Style.fontSecondary
                            font.pixelSize: Style.fontSm
                            color: Style.subText
                            wrapMode: Text.WordWrap
                            horizontalAlignment: Text.AlignHCenter
                            Layout.fillWidth: true
                        }
                        
                        Item { height: 16 }
                        
                        Button {
                            text: "Reject & Redo Test"
                            Layout.fillWidth: true
                            Layout.maximumWidth: 240
                            Layout.alignment: Qt.AlignHCenter
                            highlighted: true
                            enabled: !App.project.busy
                            onClicked: App.project.rejectAndRedoBatch()
                        }
                        
                        Item { Layout.fillHeight: true }
                    }
                }
            }

            // Right Panel: Results Visualization (Curves, Heatmap, Efficacies Table)
            Rectangle {
                visible: App.project.hasResults
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: 12
                color: Style.panel
                border.color: Style.border
                border.width: 1

                // Interactive outcome preview
                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 16
                    visible: App.project.hasResults

                    // Outcome Compounds List (Left Column)
                    ColumnLayout {
                        Layout.preferredWidth: 280
                        Layout.fillHeight: true
                        spacing: 8

                        Label {
                            text: "Compounds Summary"
                            font.family: Style.fontPrimaryBold
                            font.pixelSize: Style.fontSm
                            color: Style.text
                        }

                        Frame {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            background: Rectangle {
                                color: Style.panel2
                                border.color: Style.border
                                radius: 8
                            }

                            ListView {
                                id: compoundListView
                                anchors.fill: parent
                                clip: true
                                model: App.project.analysisResults.compounds || []
                                
                                delegate: ItemDelegate {
                                    width: compoundListView.width
                                    height: 52
                                    
                                    background: Rectangle {
                                        color: modelData.compound_name === root.selectedCompoundName ? 
                                               (Style.isDark ? "#2d2640" : "#f3e8ff") : 
                                               (hovered ? (Style.isDark ? "#1e293b" : "#f1f5f9") : "transparent")
                                        border.color: modelData.compound_name === root.selectedCompoundName ? Style.accent : "transparent"
                                        border.width: 1
                                        radius: 6
                                    }
                                    
                                    onClicked: {
                                        root.selectedCompoundName = modelData.compound_name;
                                        root.updateCurves(root.selectedCompoundName);
                                    }
                                    
                                    contentItem: RowLayout {
                                        anchors.fill: parent
                                        anchors.margins: 8
                                        spacing: 8
                                        
                                        ColumnLayout {
                                            spacing: 2
                                            Layout.fillWidth: true
                                            
                                            Text {
                                                text: modelData.compound_name
                                                font.family: Style.fontPrimaryBold
                                                font.pixelSize: Style.fontSm
                                                color: Style.text
                                                elide: Text.ElideRight
                                            }
                                            
                                            Text {
                                                text: modelData.is_standard ? "Standard Control" : (modelData.is_qc ? "QC Control" : "Test Compound")
                                                font.family: Style.fontSecondary
                                                font.pixelSize: Style.fontXs
                                                color: Style.subText
                                            }
                                        }
                                        
                                        ColumnLayout {
                                            spacing: 2
                                            Layout.alignment: Qt.AlignRight
                                            
                                            Text {
                                                text: (modelData.ec50 > 0 ? "EC50: " + modelData.ec50.toFixed(4) : "EC50: N/A") +
                                                      "\n" +
                                                      (modelData.ec80 > 0 ? "EC80: " + modelData.ec80.toFixed(4) : "EC80: N/A")
                                                font.family: Style.fontPrimaryBold
                                                font.pixelSize: Style.fontXs
                                                color: modelData.status === "Active" ? "#10b981" : Style.subText
                                                horizontalAlignment: Text.AlignRight
                                            }
                                            
                                            Rectangle {
                                                width: 50
                                                height: 16
                                                radius: 4
                                                color: modelData.status === "Active" ? "#d1fae5" : "#fee2e2"
                                                visible: !modelData.is_standard && !modelData.is_qc
                                                Layout.alignment: Qt.AlignRight
                                                
                                                Text {
                                                    anchors.centerIn: parent
                                                    text: modelData.status
                                                    font.family: Style.fontPrimaryBold
                                                    font.pixelSize: 9
                                                    color: modelData.status === "Active" ? "#065f46" : "#991b1b"
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        Button {
                            text: "Export Analysis Data"
                            Layout.fillWidth: true
                            highlighted: true
                            font.family: Style.fontPrimaryBold
                            onClicked: exportFolderDialog.open()
                        }
                    }

                    // Interactive Tab Panel (Right Column)
                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        spacing: 8

                        TabBar {
                            id: inspectTabs
                            Layout.fillWidth: true
                            background: Rectangle { color: "transparent" }

                            TabButton {
                                text: "Dose-Response"
                                font.family: Style.fontPrimaryBold
                            }
                            TabButton {
                                text: "Plate Heatmaps"
                                font.family: Style.fontPrimaryBold
                            }
                            TabButton {
                                text: "Quality Control"
                                font.family: Style.fontPrimaryBold
                            }
                            TabButton {
                                text: "Detailed Inspection"
                                font.family: Style.fontPrimaryBold
                            }
                        }

                        StackLayout {
                            currentIndex: inspectTabs.currentIndex
                            Layout.fillWidth: true
                            Layout.fillHeight: true

                            // Tab 1: 2D QtGraphs Dose-Response Curves
                            Rectangle {
                                id: chartContainer
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                color: root.graphBgColor
                                border.color: Style.border
                                radius: 8

                                ColumnLayout {
                                    anchors.fill: parent
                                    anchors.margins: 12
                                    spacing: 4

                                    RowLayout {
                                        Layout.fillWidth: true
                                        Label {
                                            text: App.project.testCode === "INV-T-009" 
                                                ? "Curves: " + root.selectedCompoundName + " vs Controls (Feeding)"
                                                : "Curves: " + root.selectedCompoundName + " vs Controls (Log10 scale)"
                                            font.family: Style.fontPrimaryBold
                                            font.pixelSize: Style.fontSm
                                            color: Style.text
                                            Layout.fillWidth: true
                                        }
                                        Button {
                                            id: customizeButton
                                            text: "⚙ Customize"
                                            font.family: Style.fontPrimaryBold
                                            font.pixelSize: Style.fontXs
                                            onClicked: graphSettingsDrawer.open()
                                        }
                                    }

                                    GraphsView {
                                        id: curvesChart
                                        Layout.fillWidth: true
                                        Layout.fillHeight: true

                                        theme: GraphsTheme {
                                            theme: GraphsTheme.Theme.UserDefined
                                            backgroundColor: root.graphBgColor
                                            backgroundVisible: true
                                            plotAreaBackgroundColor: root.graphPlotAreaColor
                                            plotAreaBackgroundVisible: root.graphPlotAreaColor !== "transparent"
                                            gridVisible: true
                                            grid.mainColor: Style.border
                                        }

                                        axisX: ValueAxis {
                                            id: axisX
                                            min: root.axisXMinVal
                                            max: root.axisXMaxVal
                                            titleText: "Log10 Concentration (uM)"
                                            titleVisible: true
                                        }
                                        axisY: ValueAxis {
                                            id: axisY
                                            min: root.axisYMinVal
                                            max: root.axisYMaxVal
                                            titleText: App.project.testCode === "INV-T-009" ? "Feeding Reduction (%)" : "Efficacy (%)"
                                            titleVisible: true
                                        }

                                        LineSeries {
                                            id: standardSeries
                                            color: root.standardColor
                                            width: root.controlCurveWidth
                                        }

                                        LineSeries {
                                            id: qcSeries
                                            color: root.qcColor
                                            width: root.controlCurveWidth
                                        }

                                        LineSeries {
                                            id: testSeries
                                            color: root.testColor
                                            width: root.curveWidth
                                        }

                                        ScatterSeries {
                                            id: standardPoints
                                            color: root.standardColor
                                            pointDelegate: Rectangle {
                                                width: root.controlPointSize
                                                height: root.controlPointSize
                                                radius: width / 2
                                                color: root.standardColor
                                            }
                                        }

                                        ScatterSeries {
                                            id: qcPoints
                                            color: root.qcColor
                                            pointDelegate: Rectangle {
                                                width: root.controlPointSize
                                                height: root.controlPointSize
                                                radius: width / 2
                                                color: root.qcColor
                                            }
                                        }

                                        ScatterSeries {
                                            id: testPoints
                                            color: root.testColor
                                            pointDelegate: Rectangle {
                                                width: root.pointSize
                                                height: root.pointSize
                                                radius: width / 2
                                                color: root.testColor
                                            }
                                        }
                                    }

                                    // Legend
                                    RowLayout {
                                        Layout.alignment: Qt.AlignHCenter
                                        spacing: 20
                                        
                                        RowLayout {
                                            spacing: 6
                                            Rectangle { width: 12; height: 12; color: root.standardColor; radius: 2 }
                                            Text { text: "Standard Control"; font.family: Style.fontSecondary; font.pixelSize: Style.fontXs; color: Style.text }
                                        }
                                        RowLayout {
                                            spacing: 6
                                            Rectangle { width: 12; height: 12; color: root.qcColor; radius: 2 }
                                            Text { text: "QC Control"; font.family: Style.fontSecondary; font.pixelSize: Style.fontXs; color: Style.text }
                                        }
                                        RowLayout {
                                            spacing: 6
                                            Rectangle { width: 12; height: 12; color: root.testColor; radius: 2 }
                                            Text { text: root.selectedCompoundName !== "" ? root.selectedCompoundName : "Active Compound"; font.family: Style.fontSecondary; font.pixelSize: Style.fontXs; color: Style.text }
                                        }
                                    }
                                }
                            }

                            // Tab 2: Tecan 96-well Heatmap Grid
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                color: Style.panel2
                                border.color: Style.border
                                radius: 8

                                ColumnLayout {
                                    anchors.fill: parent
                                    anchors.margins: 12
                                    spacing: 12

                                    // Plate selection and info
                                    RowLayout {
                                        Layout.fillWidth: true
                                        spacing: 12

                                        Label {
                                            text: "Test Plate:"
                                            font.family: Style.fontPrimaryBold
                                            font.pixelSize: Style.fontSm
                                            color: Style.text
                                        }

                                        ComboBox {
                                            id: plateSelector
                                            Layout.preferredWidth: 200
                                            font.family: Style.fontSecondary
                                            model: {
                                                var list = App.project.analysisResults.merged_files || [];
                                                var arr = [];
                                                for (var i = 0; i < list.length; ++i) {
                                                    var filename = ("" + list[i]).split("/").pop();
                                                    var barcode = filename.replace("merged_", "").replace(".csv", "");
                                                    arr.push(barcode);
                                                }
                                                return arr;
                                            }
                                            onCurrentIndexChanged: {
                                                root.selectedPlateNum = currentIndex + 1;
                                                root.updatePlateWells(root.selectedPlateNum);
                                            }
                                        }

                                        Item { Layout.fillWidth: true }

                                        Label {
                                            text: "Click a well to highlight the corresponding compound"
                                            font.family: Style.fontSecondary
                                            font.pixelSize: Style.fontXs
                                            color: Style.subText
                                        }
                                    }

                                    // 96-well visualizer grid layout
                                    Column {
                                        spacing: 4
                                        Layout.alignment: Qt.AlignHCenter
                                        Layout.fillHeight: true

                                        // Column Headers (1 to 12)
                                        Row {
                                            spacing: 4
                                            Item { width: 20; height: 20 } // Spacer for row letters
                                            Repeater {
                                                model: 12
                                                Text {
                                                    text: modelData + 1
                                                    font.family: Style.fontPrimaryBold
                                                    font.pixelSize: 10
                                                    color: Style.subText
                                                    width: 28
                                                    height: 20
                                                    horizontalAlignment: Text.AlignHCenter
                                                    verticalAlignment: Text.AlignVCenter
                                                }
                                            }
                                        }

                                        // Rows A to H
                                        Repeater {
                                            model: 8 // A-H
                                            
                                            Row {
                                                spacing: 4
                                                property int rIdx: modelData
                                                property string rowLetter: String.fromCharCode(65 + rIdx)

                                                // Row Header label (A, B, C...)
                                                Text {
                                                    text: parent.rowLetter
                                                    font.family: Style.fontPrimaryBold
                                                    font.pixelSize: 10
                                                    color: Style.subText
                                                    width: 20
                                                    height: 28
                                                    horizontalAlignment: Text.AlignHCenter
                                                    verticalAlignment: Text.AlignVCenter
                                                }

                                                // Wells
                                                Repeater {
                                                    model: 12 // Cols 1-12
                                                    
                                                    Rectangle {
                                                        width: 28
                                                        height: 28
                                                        radius: 14
                                                        
                                                        property int cIdx: modelData + 1
                                                        property string colStr: cIdx < 10 ? "0" + cIdx : "" + cIdx
                                                        property string wellKey: parent.rowLetter + colStr
                                                        property var wellData: root.currentPlateWells[wellKey] || null
                                                        
                                                        color: wellData ? root.getEfficacyColor(wellData.efficacy) : (Style.isDark ? "#1e293b" : "#e2e8f0")
                                                        border.color: wellData && wellData.compound === root.selectedCompoundName ? Style.accent : (wellHoverArea.containsMouse ? Style.accent2 : Style.border)
                                                        border.width: wellData && wellData.compound === root.selectedCompoundName ? 2.0 : 1.0
                                                        
                                                        MouseArea {
                                                            id: wellHoverArea
                                                            anchors.fill: parent
                                                            hoverEnabled: true
                                                            onClicked: {
                                                                if (parent.wellData && parent.wellData.compound !== "Empty" && parent.wellData.compound !== "DMSO") {
                                                                    root.selectedCompoundName = parent.wellData.compound;
                                                                    root.updateCurves(root.selectedCompoundName);
                                                                }
                                                            }
                                                        }
                                                        
                                                        ToolTip {
                                                            visible: wellHoverArea.containsMouse && parent.wellData !== null
                                                            delay: 100
                                                            text: parent.wellData ? 
                                                                  "Well: " + parent.wellKey + "\n" +
                                                                  "Compound: " + parent.wellData.compound + "\n" +
                                                                  "Dose: " + (parent.wellData.dose > 0 ? parent.wellData.dose + " uM" : "N/A") + "\n" +
                                                                  "Efficacy: " + parent.wellData.efficacy.toFixed(1) + "%" 
                                                                  : ""
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }

                                    // Heatmap gradient legend
                                    RowLayout {
                                        Layout.alignment: Qt.AlignHCenter
                                        spacing: 8
                                        Text { text: "0% Efficacy"; font.family: Style.fontSecondary; font.pixelSize: Style.fontXs; color: Style.subText }
                                        Rectangle {
                                            width: 120
                                            height: 12
                                            radius: 2
                                            gradient: Gradient {
                                                orientation: Gradient.Horizontal
                                                GradientStop { position: 0.0; color: Style.isDark ? "#1e293b" : "#e2e8f0" }
                                                GradientStop { position: 1.0; color: "#8b5cf6" }
                                            }
                                        }
                                        Text { text: "100% Efficacy"; font.family: Style.fontSecondary; font.pixelSize: Style.fontXs; color: Style.subText }
                                    }
                                }
                            }

                            // Tab 3: Quality Control Dashboard
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                color: Style.panel2
                                border.color: Style.border
                                radius: 8

                                ColumnLayout {
                                    anchors.fill: parent
                                    anchors.margins: 16
                                    spacing: 16

                                    Label {
                                        text: "Quality Control Metrics"
                                        font.family: Style.fontPrimaryBold
                                        font.pixelSize: Style.fontMd
                                        color: Style.text
                                    }

                                    // Metric Cards Row
                                    RowLayout {
                                        Layout.fillWidth: true
                                        spacing: 16

                                        // Card 1: Placebo Bleed-through
                                        Rectangle {
                                            Layout.fillWidth: true
                                            height: 100
                                            color: Style.panel
                                            border.color: Style.border
                                            radius: 8
                                            ColumnLayout {
                                                anchors.centerIn: parent
                                                spacing: 4
                                                Text {
                                                    text: "Placebo Bleed-through Rate"
                                                    font.family: Style.fontSecondary
                                                    font.pixelSize: Style.fontXs; color: Style.subText
                                                    horizontalAlignment: Text.AlignHCenter
                                                }
                                                Text {
                                                    property var qc: App.project.analysisResults.qc || {}
                                                    text: qc.pctBleed !== undefined ? qc.pctBleed.toFixed(2) + "%" : "N/A"
                                                    font.family: Style.fontPrimaryBold
                                                    font.pixelSize: Style.fontLg
                                                    color: {
                                                        var val = qc.pctBleed || 0.0;
                                                        return val > 8.0 ? "#ef4444" : (val > 4.8 ? "#f59e0b" : "#10b981");
                                                    }
                                                    horizontalAlignment: Text.AlignHCenter
                                                }
                                                Text {
                                                    property var qc: App.project.analysisResults.qc || {}
                                                    text: qc.nBleed !== undefined ? "(" + qc.nBleed + " / " + qc.nPlacebo + " wells > 80% eff)" : ""
                                                    font.family: Style.fontSecondary
                                                    font.pixelSize: 9
                                                    color: Style.subText
                                                    horizontalAlignment: Text.AlignHCenter
                                                }
                                            }
                                        }

                                        // Card 2: Placebo Signal Level
                                        Rectangle {
                                            Layout.fillWidth: true
                                            height: 100
                                            color: Style.panel
                                            border.color: Style.border
                                            radius: 8
                                            ColumnLayout {
                                                anchors.centerIn: parent
                                                spacing: 4
                                                Text {
                                                    text: "Placebo Motility Signal (Mean)"
                                                    font.family: Style.fontSecondary
                                                    font.pixelSize: Style.fontXs; color: Style.subText
                                                    horizontalAlignment: Text.AlignHCenter
                                                }
                                                Text {
                                                    property var qc: App.project.analysisResults.qc || {}
                                                    text: qc.meanRaw !== undefined ? qc.meanRaw.toFixed(2) : "N/A"
                                                    font.family: Style.fontPrimaryBold
                                                    font.pixelSize: Style.fontLg
                                                    color: {
                                                        var val = qc.meanRaw || 0.0;
                                                        return val < 15.0 ? "#ef4444" : (val < 19.5 ? "#f59e0b" : "#10b981");
                                                    }
                                                    horizontalAlignment: Text.AlignHCenter
                                                }
                                                Text {
                                                    property var qc: App.project.analysisResults.qc || {}
                                                    text: qc.medRaw !== undefined ? "(Median: " + qc.medRaw.toFixed(2) + ")" : ""
                                                    font.family: Style.fontSecondary
                                                    font.pixelSize: 9
                                                    color: Style.subText
                                                    horizontalAlignment: Text.AlignHCenter
                                                }
                                            }
                                        }
                                    }

                                    // Per-plate QC Status List
                                    Label {
                                        text: "Per-Plate Status"
                                        font.family: Style.fontPrimaryBold
                                        font.pixelSize: Style.fontSm
                                        color: Style.text
                                    }

                                    Frame {
                                        Layout.fillWidth: true
                                        Layout.fillHeight: true
                                        background: Rectangle {
                                            color: Style.panel
                                            border.color: Style.border
                                            radius: 8
                                        }

                                        ListView {
                                            id: qcListView
                                            anchors.fill: parent
                                            clip: true
                                            model: {
                                                var qc = App.project.analysisResults.qc || {};
                                                return qc.per_plate || [];
                                            }
                                            header: Rectangle {
                                                width: qcListView.width
                                                height: 32
                                                color: Style.panel2
                                                border.color: Style.border
                                                RowLayout {
                                                    anchors.fill: parent
                                                    anchors.leftMargin: 8
                                                    anchors.rightMargin: 8
                                                    Text { text: "Plate Barcode"; font.family: Style.fontPrimaryBold; font.pixelSize: Style.fontXs; color: Style.text; Layout.fillWidth: true }
                                                    Text { text: "DMSO Wells"; font.family: Style.fontPrimaryBold; font.pixelSize: Style.fontXs; color: Style.text; Layout.preferredWidth: 90; horizontalAlignment: Text.AlignHCenter }
                                                    Text { text: "Bleed-through %"; font.family: Style.fontPrimaryBold; font.pixelSize: Style.fontXs; color: Style.text; Layout.preferredWidth: 110; horizontalAlignment: Text.AlignHCenter }
                                                    Text { text: "Mean Motility"; font.family: Style.fontPrimaryBold; font.pixelSize: Style.fontXs; color: Style.text; Layout.preferredWidth: 110; horizontalAlignment: Text.AlignHCenter }
                                                    Text { text: "Status"; font.family: Style.fontPrimaryBold; font.pixelSize: Style.fontXs; color: Style.text; Layout.preferredWidth: 80; horizontalAlignment: Text.AlignHCenter }
                                                }
                                            }
                                            delegate: Rectangle {
                                                width: qcListView.width
                                                height: 40
                                                color: index % 2 === 0 ? "transparent" : Style.panel2
                                                border.color: Style.border
                                                border.width: 0.5
                                                RowLayout {
                                                    anchors.fill: parent
                                                    anchors.leftMargin: 8
                                                    anchors.rightMargin: 8
                                                    Text { text: modelData.barcode; font.family: Style.fontSecondary; font.pixelSize: Style.fontXs; color: Style.text; Layout.fillWidth: true }
                                                    Text { text: modelData.n; font.family: Style.fontSecondary; font.pixelSize: Style.fontXs; color: Style.text; Layout.preferredWidth: 90; horizontalAlignment: Text.AlignHCenter }
                                                    Text { text: modelData.pctBleed.toFixed(1) + "%"; font.family: Style.fontSecondary; font.pixelSize: Style.fontXs; color: Style.text; Layout.preferredWidth: 110; horizontalAlignment: Text.AlignHCenter }
                                                    Text { text: modelData.meanRaw.toFixed(2); font.family: Style.fontSecondary; font.pixelSize: Style.fontXs; color: Style.text; Layout.preferredWidth: 110; horizontalAlignment: Text.AlignHCenter }
                                                    Rectangle {
                                                        Layout.preferredWidth: 70
                                                        Layout.preferredHeight: 20
                                                        Layout.alignment: Qt.AlignHCenter
                                                        radius: 4
                                                        color: modelData.status === "PASS" ? "#d1fae5" : (modelData.status === "WARN" ? "#fef3c7" : "#fee2e2")
                                                        Text {
                                                            anchors.centerIn: parent
                                                            text: modelData.status
                                                            font.family: Style.fontPrimaryBold
                                                            font.pixelSize: 10
                                                            color: modelData.status === "PASS" ? "#065f46" : (modelData.status === "WARN" ? "#92400e" : "#991b1b")
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }

                            // Tab 4: Detailed Inspection
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                color: Style.panel2
                                border.color: Style.border
                                radius: 8

                                ColumnLayout {
                                    anchors.fill: parent
                                    anchors.margins: 16
                                    spacing: 12

                                    RowLayout {
                                        Layout.fillWidth: true
                                        spacing: 12

                                        Label {
                                            text: "Raw Well Efficacies List"
                                            font.family: Style.fontPrimaryBold
                                            font.pixelSize: Style.fontMd
                                            color: Style.text
                                            Layout.fillWidth: true
                                        }

                                        Label {
                                            text: "Filter Plate:"
                                            font.family: Style.fontPrimaryBold
                                            font.pixelSize: Style.fontSm
                                            color: Style.text
                                        }

                                        ComboBox {
                                            id: inspectPlateSelector
                                            Layout.preferredWidth: 200
                                            font.family: Style.fontSecondary
                                            model: {
                                                var list = App.project.analysisResults.merged_files || [];
                                                var arr = ["All Plates"];
                                                for (var i = 0; i < list.length; ++i) {
                                                    var filename = ("" + list[i]).split("/").pop();
                                                     var barcode = filename.replace("merged_", "").replace(".csv", "");
                                                     arr.push(barcode);
                                                }
                                                return arr;
                                            }
                                            currentIndex: 0
                                        }
                                    }

                                    Frame {
                                        Layout.fillWidth: true
                                        Layout.fillHeight: true
                                        background: Rectangle {
                                            color: Style.panel
                                            border.color: Style.border
                                            radius: 8
                                        }

                                        ListView {
                                            id: detailsListView
                                            anchors.fill: parent
                                            clip: true
                                            model: {
                                                var all = App.project.analysisResults.wells || [];
                                                var filtered = [];
                                                var selPlate = inspectPlateSelector.currentIndex; // 0 = All, 1 = Plate 1, etc.
                                                for (var i = 0; i < all.length; ++i) {
                                                    var w = all[i];
                                                    if (selPlate === 0 || w.plate === selPlate) {
                                                        filtered.push(w);
                                                    }
                                                }
                                                return filtered;
                                            }
                                            header: Rectangle {
                                                width: detailsListView.width
                                                height: 32
                                                color: Style.panel2
                                                border.color: Style.border
                                                RowLayout {
                                                    anchors.fill: parent
                                                    anchors.leftMargin: 8
                                                    anchors.rightMargin: 8
                                                    Text { text: "Well"; font.family: Style.fontPrimaryBold; font.pixelSize: Style.fontXs; color: Style.text; Layout.preferredWidth: 60 }
                                                    Text { text: "Plate"; font.family: Style.fontPrimaryBold; font.pixelSize: Style.fontXs; color: Style.text; Layout.preferredWidth: 50; horizontalAlignment: Text.AlignHCenter }
                                                    Text { text: "Category"; font.family: Style.fontPrimaryBold; font.pixelSize: Style.fontXs; color: Style.text; Layout.preferredWidth: 90; horizontalAlignment: Text.AlignHCenter }
                                                    Text { text: "Compound"; font.family: Style.fontPrimaryBold; font.pixelSize: Style.fontXs; color: Style.text; Layout.fillWidth: true }
                                                    Text { text: "Dose (uM)"; font.family: Style.fontPrimaryBold; font.pixelSize: Style.fontXs; color: Style.text; Layout.preferredWidth: 80; horizontalAlignment: Text.AlignRight }
                                                    Text { text: "Motility"; font.family: Style.fontPrimaryBold; font.pixelSize: Style.fontXs; color: Style.text; Layout.preferredWidth: 80; horizontalAlignment: Text.AlignRight }
                                                    Text { text: "Efficacy"; font.family: Style.fontPrimaryBold; font.pixelSize: Style.fontXs; color: Style.text; Layout.preferredWidth: 80; horizontalAlignment: Text.AlignRight }
                                                }
                                            }
                                            delegate: Rectangle {
                                                width: detailsListView.width
                                                height: 36
                                                color: index % 2 === 0 ? "transparent" : Style.panel2
                                                border.color: Style.border
                                                border.width: 0.5
                                                RowLayout {
                                                    anchors.fill: parent
                                                    anchors.leftMargin: 8
                                                    anchors.rightMargin: 8
                                                    Text { text: modelData.well; font.family: Style.fontSecondary; font.pixelSize: Style.fontXs; color: Style.text; Layout.preferredWidth: 60 }
                                                    Text { text: modelData.plate; font.family: Style.fontSecondary; font.pixelSize: Style.fontXs; color: Style.text; Layout.preferredWidth: 50; horizontalAlignment: Text.AlignHCenter }
                                                    
                                                    Rectangle {
                                                        Layout.preferredWidth: 80
                                                        Layout.preferredHeight: 18
                                                        Layout.alignment: Qt.AlignHCenter
                                                        radius: 4
                                                        color: {
                                                            var t = modelData.type || "";
                                                            if (t === "placebo") return "#e0f2fe";
                                                            if (t === "standard") return "#fee2e2";
                                                            if (t === "qc") return "#fef3c7";
                                                            return "#f3e8ff";
                                                        }
                                                        Text {
                                                            anchors.centerIn: parent
                                                            text: {
                                                                var t = modelData.type || "";
                                                                if (t === "placebo") return "Placebo";
                                                                if (t === "standard") return "Standard";
                                                                if (t === "qc") return "QC Ctrl";
                                                                return "Test";
                                                            }
                                                            font.family: Style.fontPrimaryBold
                                                            font.pixelSize: 9
                                                            color: {
                                                                var t = modelData.type || "";
                                                                if (t === "placebo") return "#0369a1";
                                                                if (t === "standard") return "#b91c1c";
                                                                if (t === "qc") return "#b45309";
                                                                return "#6b21a8";
                                                            }
                                                        }
                                                    }
                                                    
                                                    Text { text: modelData.compound; font.family: Style.fontSecondary; font.pixelSize: Style.fontXs; color: Style.text; Layout.fillWidth: true; elide: Text.ElideRight }
                                                    Text { text: modelData.dose > 0 ? modelData.dose.toFixed(4) : "0.0000"; font.family: Style.fontSecondary; font.pixelSize: Style.fontXs; color: Style.text; Layout.preferredWidth: 80; horizontalAlignment: Text.AlignRight }
                                                    Text { text: modelData.raw.toFixed(2); font.family: Style.fontSecondary; font.pixelSize: Style.fontXs; color: Style.text; Layout.preferredWidth: 80; horizontalAlignment: Text.AlignRight }
                                                    Text { text: modelData.efficacy.toFixed(1) + "%"; font.family: Style.fontPrimaryBold; font.pixelSize: Style.fontXs; color: Style.text; Layout.preferredWidth: 80; horizontalAlignment: Text.AlignRight }
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
    }

    // Graph Styling & Customization Drawer
    Drawer {
        id: graphSettingsDrawer
        width: 320
        height: root.height
        edge: Qt.RightEdge
        
        background: Rectangle {
            color: Style.panel
            border.color: Style.border
            border.width: 1
        }
        
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 16
            spacing: 12
            
            RowLayout {
                Layout.fillWidth: true
                Text {
                    text: "Graph Customization"
                    font.family: Style.fontPrimaryBold
                    font.pixelSize: Style.fontMd
                    color: Style.text
                    Layout.fillWidth: true
                }
                Button {
                    text: "Close"
                    onClicked: graphSettingsDrawer.close()
                }
            }
            
            Rectangle { height: 1; Layout.fillWidth: true; color: Style.border }
            
            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                
                ColumnLayout {
                    width: 288 // 320 drawer width - 32 padding
                    spacing: 12
                    
                    // --- Background Colors ---
                    Text { text: "Background Theme:"; font.family: Style.fontPrimaryBold; font.pixelSize: Style.fontXs; color: Style.text }
                    ComboBox {
                        Layout.fillWidth: true
                        model: ["Default Theme", "Charcoal Dark", "Pitch Black", "Light Slate", "Pure White", "Transparent"]
                        currentIndex: 0
                        onCurrentIndexChanged: {
                            if (currentIndex === 0) {
                                root.graphBgColor = Style.panel2;
                                root.graphPlotAreaColor = "transparent";
                            } else if (currentIndex === 1) {
                                root.graphBgColor = "#1e293b";
                                root.graphPlotAreaColor = "#0f172a";
                            } else if (currentIndex === 2) {
                                root.graphBgColor = "#000000";
                                root.graphPlotAreaColor = "#0a0a0a";
                            } else if (currentIndex === 3) {
                                root.graphBgColor = "#f1f5f9";
                                root.graphPlotAreaColor = "#ffffff";
                            } else if (currentIndex === 4) {
                                root.graphBgColor = "#ffffff";
                                root.graphPlotAreaColor = "#ffffff";
                            } else if (currentIndex === 5) {
                                root.graphBgColor = "transparent";
                                root.graphPlotAreaColor = "transparent";
                            }
                        }
                    }
                    
                    // --- Sizes ---
                    Text { text: "Sizes:"; font.family: Style.fontPrimaryBold; font.pixelSize: Style.fontXs; color: Style.text }
                    
                    RowLayout {
                        Layout.fillWidth: true
                        Text { text: "Curve Width:"; font.family: Style.fontSecondary; font.pixelSize: Style.fontXs; color: Style.subText; Layout.fillWidth: true }
                        TextField {
                            id: curveWidthInput
                            width: 50
                            Layout.preferredWidth: 50
                            text: root.curveWidth.toFixed(1)
                            validator: DoubleValidator { bottom: 1.0; top: 8.0; decimals: 1 }
                            onEditingFinished: {
                                var val = parseFloat(text);
                                if (!isNaN(val)) root.curveWidth = Math.max(1.0, Math.min(8.0, val));
                            }
                        }
                    }
                    Slider {
                        Layout.fillWidth: true
                        from: 1.0
                        to: 8.0
                        value: root.curveWidth
                        onValueChanged: {
                            root.curveWidth = value;
                            curveWidthInput.text = value.toFixed(1);
                        }
                    }
                    
                    RowLayout {
                        Layout.fillWidth: true
                        Text { text: "Point Size:"; font.family: Style.fontSecondary; font.pixelSize: Style.fontXs; color: Style.subText; Layout.fillWidth: true }
                        TextField {
                            id: pointSizeInput
                            width: 50
                            Layout.preferredWidth: 50
                            text: root.pointSize.toFixed(0)
                            validator: IntValidator { bottom: 4; top: 20 }
                            onEditingFinished: {
                                var val = parseInt(text);
                                if (!isNaN(val)) root.pointSize = Math.max(4.0, Math.min(20.0, val));
                            }
                        }
                    }
                    Slider {
                        Layout.fillWidth: true
                        from: 4.0
                        to: 20.0
                        value: root.pointSize
                        onValueChanged: {
                            root.pointSize = value;
                            pointSizeInput.text = value.toFixed(0);
                        }
                    }
                    
                    RowLayout {
                        Layout.fillWidth: true
                        Text { text: "Controls Line Width:"; font.family: Style.fontSecondary; font.pixelSize: Style.fontXs; color: Style.subText; Layout.fillWidth: true }
                        TextField {
                            id: controlCurveWidthInput
                            width: 50
                            Layout.preferredWidth: 50
                            text: root.controlCurveWidth.toFixed(1)
                            validator: DoubleValidator { bottom: 1.0; top: 8.0; decimals: 1 }
                            onEditingFinished: {
                                var val = parseFloat(text);
                                if (!isNaN(val)) root.controlCurveWidth = Math.max(1.0, Math.min(8.0, val));
                            }
                        }
                    }
                    Slider {
                        Layout.fillWidth: true
                        from: 1.0
                        to: 8.0
                        value: root.controlCurveWidth
                        onValueChanged: {
                            root.controlCurveWidth = value;
                            controlCurveWidthInput.text = value.toFixed(1);
                        }
                    }
                    
                    RowLayout {
                        Layout.fillWidth: true
                        Text { text: "Controls Point Size:"; font.family: Style.fontSecondary; font.pixelSize: Style.fontXs; color: Style.subText; Layout.fillWidth: true }
                        TextField {
                            id: controlPointSizeInput
                            width: 50
                            Layout.preferredWidth: 50
                            text: root.controlPointSize.toFixed(0)
                            validator: IntValidator { bottom: 4; top: 20 }
                            onEditingFinished: {
                                var val = parseInt(text);
                                if (!isNaN(val)) root.controlPointSize = Math.max(4.0, Math.min(20.0, val));
                            }
                        }
                    }
                    Slider {
                        Layout.fillWidth: true
                        from: 4.0
                        to: 20.0
                        value: root.controlPointSize
                        onValueChanged: {
                            root.controlPointSize = value;
                            controlPointSizeInput.text = value.toFixed(0);
                        }
                    }
                    
                    // --- Axis Limits ---
                    Text { text: "X-Axis Range (Log10 uM):"; font.family: Style.fontPrimaryBold; font.pixelSize: Style.fontXs; color: Style.text }
                    RowLayout {
                        spacing: 8
                        TextField {
                            Layout.fillWidth: true
                            placeholderText: "Min"
                            text: root.axisXMinVal.toString()
                            onTextEdited: {
                                var v = parseFloat(text);
                                if (!isNaN(v)) root.axisXMinVal = v;
                            }
                        }
                        TextField {
                            Layout.fillWidth: true
                            placeholderText: "Max"
                            text: root.axisXMaxVal.toString()
                            onTextEdited: {
                                var v = parseFloat(text);
                                if (!isNaN(v)) root.axisXMaxVal = v;
                            }
                        }
                    }
                    
                    Text { text: "Y-Axis Range (% Efficacy):"; font.family: Style.fontPrimaryBold; font.pixelSize: Style.fontXs; color: Style.text }
                    RowLayout {
                        spacing: 8
                        TextField {
                            Layout.fillWidth: true
                            placeholderText: "Min"
                            text: root.axisYMinVal.toString()
                            onTextEdited: {
                                var v = parseFloat(text);
                                if (!isNaN(v)) root.axisYMinVal = v;
                            }
                        }
                        TextField {
                            Layout.fillWidth: true
                            placeholderText: "Max"
                            text: root.axisYMaxVal.toString()
                            onTextEdited: {
                                var v = parseFloat(text);
                                if (!isNaN(v)) root.axisYMaxVal = v;
                            }
                        }
                    }
                    
                    // --- Colors ---
                    Text { text: "Series Colors:"; font.family: Style.fontPrimaryBold; font.pixelSize: Style.fontXs; color: Style.text }
                    
                    Text { text: "Active Compound:"; font.family: Style.fontSecondary; font.pixelSize: Style.fontXs; color: Style.subText }
                    RowLayout {
                        spacing: 8
                        Layout.fillWidth: true
                        
                        Rectangle {
                            width: 24
                            height: 24
                            radius: 4
                            color: root.testColor
                            border.color: Style.border
                            border.width: 1
                            
                            MouseArea {
                                anchors.fill: parent
                                onClicked: activeCompoundColorDialog.open()
                            }
                        }
                        
                        TextField {
                            id: testColorInput
                            Layout.fillWidth: true
                            text: root.colorToHex(root.testColor)
                            placeholderText: "#HEX"
                            font.family: Style.fontSecondary
                            font.pixelSize: Style.fontXs
                            
                            onTextEdited: {
                                var hex = text.trim();
                                if (!hex.startsWith("#")) {
                                    hex = "#" + hex;
                                }
                                var hexPattern = /^#[0-9A-Fa-f]{6}$/
                                if (hexPattern.test(hex)) {
                                    root.testColor = hex
                                }
                            }
                            
                            Binding {
                                target: testColorInput
                                property: "text"
                                value: root.colorToHex(root.testColor)
                                when: !testColorInput.activeFocus
                            }
                        }
                        
                        Button {
                            text: "Pick..."
                            onClicked: activeCompoundColorDialog.open()
                        }
                    }
                    
                    Text { text: "Standard Control:"; font.family: Style.fontSecondary; font.pixelSize: Style.fontXs; color: Style.subText }
                    RowLayout {
                        spacing: 8
                        Layout.fillWidth: true
                        
                        Rectangle {
                            width: 24
                            height: 24
                            radius: 4
                            color: root.standardColor
                            border.color: Style.border
                            border.width: 1
                            
                            MouseArea {
                                anchors.fill: parent
                                onClicked: standardColorDialog.open()
                            }
                        }
                        
                        TextField {
                            id: standardColorInput
                            Layout.fillWidth: true
                            text: root.colorToHex(root.standardColor)
                            placeholderText: "#HEX"
                            font.family: Style.fontSecondary
                            font.pixelSize: Style.fontXs
                            
                            onTextEdited: {
                                var hex = text.trim();
                                if (!hex.startsWith("#")) {
                                    hex = "#" + hex;
                                }
                                var hexPattern = /^#[0-9A-Fa-f]{6}$/
                                if (hexPattern.test(hex)) {
                                    root.standardColor = hex
                                }
                            }
                            
                            Binding {
                                target: standardColorInput
                                property: "text"
                                value: root.colorToHex(root.standardColor)
                                when: !standardColorInput.activeFocus
                            }
                        }
                        
                        Button {
                            text: "Pick..."
                            onClicked: standardColorDialog.open()
                        }
                    }
                    
                    Text { text: "QC Control:"; font.family: Style.fontSecondary; font.pixelSize: Style.fontXs; color: Style.subText }
                    RowLayout {
                        spacing: 8
                        Layout.fillWidth: true
                        
                        Rectangle {
                            width: 24
                            height: 24
                            radius: 4
                            color: root.qcColor
                            border.color: Style.border
                            border.width: 1
                            
                            MouseArea {
                                anchors.fill: parent
                                onClicked: qcColorDialog.open()
                            }
                        }
                        
                        TextField {
                            id: qcColorInput
                            Layout.fillWidth: true
                            text: root.colorToHex(root.qcColor)
                            placeholderText: "#HEX"
                            font.family: Style.fontSecondary
                            font.pixelSize: Style.fontXs
                            
                            onTextEdited: {
                                var hex = text.trim();
                                if (!hex.startsWith("#")) {
                                    hex = "#" + hex;
                                }
                                var hexPattern = /^#[0-9A-Fa-f]{6}$/
                                if (hexPattern.test(hex)) {
                                    root.qcColor = hex
                                }
                            }
                            
                            Binding {
                                target: qcColorInput
                                property: "text"
                                value: root.colorToHex(root.qcColor)
                                when: !qcColorInput.activeFocus
                            }
                        }
                        
                        Button {
                            text: "Pick..."
                            onClicked: qcColorDialog.open()
                        }
                    }
                }
            }
        }
    }

    // Asynchronous Export Busy Overlay
    Rectangle {
        id: exportOverlay
        anchors.fill: parent
        color: Qt.rgba(0, 0, 0, 0.6)
        visible: false
        z: 9999
        
        MouseArea {
            anchors.fill: parent
            preventStealing: true
            hoverEnabled: true
        }
        
        ColumnLayout {
            anchors.centerIn: parent
            spacing: 16
            
            BusyIndicator {
                Layout.alignment: Qt.AlignHCenter
                running: exportOverlay.visible
            }
            
            Text {
                text: "Exporting analysis data and compound graphs..."
                font.family: Style.fontPrimaryBold
                font.pixelSize: Style.fontMd
                color: "#ffffff"
                Layout.alignment: Qt.AlignHCenter
            }
        }
    }
}
