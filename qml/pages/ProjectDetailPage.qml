import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import TestRequests 1.0
import "../components"
import "../controls"
import "../dialogs"

Page {
    id: root
    background: null

    function popPage() {
        var w = root.Window.window
        if (w && w.appStack) w.appStack.pop()
        else console.error("ProjectDetailPage: window/appStack not available")
    }

    DatePickerDialog {
        id: dateDialog
        onAcceptedDate: (d) => App.project.sendSelected(d)
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Style.pad
        spacing: Style.pad

        ProjectHeader {
            title: "Project Detail"
            onBackClicked: popPage()
        }

        ProjectInfoCard {
            batchId: App.project.batchId
            projectCode: "Project: " + App.project.projectCode
            testCode: App.project.testCode
            overallStatus: App.project.overallStatus
            scheduledFor: App.project.scheduledFor
            createdBy: App.project.createdBy
        }

        ProjectActionsBar {
            canSend: App.session.canSend
            role: App.session.role
            compoundsCount: compounds.count
            estimatedPlates: App.project.estimatedTestPlates
            availableSpecies: App.project.compoundsModel.availableSpecies
            availableSolvents: App.project.compoundsModel.availableSolvents

            onSelectAllPendingClicked: App.project.selectAllPending()
            onFilterBySpeciesChanged: { App.project.compoundsModel.setSpeciesFilter(species); }
            onSelectAllBySolventClicked: { App.project.compoundsModel.selectAllBySolvent(solvent); }

            onSendSelectedClicked: {
                if (App.project.scheduledFor && App.project.scheduledFor.isValid && App.project.scheduledFor.isValid()) {
                    dateDialog.initialDate = App.project.scheduledFor
                } else {
                    dateDialog.initialDate = new Date()
                }
                dateDialog.open()
            }
        }

        ErrorBanner {
            message: App.project.errorMessage
            severity: "error"
        }

        ListView {
            id: compounds
            Layout.fillWidth: true
            Layout.fillHeight: true
            topMargin: 4
            leftMargin: 4
            rightMargin: 4
            bottomMargin: 6
            clip: true
            spacing: Style.padSm
            model: App.project.compoundsModel

            delegate: CompoundRow {
                width: compounds.width - compounds.leftMargin - compounds.rightMargin
                name: model.compoundName
                species: model.species
                solvent: model.solvent
                statusText: model.statusText
                selectable: model.selectable
                checked: model.selected
                onToggled: { App.project.compoundsModel.setSelected(index, checked) }
            }

            footer: Item { height: 8 }
        }

        Label {
            Layout.fillWidth: true
            visible: compounds.count === 0
            text: "No compounds loaded yet."
            color: Style.subText
            font.family: Style.fontSecondary
            font.pixelSize: Style.fontSm
            horizontalAlignment: Text.AlignHCenter
        }
    }
}
