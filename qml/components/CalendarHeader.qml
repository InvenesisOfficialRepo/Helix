import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import TestRequests 1.0
import "../controls"

RowLayout {
    id: root
    Layout.fillWidth: true
    spacing: Style.padSm

    property string title: "Calendar"
    property bool busy: false
    signal backClicked()

    Label {
        text: root.title
        color: Style.text
        font.family: Style.fontPrimaryBold
        font.pixelSize: Style.fontXl
        Layout.fillWidth: true
        elide: Text.ElideRight
    }

    BusyIndicator {
        running: root.busy
        visible: root.busy
        Layout.preferredWidth: 18
        Layout.preferredHeight: 18
    }

    Button {
        text: "Back"
        onClicked: root.backClicked()
    }
}
