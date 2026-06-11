import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import TestRequests 1.0
import "../controls"

RowLayout {
    id: root
    Layout.fillWidth: true
    spacing: Style.padSm

    property string title: "Project Detail"
    signal backClicked()

    Text {
        text: root.title
        color: Style.text
        font.pixelSize: Style.fontXl
        font.family: Style.fontPrimaryBold
        Layout.fillWidth: true
        elide: Text.ElideRight
    }

    Button {
        text: "Back"
        onClicked: root.backClicked()
    }
}
