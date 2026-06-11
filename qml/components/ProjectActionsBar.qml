import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import TestRequests 1.0
import "../controls"

RowLayout {
    id: root
    Layout.fillWidth: true
    spacing: Style.padSm

    property bool canSend: false
    property string role: ""
    property int compoundsCount: 0

    signal selectAllPendingClicked()
    signal sendSelectedClicked()

    Button {
        text: "Select All Pending"
        onClicked: root.selectAllPendingClicked()
    }

    Button {
        text: "Send Selected"
        enabled: root.canSend
        onClicked: root.sendSelectedClicked()
    }

    Label {
        visible: !root.canSend
        text: "Send disabled (role: " + root.role + ")"
        color: Style.warn
        font.family: Style.fontSecondary
        font.pixelSize: Style.fontSm
        verticalAlignment: Text.AlignVCenter
    }

    Item { Layout.fillWidth: true }

    Label {
        text: "Compounds: " + root.compoundsCount
        color: Style.subText
        font.family: Style.fontSecondary
        font.pixelSize: Style.fontSm
    }
}
