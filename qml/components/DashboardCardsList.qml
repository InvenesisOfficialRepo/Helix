import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import TestRequests 1.0
import "../components"

Item {
    id: root
    Layout.fillWidth: true
    Layout.fillHeight: true

    property var cardsModel: null
    readonly property int count: cards.count
    signal openBatch(string batchId)

    ListView {
        id: cards
        anchors.fill: parent
        clip: true
        spacing: Style.pad
        model: root.cardsModel

        delegate: TestCard {
            width: cards.width
            testCode: model.testCode
            pendingCount: model.pendingCount
            sentCount: model.sentCount
            projectsModel: model.projectsModel

            // IMPORTANT: forward as string (uuid)
            onOpenBatch: function(batchId) {
                // batchId from TestCard is already a uuid string
                root.openBatch(String(batchId))
            }
        }
    }
}
