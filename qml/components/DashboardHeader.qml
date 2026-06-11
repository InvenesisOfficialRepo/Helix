import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import TestRequests 1.0
import "../controls"

RowLayout {
    id: root
    Layout.fillWidth: true
    spacing: Style.padSm

    property bool busy: false

    Text {
        text: "Dashboard"
        color: Style.text
        font.pixelSize: Style.fontXl
        font.family: Style.fontPrimaryBold
        Layout.fillWidth: true
    }

    BusyIndicator {
        running: root.busy
        visible: root.busy
        Layout.preferredWidth: 18
        Layout.preferredHeight: 18
    }
}
