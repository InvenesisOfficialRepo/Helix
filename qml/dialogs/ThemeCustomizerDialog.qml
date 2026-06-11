import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import TestRequests 1.0
import "../controls"

Popup {
    id: root
    
    x: parent.width - width
    y: 0
    width: 400
    height: parent.height
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    
    Overlay.modal: Rectangle {
        color: Qt.rgba(0, 0, 0, 0.4)
    }
    
    enter: Transition {
        NumberAnimation { property: "x"; from: parent.width; to: parent.width - width; duration: 250; easing.type: Easing.OutCubic }
    }
    exit: Transition {
        NumberAnimation { property: "x"; from: parent.width - width; to: parent.width; duration: 200; easing.type: Easing.InCubic }
    }
    
    background: Rectangle {
        color: Style.panel
        border.color: Style.border
        border.width: 1
    }
    
    property string activeTokenKey: ""
    property string activeTokenName: ""
    
    ListModel {
        id: tokenModel
        ListElement { tokenKey: "backgroundMain";   tokenName: "Background Canvas" }
        ListElement { tokenKey: "backgroundPanel";  tokenName: "Container Background" }
        ListElement { tokenKey: "backgroundPanel2"; tokenName: "Interactive Background" }
        ListElement { tokenKey: "borderCard";       tokenName: "Card Border Frame" }
        ListElement { tokenKey: "divider";          tokenName: "Horizontal Dividers" }
        ListElement { tokenKey: "textPrimary";      tokenName: "Primary Text" }
        ListElement { tokenKey: "textSecondary";    tokenName: "Secondary Text/Metadata" }
        ListElement { tokenKey: "brandAccent";      tokenName: "Active Accents" }
        ListElement { tokenKey: "brandAccent2";     tokenName: "Accent Highlighters" }
        ListElement { tokenKey: "statusOk";         tokenName: "Status Success (Green)" }
        ListElement { tokenKey: "statusWarn";       tokenName: "Status Pending (Amber)" }
        ListElement { tokenKey: "statusBad";        tokenName: "Status High-Alert (Red)" }
    }
    
    ColorDialog {
        id: colorDialog
        title: "Pick color for " + root.activeTokenName
        options: ColorDialog.DontUseNativeDialog
        
        onAccepted: {
            if (root.activeTokenKey !== "") {
                App.theme.setCustomColor(root.activeTokenKey, selectedColor.toString(), true)
            }
        }
        onSelectedColorChanged: {
            if (root.activeTokenKey !== "" && colorDialog.visible) {
                App.theme.setCustomColor(root.activeTokenKey, selectedColor.toString(), false)
            }
        }
    }
    
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Style.padLg
        spacing: Style.padLg
        
        // Header
        RowLayout {
            Layout.fillWidth: true
            
            Text {
                text: "Theme Customizer"
                color: Style.text
                font.family: Style.fontPrimaryBold
                font.pixelSize: Style.fontLg
                Layout.fillWidth: true
            }
            
            ToolButton {
                text: "close"
                font.family: Style.iconFontFamily
                font.pixelSize: 18
                onClicked: root.close()
            }
        }
        
        // Separator
        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: Style.divider
        }
        
        // Preset Cloner
        ColumnLayout {
            Layout.fillWidth: true
            spacing: Style.padXs
            
            Text {
                text: "Clone Curated Baseline:"
                color: Style.subText
                font.family: Style.fontSecondary
                font.pixelSize: Style.fontXs
            }
            
            RowLayout {
                Layout.fillWidth: true
                spacing: Style.padSm
                
                ComboBox {
                    id: cloneCombo
                    Layout.fillWidth: true
                    
                    model: ["Clinical Dark", "Swiss Light", "Cyberpunk Neon", "Forest Laboratory"]
                    
                    background: Rectangle {
                        color: Style.panel2
                        border.color: Style.border
                        border.width: 1
                        radius: Style.radiusSm
                    }
                    
                    contentItem: Text {
                        leftPadding: 10
                        text: cloneCombo.currentText
                        font.family: Style.fontSecondary
                        font.pixelSize: Style.fontSm
                        color: Style.text
                        verticalAlignment: Text.AlignVCenter
                    }
                }
                
                Button {
                    text: "Clone"
                    onClicked: {
                        App.theme.resetCustomThemeToPreset(cloneCombo.currentText)
                    }
                }
            }
        }
        
        // Separator
        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: Style.divider
        }
        
        // Token List ScrollView
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            
            ListView {
                model: tokenModel
                spacing: 4
                
                delegate: ItemDelegate {
                    width: parent ? parent.width - Style.pad : 0
                    height: 48
                    
                    background: Rectangle {
                        color: hovered ? Style.hover : "transparent"
                        radius: Style.radiusSm
                        border.color: hovered ? Style.border : "transparent"
                        border.width: 1
                    }
                    
                    onClicked: {
                        root.activeTokenKey = model.tokenKey;
                        root.activeTokenName = model.tokenName;
                        
                        var activeColor = App.theme[model.tokenKey];
                        colorDialog.selectedColor = activeColor;
                        colorDialog.open();
                    }
                    
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: Style.padSm
                        anchors.rightMargin: Style.padSm
                        spacing: Style.padSm
                        
                        Text {
                            text: model.tokenName
                            color: Style.text
                            font.family: Style.fontSecondary
                            font.pixelSize: Style.fontSm
                            font.bold: true
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                        }
                        
                        RowLayout {
                            spacing: Style.padSm
                            Layout.alignment: Qt.AlignRight
                            
                            Text {
                                text: App.theme[model.tokenKey].toString().toUpperCase()
                                color: Style.subText
                                font.family: Style.fontSecondary
                                font.pixelSize: Style.fontXs
                                horizontalAlignment: Text.AlignRight
                            }
                            
                            // Colored Preview Circle
                            Rectangle {
                                width: 20
                                height: 20
                                radius: 10
                                color: App.theme[model.tokenKey]
                                border.color: Style.border
                                border.width: 2
                                Layout.alignment: Qt.AlignVCenter
                            }
                        }
                    }
                }
            }
        }
        
        // Separator
        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: Style.divider
        }
        
        // Save Theme Section
        ColumnLayout {
            Layout.fillWidth: true
            spacing: Style.padXs
            
            Text {
                text: "Save Custom Theme As:"
                color: Style.subText
                font.family: Style.fontSecondary
                font.pixelSize: Style.fontXs
            }
            
            RowLayout {
                Layout.fillWidth: true
                spacing: Style.padSm
                
                TextField {
                    id: saveNameInput
                    Layout.fillWidth: true
                    placeholderText: "My theme name..."
                    
                    background: Rectangle {
                        color: Style.panel2
                        border.color: Style.border
                        border.width: 1
                        radius: Style.radiusSm
                    }
                    
                    color: Style.text
                    font.family: Style.fontSecondary
                    font.pixelSize: Style.fontSm
                    leftPadding: 10
                }
                
                Button {
                    text: "Save"
                    enabled: saveNameInput.text.trim().length > 0
                    onClicked: {
                        var name = saveNameInput.text.trim();
                        App.theme.saveCustomTheme(name);
                        saveNameInput.text = "";
                    }
                }
            }
        }
        
        // User Saved Presets Section (Only visible if there is at least one custom preset)
        ColumnLayout {
            id: savedPresetsSection
            Layout.fillWidth: true
            spacing: Style.padXs
            
            property var customPresets: {
                var list = App.theme.availableThemes;
                var res = [];
                for (var i = 0; i < list.length; ++i) {
                    var t = list[i];
                    if (t !== "Custom" && t !== "Clinical Dark" && t !== "Swiss Light" && t !== "Cyberpunk Neon" && t !== "Forest Laboratory") {
                        res.push(t);
                    }
                }
                return res;
            }
            
            visible: savedPresetsSection.customPresets.length > 0
            
            Text {
                text: "My Saved Themes:"
                color: Style.subText
                font.family: Style.fontSecondary
                font.pixelSize: Style.fontXs
            }
            
            ScrollView {
                Layout.fillWidth: true
                implicitHeight: Math.min(savedPresetsSection.customPresets.length * 36, 120)
                clip: true
                
                ListView {
                    model: savedPresetsSection.customPresets
                    spacing: 4
                    
                    delegate: ItemDelegate {
                        width: parent ? parent.width - 8 : 0
                        height: 32
                        
                        background: Rectangle {
                            color: hovered ? Style.hover : "transparent"
                            radius: Style.radiusSm
                        }
                        
                        onClicked: {
                            App.theme.selectTheme(modelData)
                        }
                        
                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: Style.padSm
                            anchors.rightMargin: Style.padSm
                            
                            Text {
                                text: modelData
                                color: Style.text
                                font.family: Style.fontSecondary
                                font.pixelSize: Style.fontSm
                                Layout.fillWidth: true
                                elide: Text.ElideRight
                                font.bold: App.theme.currentTheme === modelData
                            }
                            
                            ToolButton {
                                text: "delete"
                                font.family: Style.iconFontFamily
                                font.pixelSize: 16
                                ToolTip.visible: hovered
                                ToolTip.text: "Delete Preset"
                                
                                background: Rectangle {
                                    color: parent.hovered ? Style.hover : "transparent"
                                    radius: Style.radiusSm
                                }
                                
                                contentItem: Text {
                                    text: parent.text
                                    font: parent.font
                                    color: Style.bad
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }
                                
                                onClicked: {
                                    App.theme.deleteCustomTheme(modelData)
                                }
                            }
                        }
                    }
                }
            }
        }
        
        // Separator
        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: Style.divider
        }
        
        // Bottom Actions
        RowLayout {
            Layout.fillWidth: true
            spacing: Style.padSm
            
            Button {
                text: "Reset Custom to Default"
                Layout.fillWidth: true
                onClicked: {
                    App.theme.resetCustomThemeToPreset("Clinical Dark")
                }
            }
        }
    }
}
