import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtQuick.Window
import TestRequests 1.0
import "../"

Window {
    id: root
    title: "INV-T-009 Flea Feeding Analysis"
    width: 1280
    height: 800
    minimumWidth: 1000
    minimumHeight: 700
    visible: false
    modality: Qt.ApplicationModal
    color: Style.bg

    property var layoutUrls: []
    property bool isBusy: false
    property string errorMessage: ""
    property real frameTimeSec: 0.5
    property int activePlateIdx: -1
    property string selectedHandle: "a1"

    // Zoom & Pan properties
    property real zoomScale: 1.0
    property real panX: 0.0
    property real panY: 0.0

    // Settings Sync properties
    property real activeRadius: 40.0
    property real activeInnerRatio: 0.8
    property int activeFixedThresh: 100

    signal analysisApplied(var rawPaths)

    function resetZoomPan() {
        zoomScale = 1.0;
        panX = 0.0;
        panY = 0.0;
    }

    onActivePlateIdxChanged: {
        resetZoomPan();
        if (activePlateIdx >= 0 && activePlateIdx < platesModel.count) {
            var p = platesModel.get(activePlateIdx);
            activeRadius = p.radius;
            activeInnerRatio = p.innerRatio;
            activeFixedThresh = p.fixedThresh;
        } else {
            activeRadius = 40.0;
            activeInnerRatio = 0.8;
            activeFixedThresh = 100;
        }
        overlayCanvas.requestPaint();
    }

    onActiveRadiusChanged: {
        if (activePlateIdx >= 0 && activePlateIdx < platesModel.count) {
            var p = platesModel.get(activePlateIdx);
            if (p.radius !== activeRadius) {
                platesModel.setProperty(activePlateIdx, "radius", activeRadius);
                overlayCanvas.requestPaint();
            }
        }
    }

    onActiveInnerRatioChanged: {
        if (activePlateIdx >= 0 && activePlateIdx < platesModel.count) {
            var p = platesModel.get(activePlateIdx);
            if (p.innerRatio !== activeInnerRatio) {
                platesModel.setProperty(activePlateIdx, "innerRatio", activeInnerRatio);
                overlayCanvas.requestPaint();
            }
        }
    }

    onActiveFixedThreshChanged: {
        if (activePlateIdx >= 0 && activePlateIdx < platesModel.count) {
            var p = platesModel.get(activePlateIdx);
            if (p.fixedThresh !== activeFixedThresh) {
                platesModel.setProperty(activePlateIdx, "fixedThresh", activeFixedThresh);
                overlayCanvas.requestPaint();
            }
        }
    }

    onVisibleChanged: {
        if (visible) {
            initializePlates();
            resetZoomPan();
        }
    }

    // Helper to format QML color mapping (from HTML)
    function getFeedingHeatColor(pct, alpha) {
        if (pct === undefined || isNaN(pct)) return "rgba(241, 245, 249, " + alpha + ")";
        var stops = [
            [0,   [250, 238, 218]],
            [25,  [192, 221, 151]],
            [50,  [151, 196, 89]],
            [75,  [99,  153, 34]],
            [100, [39,  80,  10]]
        ];
        var x = Math.max(0, Math.min(100, pct));
        var i = 0; 
        while (i < stops.length - 1 && stops[i+1][0] < x) i++;
        var s0 = stops[i]; 
        var s1 = stops[Math.min(i+1, stops.length-1)];
        var x0 = s0[0];
        var c0 = s0[1];
        var x1 = s1[0];
        var c1 = s1[1];
        var t = x1 === x0 ? 0 : (x - x0) / (x1 - x0);
        var r = Math.round(c0[0] + (c1[0]-c0[0])*t);
        var g = Math.round(c0[1] + (c1[1]-c0[1])*t);
        var b = Math.round(c0[2] + (c1[2]-c0[2])*t);
        return "rgba(" + r + "," + g + "," + b + "," + alpha + ")";
    }

    // Convert QML file URL to absolute local path
    function urlToLocalPath(url) {
        var s = url.toString();
        if (s.startsWith("file:///")) {
            s = s.substring(8);
        } else if (s.startsWith("file://")) {
            s = s.substring(7);
        }
        return decodeURIComponent(s);
    }

    ListModel {
        id: platesModel
    }

    function initializePlates() {
        platesModel.clear();
        for (var i = 0; i < layoutUrls.length; ++i) {
            var url = layoutUrls[i].toString();
            var parts = url.split("/");
            var filename = parts[parts.length - 1];
            var baseName = filename.split(".")[0];
            if (baseName !== "") {
                platesModel.append({
                    barcode: baseName,
                    videoPath: "",
                    framePath: "",
                    a1X: -1.0,
                    a1Y: -1.0,
                    h12X: -1.0,
                    h12Y: -1.0,
                    radius: 40.0,
                    innerRatio: 0.8,
                    threshMode: "otsu-well",
                    fixedThresh: 100,
                    polarity: "dark",
                    isAnalyzed: false,
                    wellDensitiesJson: "",
                    calibrationStep: "a1"
                });
            }
        }
        if (platesModel.count > 0) {
            activePlateIdx = 0;
        } else {
            activePlateIdx = -1;
        }
        overlayCanvas.requestPaint();
    }

    function extractActiveFrame() {
        if (activePlateIdx < 0 || activePlateIdx >= platesModel.count) return;
        var pData = platesModel.get(activePlateIdx);
        if (pData.videoPath === "") return;
        
        root.isBusy = true;
        
        // Extract frame in C++
        var framePath = App.feedingProcessor.extractFrameFromVideo(pData.videoPath, root.frameTimeSec);
        if (framePath !== "") {
            platesModel.setProperty(activePlateIdx, "framePath", framePath);
            platesModel.setProperty(activePlateIdx, "isAnalyzed", false);
            platesModel.setProperty(activePlateIdx, "wellDensitiesJson", "");
            // Reset calibration
            platesModel.setProperty(activePlateIdx, "a1X", -1.0);
            platesModel.setProperty(activePlateIdx, "a1Y", -1.0);
            platesModel.setProperty(activePlateIdx, "h12X", -1.0);
            platesModel.setProperty(activePlateIdx, "h12Y", -1.0);
            platesModel.setProperty(activePlateIdx, "calibrationStep", "a1");
            root.errorMessage = "";
        } else {
            root.errorMessage = "Failed to extract frame from video file. Verify video encoding.";
        }
        root.isBusy = false;
        overlayCanvas.requestPaint();
    }

    function computeFeedingDensity() {
        if (activePlateIdx < 0 || activePlateIdx >= platesModel.count) return;
        var pData = platesModel.get(activePlateIdx);
        if (pData.framePath === "") {
            root.errorMessage = "Please upload a video file first.";
            return;
        }
        if (pData.calibrationStep !== "done" || pData.h12X < 0) {
            root.errorMessage = "Please complete the A1 / H12 grid calibration first.";
            return;
        }
        
        root.isBusy = true;
        
        var wellDensities = App.feedingProcessor.processPlateImage(
            pData.framePath,
            Qt.point(pData.a1X, pData.a1Y),
            Qt.point(pData.h12X, pData.h12Y),
            pData.radius,
            pData.innerRatio,
            pData.threshMode,
            pData.fixedThresh,
            pData.polarity
        );
        
        if (wellDensities && Object.keys(wellDensities).length > 0) {
            platesModel.setProperty(activePlateIdx, "wellDensitiesJson", JSON.stringify(wellDensities));
            platesModel.setProperty(activePlateIdx, "isAnalyzed", true);
            root.errorMessage = "";
        } else {
            root.errorMessage = "Failed to compute feeding density. Verify calibration points.";
        }
        root.isBusy = false;
        overlayCanvas.requestPaint();
    }

    function confirmAndApply() {
        var rawCsvPaths = [];
        for (var i = 0; i < platesModel.count; ++i) {
            var pData = platesModel.get(i);
            if (!pData.isAnalyzed) {
                root.errorMessage = "Please analyze all plates before confirming.";
                return;
            }
        }
        
        for (var idx = 0; idx < platesModel.count; ++idx) {
            var p = platesModel.get(idx);
            var densities = JSON.parse(p.wellDensitiesJson);
            var csvPath = App.feedingProcessor.writeTemporaryRawCsv(p.barcode, densities);
            if (csvPath !== "") {
                rawCsvPaths.push("file:///" + csvPath);
            } else {
                root.errorMessage = "Failed to write raw CSV for plate " + p.barcode;
                return;
            }
        }
        
        root.analysisApplied(rawCsvPaths);
        root.close();
    }

    function undoLastCalibrationStep() {
        if (activePlateIdx < 0 || activePlateIdx >= platesModel.count) return;
        var pData = platesModel.get(activePlateIdx);
        var step = pData.calibrationStep;
        if (step === "done") {
            platesModel.setProperty(activePlateIdx, "h12X", -1.0);
            platesModel.setProperty(activePlateIdx, "h12Y", -1.0);
            platesModel.setProperty(activePlateIdx, "calibrationStep", "h12");
            platesModel.setProperty(activePlateIdx, "isAnalyzed", false);
            platesModel.setProperty(activePlateIdx, "wellDensitiesJson", "");
            root.selectedHandle = "h12";
        } else if (step === "h12") {
            platesModel.setProperty(activePlateIdx, "a1X", -1.0);
            platesModel.setProperty(activePlateIdx, "a1Y", -1.0);
            platesModel.setProperty(activePlateIdx, "calibrationStep", "a1");
            root.selectedHandle = "a1";
        }
        overlayCanvas.requestPaint();
    }

    function nudgeSelectedHandle(dx, dy) {
        if (activePlateIdx < 0 || activePlateIdx >= platesModel.count) return;
        var p = platesModel.get(activePlateIdx);
        if (selectedHandle === "a1" && p.a1X >= 0) {
            var newX = Math.max(0, Math.min(frameImage.sourceSize.width, p.a1X + dx));
            var newY = Math.max(0, Math.min(frameImage.sourceSize.height, p.a1Y + dy));
            platesModel.setProperty(activePlateIdx, "a1X", newX);
            platesModel.setProperty(activePlateIdx, "a1Y", newY);
        } else if (selectedHandle === "h12" && p.h12X >= 0) {
            var newX = Math.max(0, Math.min(frameImage.sourceSize.width, p.h12X + dx));
            var newY = Math.max(0, Math.min(frameImage.sourceSize.height, p.h12Y + dy));
            platesModel.setProperty(activePlateIdx, "h12X", newX);
            platesModel.setProperty(activePlateIdx, "h12Y", newY);
        }
        overlayCanvas.requestPaint();
    }

    Shortcut {
        sequence: "Ctrl+Z"
        context: Qt.WindowShortcut
        onActivated: undoLastCalibrationStep()
    }

    Shortcut {
        sequence: "Left"
        context: Qt.WindowShortcut
        onActivated: nudgeSelectedHandle(-0.5, 0)
    }
    Shortcut {
        sequence: "Right"
        context: Qt.WindowShortcut
        onActivated: nudgeSelectedHandle(0.5, 0)
    }
    Shortcut {
        sequence: "Up"
        context: Qt.WindowShortcut
        onActivated: nudgeSelectedHandle(0, -0.5)
    }
    Shortcut {
        sequence: "Down"
        context: Qt.WindowShortcut
        onActivated: nudgeSelectedHandle(0, 0.5)
    }

    Shortcut {
        sequence: "Shift+Left"
        context: Qt.WindowShortcut
        onActivated: nudgeSelectedHandle(-3, 0)
    }
    Shortcut {
        sequence: "Shift+Right"
        context: Qt.WindowShortcut
        onActivated: nudgeSelectedHandle(3, 0)
    }
    Shortcut {
        sequence: "Shift+Up"
        context: Qt.WindowShortcut
        onActivated: nudgeSelectedHandle(0, -3)
    }
    Shortcut {
        sequence: "Shift+Down"
        context: Qt.WindowShortcut
        onActivated: nudgeSelectedHandle(0, 3)
    }

    function getScaleAndOffset() {
        if (frameImage.status !== Image.Ready) return { scale: 1.0, xOffset: 0.0, yOffset: 0.0 };
        
        var wScale = frameImage.width / frameImage.sourceSize.width;
        var hScale = frameImage.height / frameImage.sourceSize.height;
        var scale = Math.min(wScale, hScale);
        
        var displayWidth = frameImage.sourceSize.width * scale;
        var displayHeight = frameImage.sourceSize.height * scale;
        
        var xOffset = (frameImage.width - displayWidth) / 2;
        var yOffset = (frameImage.height - displayHeight) / 2;
        
        return { scale: scale, xOffset: xOffset, yOffset: yOffset };
    }

    FileDialog {
        id: videoFileDialog
        title: "Select MP4 Video for Plate " + (activePlateIdx >= 0 ? platesModel.get(activePlateIdx).barcode : "")
        nameFilters: ["MP4 Video files (*.mp4)"]
        onAccepted: {
            if (activePlateIdx >= 0) {
                var localPath = root.urlToLocalPath(selectedFile);
                platesModel.setProperty(activePlateIdx, "videoPath", localPath);
                extractActiveFrame();
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        // Header Title
        RowLayout {
            Layout.fillWidth: true
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2
                Text {
                    text: "Flea Feeding Analysis Pipeline"
                    font.family: Style.fontPrimaryBold
                    font.pixelSize: Style.fontLg
                    color: Style.text
                }
                Text {
                    text: "Load plate videos, calibrate the A1/H12 well boundaries, and analyze dark pixel density."
                    font.family: Style.fontSecondary
                    font.pixelSize: Style.fontXs
                    color: Style.subText
                }
            }
            Button {
                text: "Reset All"
                onClicked: initializePlates()
            }
        }

        Rectangle { height: 1; Layout.fillWidth: true; color: Style.border }

        // Error Banner
        Rectangle {
            Layout.fillWidth: true
            height: 36
            color: "#fef2f2"
            border.color: "#ef4444"
            radius: 6
            visible: root.errorMessage !== ""
            RowLayout {
                anchors.fill: parent
                anchors.margins: 8
                Text {
                    text: root.errorMessage
                    font.family: Style.fontSecondary
                    font.pixelSize: Style.fontXs
                    color: "#991b1b"
                    Layout.fillWidth: true
                }
            }
        }

        // Main Layout
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 16

            // 1. Sidebar: Plate List
            Rectangle {
                z: 1
                Layout.preferredWidth: 260
                Layout.fillHeight: true
                color: Style.panel
                border.color: Style.border
                radius: 8

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 8

                    Text {
                        text: "Plates list:"
                        font.family: Style.fontPrimaryBold
                        font.pixelSize: Style.fontXs
                        color: Style.text
                        Layout.margins: 4
                    }

                    ListView {
                        id: platesListView
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        model: platesModel
                        clip: true
                        spacing: 6
                        delegate: Rectangle {
                            width: platesListView.width
                            height: 64
                            radius: 6
                            color: activePlateIdx === index ? Style.panel2 : "transparent"
                            border.color: activePlateIdx === index ? Style.accent : Style.border
                            border.width: activePlateIdx === index ? 1.5 : 1

                            MouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    activePlateIdx = index;
                                    overlayCanvas.requestPaint();
                                }
                            }

                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 8
                                spacing: 8

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 2
                                    Text {
                                        text: model.barcode
                                        font.family: Style.fontPrimaryBold
                                        font.pixelSize: Style.fontXs
                                        color: Style.text
                                    }
                                    Text {
                                        text: model.videoPath !== "" ? model.videoPath.split("/").pop() : "No video loaded"
                                        font.family: Style.fontSecondary
                                        font.pixelSize: Style.fontXs - 1
                                        color: Style.subText
                                        elide: Text.ElideMiddle
                                        Layout.fillWidth: true
                                    }
                                }

                                // Status Badge
                                Rectangle {
                                    width: 80
                                    height: 22
                                    radius: 4
                                    color: model.isAnalyzed ? "#dcfce7" : (model.videoPath !== "" ? "#fef3c7" : "#fee2e2")
                                    Text {
                                        anchors.centerIn: parent
                                        text: model.isAnalyzed ? "Analyzed" : (model.videoPath !== "" ? "Calibrating" : "No Video")
                                        font.family: Style.fontSecondary
                                        font.pixelSize: Style.fontXs - 2
                                        font.bold: true
                                        color: model.isAnalyzed ? "#15803d" : (model.videoPath !== "" ? "#b45309" : "#b91c1c")
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // 2. Central Frame Viewer & Calibration Grid
            Rectangle {
                id: centralViewer
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#0f172a"
                border.color: Style.border
                radius: 8
                clip: true

                // Zoom and Pan Handlers on the parent viewport container
                MouseArea {
                    id: zoomPanMouseArea
                    anchors.fill: parent
                    acceptedButtons: Qt.RightButton
                    cursorShape: pressed ? Qt.ClosedHandCursor : Qt.OpenHandCursor
                    
                    property real lastX: 0
                    property real lastY: 0
                    
                    onPressed: function(mouse) {
                        lastX = mouse.x;
                        lastY = mouse.y;
                    }
                    
                    onPositionChanged: function(mouse) {
                        if (pressed) {
                            var dx = mouse.x - lastX;
                            var dy = mouse.y - lastY;
                            root.panX += dx;
                            root.panY += dy;
                            lastX = mouse.x;
                            lastY = mouse.y;
                        }
                    }
                    
                    onDoubleClicked: function(mouse) {
                        resetZoomPan();
                    }
                }

                WheelHandler {
                    id: wheelHandler
                    target: null
                    onWheel: function(event) {
                        var zoomFactor = 1.15;
                        if (event.angleDelta.y < 0) {
                            zoomFactor = 1.0 / zoomFactor;
                        }
                        
                        var oldScale = root.zoomScale;
                        var newScale = Math.max(0.5, Math.min(10.0, oldScale * zoomFactor));
                        
                        // Zoom to mouse cursor position
                        var mouseX = wheelHandler.point.position.x;
                        var mouseY = wheelHandler.point.position.y;
                        var dx = mouseX - centralViewer.width / 2;
                        var dy = mouseY - centralViewer.height / 2;
                        
                        root.panX = dx - (dx - root.panX) * (newScale / oldScale);
                        root.panY = dy - (dy - root.panY) * (newScale / oldScale);
                        root.zoomScale = newScale;
                    }
                }

                PinchArea {
                    id: pinchArea
                    anchors.fill: parent
                    property real initialScale: 1.0
                    onPinchStarted: {
                        initialScale = root.zoomScale;
                    }
                    onPinchUpdated: function(pinch) {
                        var oldScale = root.zoomScale;
                        var newScale = Math.max(0.5, Math.min(10.0, initialScale * pinch.scale));
                        
                        // Zoom to pinch center
                        var px = pinch.center.x;
                        var py = pinch.center.y;
                        var dx = px - centralViewer.width / 2;
                        var dy = py - centralViewer.height / 2;
                        
                        root.panX = dx - (dx - root.panX) * (newScale / oldScale);
                        root.panY = dy - (dy - root.panY) * (newScale / oldScale);
                        root.zoomScale = newScale;
                    }
                }

                Image {
                    id: frameImage
                    anchors.fill: parent
                    anchors.margins: 16
                    fillMode: Image.PreserveAspectFit
                    clip: true
                    source: (activePlateIdx >= 0 && activePlateIdx < platesModel.count && platesModel.get(activePlateIdx).framePath !== "") 
                            ? "file:///" + platesModel.get(activePlateIdx).framePath 
                            : ""

                    onStatusChanged: {
                        if (status === Image.Ready) {
                            overlayCanvas.requestPaint();
                        }
                    }

                    transform: [
                        Scale {
                            origin.x: frameImage.width / 2
                            origin.y: frameImage.height / 2
                            xScale: root.zoomScale
                            yScale: root.zoomScale
                        },
                        Translate {
                            x: root.panX
                            y: root.panY
                        }
                    ]

                    // Click coordinates mapper
                    MouseArea {
                        anchors.fill: parent
                        onClicked: function(mouse) {
                            if (activePlateIdx < 0 || activePlateIdx >= platesModel.count) return;
                            var pData = platesModel.get(activePlateIdx);
                            if (pData.framePath === "") return;
                            
                            var geom = getScaleAndOffset();
                            var relativeX = mouse.x - geom.xOffset;
                            var relativeY = mouse.y - geom.yOffset;
                            
                            if (relativeX < 0 || relativeX > (frameImage.sourceSize.width * geom.scale) ||
                                relativeY < 0 || relativeY > (frameImage.sourceSize.height * geom.scale)) {
                                return; // out of image bounds
                            }
                            
                            var imageX = relativeX / geom.scale;
                            var imageY = relativeY / geom.scale;
                            
                            var step = pData.calibrationStep;
                            if (step === "a1") {
                                platesModel.setProperty(activePlateIdx, "a1X", imageX);
                                platesModel.setProperty(activePlateIdx, "a1Y", imageY);
                                platesModel.setProperty(activePlateIdx, "calibrationStep", "h12");
                                root.selectedHandle = "h12";
                            } else if (step === "h12") {
                                platesModel.setProperty(activePlateIdx, "h12X", imageX);
                                platesModel.setProperty(activePlateIdx, "h12Y", imageY);
                                platesModel.setProperty(activePlateIdx, "calibrationStep", "done");
                                root.selectedHandle = "h12";
                            } else {
                                platesModel.setProperty(activePlateIdx, "a1X", -1.0);
                                platesModel.setProperty(activePlateIdx, "a1Y", -1.0);
                                platesModel.setProperty(activePlateIdx, "h12X", -1.0);
                                platesModel.setProperty(activePlateIdx, "h12Y", -1.0);
                                platesModel.setProperty(activePlateIdx, "calibrationStep", "a1");
                                root.selectedHandle = "a1";
                            }
                            overlayCanvas.requestPaint();
                        }
                    }

                    Canvas {
                        id: overlayCanvas
                        anchors.fill: parent
                        clip: true

                        Connections {
                            target: frameImage
                            function onWidthChanged() { overlayCanvas.requestPaint(); }
                            function onHeightChanged() { overlayCanvas.requestPaint(); }
                        }

                        onPaint: {
                            var ctx = getContext("2d");
                            ctx.clearRect(0, 0, width, height);

                            if (activePlateIdx < 0 || activePlateIdx >= platesModel.count) return;
                            var pData = platesModel.get(activePlateIdx);
                            if (pData.framePath === "") return;

                            var geom = getScaleAndOffset();

                            // 1. Draw A1 Circle
                            if (pData.a1X >= 0) {
                                var a1X_d = pData.a1X * geom.scale + geom.xOffset;
                                var a1Y_d = pData.a1Y * geom.scale + geom.yOffset;

                                ctx.strokeStyle = (root.selectedHandle === "a1") ? '#f43f5e' : '#38bdf8'; 
                                ctx.lineWidth = (root.selectedHandle === "a1") ? 3.5 : 2.5;
                                ctx.beginPath(); 
                                ctx.arc(a1X_d, a1Y_d, 12, 0, Math.PI * 2); 
                                ctx.stroke();

                                ctx.fillStyle = (root.selectedHandle === "a1") ? '#e11d48' : '#0ea5e9'; 
                                ctx.beginPath(); 
                                ctx.arc(a1X_d, a1Y_d, 4, 0, Math.PI * 2); 
                                ctx.fill();

                                ctx.fillStyle = '#ffffff'; 
                                ctx.font = 'bold 11px sans-serif'; 
                                ctx.fillText('A1', a1X_d + 16, a1Y_d - 8);
                            }

                            // 2. Draw H12 Circle & Well Grid
                            if (pData.h12X >= 0) {
                                var h12X_d = pData.h12X * geom.scale + geom.xOffset;
                                var h12Y_d = pData.h12Y * geom.scale + geom.yOffset;

                                ctx.strokeStyle = (root.selectedHandle === "h12") ? '#f43f5e' : '#38bdf8'; 
                                ctx.lineWidth = (root.selectedHandle === "h12") ? 3.5 : 2.5;
                                ctx.beginPath(); 
                                ctx.arc(h12X_d, h12Y_d, 12, 0, Math.PI * 2); 
                                ctx.stroke();

                                ctx.fillStyle = (root.selectedHandle === "h12") ? '#e11d48' : '#0ea5e9'; 
                                ctx.beginPath(); 
                                ctx.arc(h12X_d, h12Y_d, 4, 0, Math.PI * 2); 
                                ctx.fill();

                                ctx.fillStyle = '#ffffff'; 
                                ctx.font = 'bold 11px sans-serif'; 
                                ctx.fillText('H12', h12X_d + 16, h12Y_d - 8);

                                // Dashed line connecting A1 and H12
                                var a1X_d = pData.a1X * geom.scale + geom.xOffset;
                                var a1Y_d = pData.a1Y * geom.scale + geom.yOffset;
                                ctx.strokeStyle = 'rgba(14, 165, 233, 0.5)';
                                ctx.lineWidth = 1.5;
                                ctx.setLineDash([4, 4]);
                                ctx.beginPath();
                                ctx.moveTo(a1X_d, a1Y_d);
                                ctx.lineTo(h12X_d, h12Y_d);
                                ctx.stroke();
                                ctx.setLineDash([]);

                                // Draw 96 well overlay
                                var rows = 8;
                                var cols = 12;
                                var dx = (pData.h12X - pData.a1X) / 11.0;
                                var dy = (pData.h12Y - pData.a1Y) / 7.0;
                                var drawRadius = pData.radius * geom.scale;
                                var drawInnerRadius = drawRadius * pData.innerRatio;

                                var densities = null;
                                if (pData.isAnalyzed && pData.wellDensitiesJson !== "") {
                                    densities = JSON.parse(pData.wellDensitiesJson);
                                }

                                var rowLabels = 'ABCDEFGH';
                                ctx.lineWidth = 1.0;

                                for (var r = 0; r < rows; ++r) {
                                    for (var c = 0; c < cols; ++c) {
                                        var wx = pData.a1X + dx * c;
                                        var wy = pData.a1Y + dy * r;
                                        var wx_d = wx * geom.scale + geom.xOffset;
                                        var wy_d = wy * geom.scale + geom.yOffset;

                                        var wellKey = rowLabels.charAt(r) + (c + 1 < 10 ? "0" + (c + 1) : (c + 1));

                                        // If analyzed, fill the circle with heat color
                                        if (densities !== null && densities[wellKey] !== undefined) {
                                            var dens = densities[wellKey];
                                            ctx.fillStyle = getFeedingHeatColor(dens, 0.45);
                                            ctx.beginPath();
                                            ctx.arc(wx_d, wy_d, drawInnerRadius, 0, Math.PI * 2);
                                            ctx.fill();

                                            ctx.fillStyle = '#ffffff';
                                            ctx.font = 'bold 9px monospace';
                                            ctx.textAlign = 'center';
                                            ctx.textBaseline = 'middle';
                                            ctx.fillText(Math.round(dens) + "%", wx_d, wy_d);
                                        } else {
                                            ctx.strokeStyle = 'rgba(45, 212, 191, 0.8)'; // green outline
                                            ctx.beginPath();
                                            ctx.arc(wx_d, wy_d, drawRadius, 0, Math.PI * 2);
                                            ctx.stroke();

                                            if (pData.innerRatio < 1.0) {
                                                ctx.strokeStyle = 'rgba(45, 212, 191, 0.35)';
                                                ctx.beginPath();
                                                ctx.arc(wx_d, wy_d, drawInnerRadius, 0, Math.PI * 2);
                                                ctx.stroke();
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // A1 Drag Handle overlay
                    Rectangle {
                        id: a1Handle
                        width: 28
                        height: 28
                        radius: 14
                        color: "transparent"
                        border.color: "transparent"
                        visible: {
                            if (activePlateIdx < 0) return false;
                            var pData = platesModel.get(activePlateIdx);
                            return pData.framePath !== "" && pData.a1X >= 0;
                        }
                        x: {
                            if (activePlateIdx < 0) return 0;
                            var p = platesModel.get(activePlateIdx);
                            var geom = getScaleAndOffset();
                            return p.a1X * geom.scale + geom.xOffset - width / 2;
                        }
                        y: {
                            if (activePlateIdx < 0) return 0;
                            var p = platesModel.get(activePlateIdx);
                            var geom = getScaleAndOffset();
                            return p.a1Y * geom.scale + geom.yOffset - height / 2;
                        }
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.SizeAllCursor
                            property real lastX: 0
                            property real lastY: 0
                            onPressed: function(mouse) {
                                root.selectedHandle = "a1";
                                lastX = mouse.x;
                                lastY = mouse.y;
                                overlayCanvas.requestPaint();
                            }
                            onPositionChanged: function(mouse) {
                                if (pressed) {
                                    var deltaX = mouse.x - lastX;
                                    var deltaY = mouse.y - lastY;
                                    var p = platesModel.get(activePlateIdx);
                                    var geom = getScaleAndOffset();
                                    var deltaImgX = deltaX / geom.scale;
                                    var deltaImgY = deltaY / geom.scale;
                                    var newImgX = Math.max(0, Math.min(frameImage.sourceSize.width, p.a1X + deltaImgX));
                                    var newImgY = Math.max(0, Math.min(frameImage.sourceSize.height, p.a1Y + deltaImgY));
                                    platesModel.setProperty(activePlateIdx, "a1X", newImgX);
                                    platesModel.setProperty(activePlateIdx, "a1Y", newImgY);
                                    overlayCanvas.requestPaint();
                                }
                            }
                        }
                    }

                    // H12 Drag Handle overlay
                    Rectangle {
                        id: h12Handle
                        width: 28
                        height: 28
                        radius: 14
                        color: "transparent"
                        border.color: "transparent"
                        visible: {
                            if (activePlateIdx < 0) return false;
                            var pData = platesModel.get(activePlateIdx);
                            return pData.framePath !== "" && pData.h12X >= 0;
                        }
                        x: {
                            if (activePlateIdx < 0) return 0;
                            var p = platesModel.get(activePlateIdx);
                            var geom = getScaleAndOffset();
                            return p.h12X * geom.scale + geom.xOffset - width / 2;
                        }
                        y: {
                            if (activePlateIdx < 0) return 0;
                            var p = platesModel.get(activePlateIdx);
                            var geom = getScaleAndOffset();
                            return p.h12Y * geom.scale + geom.yOffset - height / 2;
                        }
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.SizeAllCursor
                            property real lastX: 0
                            property real lastY: 0
                            onPressed: function(mouse) {
                                root.selectedHandle = "h12";
                                lastX = mouse.x;
                                lastY = mouse.y;
                                overlayCanvas.requestPaint();
                            }
                            onPositionChanged: function(mouse) {
                                if (pressed) {
                                    var deltaX = mouse.x - lastX;
                                    var deltaY = mouse.y - lastY;
                                    var p = platesModel.get(activePlateIdx);
                                    var geom = getScaleAndOffset();
                                    var deltaImgX = deltaX / geom.scale;
                                    var deltaImgY = deltaY / geom.scale;
                                    var newImgX = Math.max(0, Math.min(frameImage.sourceSize.width, p.h12X + deltaImgX));
                                    var newImgY = Math.max(0, Math.min(frameImage.sourceSize.height, p.h12Y + deltaImgY));
                                    platesModel.setProperty(activePlateIdx, "h12X", newImgX);
                                    platesModel.setProperty(activePlateIdx, "h12Y", newImgY);
                                    overlayCanvas.requestPaint();
                                }
                            }
                        }
                    }
                }

                // Placeholder text
                ColumnLayout {
                    anchors.centerIn: parent
                    visible: activePlateIdx < 0 || platesModel.get(activePlateIdx).framePath === ""
                    spacing: 12
                    Text {
                        text: "📁"
                        font.pixelSize: 48
                        Layout.alignment: Qt.AlignHCenter
                    }
                    Text {
                        text: activePlateIdx >= 0 ? "Select video file (.mp4) for plate " + platesModel.get(activePlateIdx).barcode : "Select a plate from the left panel"
                        font.family: Style.fontSecondary
                        font.pixelSize: Style.fontSm
                        color: Style.subText
                        Layout.alignment: Qt.AlignHCenter
                    }
                }
            }

            // 3. Settings & Parameters Panel (Right)
            Rectangle {
                z: 1
                Layout.preferredWidth: 320
                Layout.fillHeight: true
                color: Style.panel
                border.color: Style.border
                radius: 8

                ScrollView {
                    anchors.fill: parent
                    clip: true

                    ColumnLayout {
                        anchors.top: parent.top
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.topMargin: 12
                        anchors.leftMargin: 16
                        anchors.rightMargin: 16
                        spacing: 14

                        Text {
                            text: "Plate settings:"
                            font.family: Style.fontPrimaryBold
                            font.pixelSize: Style.fontSm
                            color: Style.text
                            Layout.topMargin: 12
                        }

                        Button {
                            text: "Load Plate Video (.mp4)"
                            Layout.fillWidth: true
                            enabled: activePlateIdx >= 0
                            onClicked: videoFileDialog.open()
                        }

                        Rectangle { height: 1; Layout.fillWidth: true; color: Style.border }

                        // Timepoint frame select
                        Text {
                            text: "Frame Extraction Time (s):"
                            font.family: Style.fontPrimaryBold
                            font.pixelSize: Style.fontXs
                            color: Style.text
                        }
                        RowLayout {
                            spacing: 8
                            ComboBox {
                                id: frameTimeCombo
                                Layout.fillWidth: true
                                model: ["0.2", "0.5", "1.0", "1.5", "2.0"]
                                currentIndex: 1 // default 0.5s
                                onCurrentIndexChanged: {
                                    root.frameTimeSec = parseFloat(currentText);
                                    if (activePlateIdx >= 0 && platesModel.get(activePlateIdx).videoPath !== "") {
                                        extractActiveFrame();
                                    }
                                }
                            }
                        }

                        // Well Radius
                        RowLayout {
                            Layout.fillWidth: true
                            Text {
                                text: "Well Radius (px):"
                                font.family: Style.fontSecondary
                                font.pixelSize: Style.fontXs
                                color: Style.text
                                Layout.fillWidth: true
                            }
                            TextField {
                                id: radiusInput
                                width: 50
                                Layout.preferredWidth: 50
                                text: activeRadius.toFixed(0)
                                validator: IntValidator { bottom: 6; top: 80 }
                                enabled: activePlateIdx >= 0 && platesModel.get(activePlateIdx).framePath !== ""
                                onEditingFinished: {
                                    var val = parseInt(text);
                                    if (!isNaN(val)) {
                                        activeRadius = Math.max(6, Math.min(80, val));
                                    }
                                }
                            }
                        }
                        Slider {
                            Layout.fillWidth: true
                            from: 6.0
                            to: 80.0
                            value: activeRadius
                            enabled: activePlateIdx >= 0 && platesModel.get(activePlateIdx).framePath !== ""
                            onMoved: {
                                activeRadius = value;
                            }
                            onValueChanged: {
                                if (activeRadius !== value) {
                                    activeRadius = value;
                                }
                            }
                        }

                        // Inner ratio
                        RowLayout {
                            Layout.fillWidth: true
                            Text {
                                text: "Inner Sampling (%):"
                                font.family: Style.fontSecondary
                                font.pixelSize: Style.fontXs
                                color: Style.text
                                Layout.fillWidth: true
                            }
                            TextField {
                                id: innerRatioInput
                                width: 50
                                Layout.preferredWidth: 50
                                text: Math.round(activeInnerRatio * 100).toFixed(0)
                                validator: IntValidator { bottom: 50; top: 100 }
                                enabled: activePlateIdx >= 0 && platesModel.get(activePlateIdx).framePath !== ""
                                onEditingFinished: {
                                    var val = parseInt(text);
                                    if (!isNaN(val)) {
                                        activeInnerRatio = Math.max(50, Math.min(100, val)) / 100.0;
                                    }
                                }
                            }
                        }
                        Slider {
                            Layout.fillWidth: true
                            from: 0.5
                            to: 1.0
                            value: activeInnerRatio
                            enabled: activePlateIdx >= 0 && platesModel.get(activePlateIdx).framePath !== ""
                            onMoved: {
                                activeInnerRatio = value;
                            }
                            onValueChanged: {
                                if (activeInnerRatio !== value) {
                                    activeInnerRatio = value;
                                }
                            }
                        }

                        // Threshold Mode
                        Text {
                            text: "Thresholding Mode:"
                            font.family: Style.fontPrimaryBold
                            font.pixelSize: Style.fontXs
                            color: Style.text
                        }
                        ComboBox {
                            Layout.fillWidth: true
                            model: ["Otsu (per well)", "Otsu (per plate)", "Fixed luminance"]
                            currentIndex: (activePlateIdx >= 0) ? (platesModel.get(activePlateIdx).threshMode === "otsu-well" ? 0 : (platesModel.get(activePlateIdx).threshMode === "otsu-plate" ? 1 : 2)) : 0
                            enabled: activePlateIdx >= 0 && platesModel.get(activePlateIdx).framePath !== ""
                            onCurrentIndexChanged: {
                                if (activePlateIdx >= 0) {
                                    var mode = currentIndex === 0 ? "otsu-well" : (currentIndex === 1 ? "otsu-plate" : "fixed");
                                    platesModel.setProperty(activePlateIdx, "threshMode", mode);
                                }
                            }
                        }

                        // Fixed Threshold
                        ColumnLayout {
                            Layout.fillWidth: true
                            visible: activePlateIdx >= 0 && platesModel.get(activePlateIdx).threshMode === "fixed"
                            RowLayout {
                                Layout.fillWidth: true
                                Text {
                                    text: "Fixed Luminance Threshold:"
                                    font.family: Style.fontSecondary
                                    font.pixelSize: Style.fontXs
                                    color: Style.text
                                    Layout.fillWidth: true
                                }
                                TextField {
                                    id: fixedThreshInput
                                    width: 50
                                    Layout.preferredWidth: 50
                                    text: activeFixedThresh.toString()
                                    validator: IntValidator { bottom: 0; top: 255 }
                                    onEditingFinished: {
                                        var val = parseInt(text);
                                        if (!isNaN(val)) {
                                            activeFixedThresh = Math.max(0, Math.min(255, val));
                                        }
                                    }
                                }
                            }
                            Slider {
                                Layout.fillWidth: true
                                from: 0
                                to: 255
                                value: activeFixedThresh
                                onMoved: {
                                    activeFixedThresh = Math.round(value);
                                }
                                onValueChanged: {
                                    var rounded = Math.round(value);
                                    if (activeFixedThresh !== rounded) {
                                        activeFixedThresh = rounded;
                                    }
                                }
                            }
                        }

                        // Polarity selection
                        Text {
                            text: "Pixel Polarity (feeding proxy color):"
                            font.family: Style.fontPrimaryBold
                            font.pixelSize: Style.fontXs
                            color: Style.text
                        }
                        ComboBox {
                            Layout.fillWidth: true
                            model: ["Dark pixels", "Light pixels"]
                            currentIndex: (activePlateIdx >= 0) ? (platesModel.get(activePlateIdx).polarity === "dark" ? 0 : 1) : 0
                            enabled: activePlateIdx >= 0 && platesModel.get(activePlateIdx).framePath !== ""
                            onCurrentIndexChanged: {
                                if (activePlateIdx >= 0) {
                                    var pol = currentIndex === 0 ? "dark" : "light";
                                    platesModel.setProperty(activePlateIdx, "polarity", pol);
                                }
                            }
                        }

                        // Grid Calibration Help Instruction
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 80
                            color: Style.panel2
                            border.color: Style.border
                            radius: 6
                            clip: true
                            
                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 8
                                Text {
                                    text: "Calibration Instruction:"
                                    font.family: Style.fontPrimaryBold
                                    font.pixelSize: Style.fontXs - 1
                                    color: Style.accent
                                }
                                Text {
                                    text: {
                                        if (activePlateIdx < 0) return "Select a plate to start.";
                                        var p = platesModel.get(activePlateIdx);
                                        if (p.framePath === "") return "Load a video file to extract the frame.";
                                        if (p.calibrationStep === "a1") return "Click the exact center of well A1 (top-left).";
                                        if (p.calibrationStep === "h12") return "Now click the center of well H12 (bottom-right).";
                                        return "Calibration complete. Adjust radius if needed, then click Compute.";
                                    }
                                    font.family: Style.fontSecondary
                                    font.pixelSize: Style.fontXs - 1
                                    color: Style.text
                                    wrapMode: Text.Wrap
                                    Layout.fillWidth: true
                                }
                            }
                        }

                        // Compute trigger
                        Button {
                            text: "Compute Feeding Density"
                            Layout.fillWidth: true
                            Layout.topMargin: 8
                            highlighted: true
                            enabled: activePlateIdx >= 0 && platesModel.get(activePlateIdx).framePath !== "" && platesModel.get(activePlateIdx).calibrationStep === "done" && !root.isBusy
                            onClicked: computeFeedingDensity()
                        }

                        Button {
                            text: "Reset Calibration"
                            Layout.fillWidth: true
                            enabled: activePlateIdx >= 0 && platesModel.get(activePlateIdx).framePath !== ""
                            onClicked: {
                                if (activePlateIdx >= 0) {
                                    platesModel.setProperty(activePlateIdx, "a1X", -1.0);
                                    platesModel.setProperty(activePlateIdx, "a1Y", -1.0);
                                    platesModel.setProperty(activePlateIdx, "h12X", -1.0);
                                    platesModel.setProperty(activePlateIdx, "h12Y", -1.0);
                                    platesModel.setProperty(activePlateIdx, "calibrationStep", "a1");
                                    platesModel.setProperty(activePlateIdx, "isAnalyzed", false);
                                    platesModel.setProperty(activePlateIdx, "wellDensitiesJson", "");
                                    overlayCanvas.requestPaint();
                                }
                            }
                        }
                    }
                }
            }
        }

        Rectangle { height: 1; Layout.fillWidth: true; color: Style.border }

        // Action Buttons Footer
        RowLayout {
            Layout.fillWidth: true
            spacing: 12
            Layout.bottomMargin: 4

            Item { Layout.fillWidth: true }

            Button {
                text: "Cancel"
                onClicked: root.close()
            }

            Button {
                text: "Confirm & Apply to Pipeline"
                highlighted: true
                enabled: {
                    if (platesModel.count === 0) return false;
                    for (var i = 0; i < platesModel.count; ++i) {
                        if (!platesModel.get(i).isAnalyzed) return false;
                    }
                    return true;
                }
                onClicked: confirmAndApply()
            }
        }
    }

    // Processing Busy Overlay
    Rectangle {
        anchors.fill: parent
        color: Qt.rgba(0, 0, 0, 0.5)
        visible: root.isBusy
        
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
                running: root.isBusy
            }
            Text {
                text: "Processing video frame pixels... Please wait"
                font.family: Style.fontPrimaryBold
                font.pixelSize: Style.fontMd
                color: "#ffffff"
                Layout.alignment: Qt.AlignHCenter
            }
        }
    }
}
