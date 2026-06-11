import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import TestRequests 1.0
import "../controls"
import "../dialogs"

RowLayout {
    id: root
    Layout.fillWidth: true
    spacing: Style.padSm

    property string username: ""
    property string role: ""
    property bool busy: false

    signal refreshClicked()
    signal calendarClicked()
    signal logoutClicked()

    // NEW
    signal addClicked()
    signal publishResultsClicked()

    Text {
        text: "User: " + root.username + " (" + root.role + ")"
        color: Style.subText
        font.family: Style.fontSecondary
        font.pixelSize: Style.fontSm
        Layout.fillWidth: true
        elide: Text.ElideRight
    }

    // NEW
    Button {
        text: "Add"
        enabled: !root.busy
        onClicked: root.addClicked()
    }

    Button {
        text: "Analyze & Publish"
        enabled: !root.busy
        onClicked: root.publishResultsClicked()
    }

    Button {
        text: "Refresh"
        enabled: !root.busy
        onClicked: root.refreshClicked()
    }

    Button {
        text: "Calendar"
        enabled: !root.busy
        onClicked: root.calendarClicked()
    }

    Button {
        text: "Logout"
        enabled: !root.busy
        onClicked: root.logoutClicked()
    }

    RowLayout {
        spacing: 8

        Text {
            text: "Theme:"
            color: Style.subText
            font.family: Style.fontSecondary
            font.pixelSize: Style.fontSm
        }

        ComboBox {
            id: themeSelector
            model: App.theme.availableThemes
            currentIndex: find(App.theme.currentTheme)
            
            onActivated: (index) => {
                App.theme.selectTheme(currentText)
            }
            
            Connections {
                target: App.theme
                function onThemeChanged() {
                    themeSelector.currentIndex = themeSelector.find(App.theme.currentTheme)
                    canvas.requestPaint()
                }
            }
            
            implicitWidth: 160
            background: Rectangle {
                color: Style.panel2
                border.color: Style.border
                border.width: 1
                radius: Style.radiusSm
            }
            
            contentItem: Text {
                leftPadding: 10
                rightPadding: 24
                text: themeSelector.currentText
                font.family: Style.fontSecondary
                font.pixelSize: Style.fontSm
                color: Style.text
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
            }
            
            indicator: Canvas {
                id: canvas
                x: themeSelector.width - width - 10
                y: themeSelector.topPadding + (themeSelector.availableHeight - height) / 2
                width: 12
                height: 8
                contextType: "2d"
                
                onPaint: {
                    var context = canvas.getContext("2d");
                    context.reset();
                    context.moveTo(0, 0);
                    context.lineTo(width, 0);
                    context.lineTo(width / 2, height);
                    context.closePath();
                    context.fillStyle = Style.text;
                    context.fill();
                }
            }
            
            popup: Popup {
                y: themeSelector.height + 4
                width: themeSelector.width
                implicitHeight: contentItem.implicitHeight
                padding: 1
                
                contentItem: ListView {
                    clip: true
                    implicitHeight: contentHeight
                    model: themeSelector.popup.visible ? themeSelector.delegateModel : null
                    currentIndex: themeSelector.highlightedIndex
                    
                    ScrollIndicator.vertical: ScrollIndicator { }
                }
                
                background: Rectangle {
                    color: Style.panel
                    border.color: Style.border
                    border.width: 1
                    radius: Style.radiusSm
                }
            }
            
            delegate: ItemDelegate {
                id: delegateRoot
                width: themeSelector.width
                height: 36
                
                contentItem: Text {
                    text: modelData
                    color: highlighted ? Style.accent : Style.text
                    font.family: Style.fontSecondary
                    font.pixelSize: Style.fontSm
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }
                
                background: Rectangle {
                    color: highlighted ? Style.hover : "transparent"
                    radius: Style.radiusSm
                }
            }
        }

        ToolButton {
            text: "palette"
            font.family: Style.iconFontFamily
            font.pixelSize: 20
            ToolTip.visible: hovered
            ToolTip.text: "Customize Colors"
            
            background: Rectangle {
                color: parent.hovered ? Style.hover : "transparent"
                radius: Style.radiusSm
            }
            
            contentItem: Text {
                text: parent.text
                font: parent.font
                color: Style.text
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            
            onClicked: themeCustomizer.open()
        }
    }

    ThemeCustomizerDialog {
        id: themeCustomizer
    }
}
