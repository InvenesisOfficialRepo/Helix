import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import TestRequests 1.0

Item {
    id: chartRoot
    property var dataModel: []
    
    property var colors: ["#0ea5e9", "#10b981", "#f59e0b", "#ef4444", "#8b5cf6", "#ec4899", "#f97316", "#14b8a6", "#6366f1", "#84cc16"]
    
    onDataModelChanged: {
        canvas.requestPaint()
    }
    
    RowLayout {
        anchors.fill: parent
        spacing: 20
        
        Canvas {
            id: canvas
            Layout.preferredWidth: Math.min(parent.width * 0.5, parent.height)
            Layout.preferredHeight: Layout.preferredWidth
            Layout.alignment: Qt.AlignVCenter
            
            onWidthChanged: requestPaint()
            onHeightChanged: requestPaint()
            
            onPaint: {
                var ctx = getContext("2d")
                ctx.clearRect(0, 0, width, height)
                
                if (!chartRoot.dataModel || chartRoot.dataModel.length === 0) return
                
                var total = 0
                for (var i = 0; i < chartRoot.dataModel.length; i++) {
                    total += chartRoot.dataModel[i].value
                }
                
                if (total === 0) return
                
                var centerX = width / 2
                var centerY = height / 2
                var radius = Math.min(centerX, centerY) * 0.9
                
                var startAngle = -Math.PI / 2
                for (var j = 0; j < chartRoot.dataModel.length; j++) {
                    var slice = chartRoot.dataModel[j].value / total
                    var sliceAngle = slice * 2 * Math.PI
                    var endAngle = startAngle + sliceAngle
                    
                    ctx.beginPath()
                    ctx.moveTo(centerX, centerY)
                    ctx.arc(centerX, centerY, radius, startAngle, endAngle)
                    ctx.closePath()
                    
                    ctx.fillStyle = chartRoot.colors[j % chartRoot.colors.length]
                    ctx.fill()
                    
                    startAngle = endAngle
                }
                
                // Optional: donut hole
                ctx.beginPath()
                ctx.moveTo(centerX, centerY)
                ctx.arc(centerX, centerY, radius * 0.5, 0, 2 * Math.PI)
                ctx.closePath()
                ctx.fillStyle = Style.panel
                ctx.fill()
            }
        }
        
        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: ListModel { id: innerModel; dynamicRoles: true }
            
            Connections {
                target: chartRoot
                function onDataModelChanged() {
                    innerModel.clear()
                    if (chartRoot.dataModel) {
                        for (let i = 0; i < chartRoot.dataModel.length; i++) {
                            innerModel.append(chartRoot.dataModel[i])
                        }
                    }
                }
            }
            
            delegate: RowLayout {
                width: ListView.view.width
                height: 30
                spacing: 10
                
                Rectangle {
                    width: 12; height: 12; radius: 6
                    color: (typeof index !== "undefined" && index >= 0) ? chartRoot.colors[index % 10] : "transparent"
                }
                Label {
                    text: name || "Unknown"
                    color: Style.text
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                }
                Label {
                    text: value
                    color: Style.subText
                    font.bold: true
                }
            }
        }
    }
}
