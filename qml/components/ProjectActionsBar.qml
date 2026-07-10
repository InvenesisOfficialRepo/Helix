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
    property int estimatedPlates: 0
    property var availableSpecies: []
    property var availableSolvents: []

    signal selectAllPendingClicked()
    signal selectAllBySpeciesClicked(string species)
    signal selectAllBySolventClicked(string solvent)
    signal sendSelectedClicked()

    Button {
        text: "Select All Pending"
        onClicked: root.selectAllPendingClicked()
    }

    ComboBox {
        id: speciesCombo
        visible: root.availableSpecies.length > 0
        model: root.availableSpecies
        textRole: ""
    }

    Button {
        text: "Select by Species"
        visible: root.availableSpecies.length > 0
        onClicked: {
            if (speciesCombo.currentText !== "") {
                root.selectAllBySpeciesClicked(speciesCombo.currentText)
            }
        }
    }

    ComboBox {
        id: solventCombo
        visible: root.availableSolvents.length > 0
        model: root.availableSolvents
        textRole: ""
    }

    Button {
        text: "Select by Solvent"
        visible: root.availableSolvents.length > 0
        onClicked: {
            if (solventCombo.currentText !== "") {
                root.selectAllBySolventClicked(solventCombo.currentText)
            }
        }
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

    ColumnLayout {
        spacing: 2
        
        Label {
            text: "Estimated Plates: " + root.estimatedPlates
            color: root.estimatedPlates > 0 ? Style.accent : Style.subText
            font.family: Style.fontSecondary
            font.pixelSize: Style.fontSm
            visible: root.estimatedPlates > 0
            font.bold: true
        }

        Label {
            text: "Compounds: " + root.compoundsCount
            color: Style.subText
            font.family: Style.fontSecondary
            font.pixelSize: Style.fontSm
        }
    }
}
