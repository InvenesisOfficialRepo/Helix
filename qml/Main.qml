import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtCore
import TestRequests 1.0
import QtQuick.Controls.Material

ApplicationWindow {
    id: win
    width: 900
    height: 650
    visible: true
    title: "Invenesis Master Hub"

    Material.theme: Style.isDark ? Material.Dark : Material.Light
    Material.accent: Style.accent
    Material.primary: Style.accent2

    color: Style.bg

    Component.onCompleted: {
        stack.replace(loginPage)
    }

    property alias appStack: stack
    property var dashboardWindowInstance: null

    function launchDashboardWindow() {
        if (dashboardWindowInstance === null) {
            dashboardWindowInstance = dashboardWindowComponent.createObject(null)
            dashboardWindowInstance.closing.connect(function() {
                dashboardWindowInstance = null
            })
        }
        dashboardWindowInstance.show()
        dashboardWindowInstance.raise()
        dashboardWindowInstance.requestActivate()
    }

    StackView {
        id: stack
        anchors.fill: parent
    }

    Component { id: loginPage; LoginPage {} }
    Component { id: launcherHubPage; MasterLauncherHub {} }

    Connections {
        target: App.session
        function onIsAuthenticatedChanged() {
            if (App.session.isAuthenticated) stack.replace(launcherHubPage)
            else {
                if (dashboardWindowInstance !== null) {
                    dashboardWindowInstance.close()
                    dashboardWindowInstance = null
                }
                stack.replace(loginPage)
            }
        }
    }

    Component {
        id: dashboardWindowComponent
        ApplicationWindow {
            id: dashWin
            width: 1400
            height: 900
            visible: false
            title: "Test Request Dashboard"

            Material.theme: Style.isDark ? Material.Dark : Material.Light
            Material.accent: Style.accent
            Material.primary: Style.accent2

            color: Style.bg

            property alias appStack: stack

            StackView {
                id: stack // Named 'stack' so inner views can call stack.push/pop dynamically
                anchors.fill: parent
                initialItem: DashboardPage {}
            }
        }
    }
}
