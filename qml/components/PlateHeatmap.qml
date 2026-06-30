import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import TestRequests 1.0

Column {
    id: plateRoot

    property int plateFormat: 96
    property var currentPlateWells: ({})
    property string selectedCompoundName: ""
    property var getEfficacyColor: null // function ref passed from parent

    signal compoundSelected(string compoundName)

    spacing: plateFormat === 384 ? 2 : 4
    Layout.alignment: Qt.AlignHCenter
    Layout.fillHeight: true

    property int numRows: plateFormat === 384 ? 16 : 8
    property int numCols: plateFormat === 384 ? 24 : 12

    property int cellSize: plateFormat === 384 ? 14 : 28
    property int cellRadius: plateFormat === 384 ? 7 : 14
    property int fontSz: plateFormat === 384 ? 8 : 10

    // Column Headers
    Row {
        spacing: plateFormat === 384 ? 2 : 4
        Item { width: 20; height: 20 } // Spacer for row letters
        Repeater {
            model: plateRoot.numCols
            Text {
                text: modelData + 1
                font.family: Style.fontPrimaryBold
                font.pixelSize: plateRoot.fontSz
                color: Style.subText
                width: plateRoot.cellSize
                height: 20
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }
    }

    // Rows
    Repeater {
        model: plateRoot.numRows
        
        Row {
            spacing: plateFormat === 384 ? 2 : 4
            property int rIdx: modelData
            property string rowLetter: String.fromCharCode(65 + rIdx)

            // Row Header label (A, B, C...)
            Text {
                text: parent.rowLetter
                font.family: Style.fontPrimaryBold
                font.pixelSize: plateRoot.fontSz
                color: Style.subText
                width: 20
                height: plateRoot.cellSize
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            // Wells
            Repeater {
                model: plateRoot.numCols
                
                Rectangle {
                    width: plateRoot.cellSize
                    height: plateRoot.cellSize
                    radius: plateRoot.cellRadius
                    
                    property int cIdx: modelData + 1
                    property string colStr: cIdx < 10 ? "0" + cIdx : "" + cIdx
                    property string wellKey: parent.rowLetter + colStr
                    property var wellData: plateRoot.currentPlateWells[wellKey] || null
                    
                    color: {
                        if (!wellData) return Style.isDark ? "#1e293b" : "#e2e8f0";
                        if (plateRoot.getEfficacyColor) {
                            return plateRoot.getEfficacyColor(wellData.efficacy);
                        }
                        return Style.isDark ? "#1e293b" : "#e2e8f0";
                    }
                    
                    border.color: wellData && wellData.compound === plateRoot.selectedCompoundName ? Style.accent : (wellHoverArea.containsMouse ? Style.accent2 : Style.border)
                    border.width: wellData && wellData.compound === plateRoot.selectedCompoundName ? 2.0 : (plateFormat === 384 ? 0.5 : 1.0)
                    
                    MouseArea {
                        id: wellHoverArea
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: {
                            if (parent.wellData && parent.wellData.compound !== "Empty" && parent.wellData.compound !== "DMSO") {
                                plateRoot.compoundSelected(parent.wellData.compound);
                            }
                        }
                        
                        ToolTip.visible: containsMouse && parent.wellData
                        ToolTip.text: parent.wellData ? (parent.wellKey + " - " + parent.wellData.compound + "\nEfficacy: " + Number(parent.wellData.efficacy).toFixed(1) + "%") : ""
                    }
                }
            }
        }
    }
}
