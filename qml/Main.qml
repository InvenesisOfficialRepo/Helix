import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtCore
import TestRequests 1.0
import QtQuick.Controls.Material
import Qt.labs.platform 1.1

ApplicationWindow {
    id: win
    width: 900
    height: 650
    visible: true
    title: "Invenesis Master Hub"

    onClosing: (close) => {
        if (trayIcon.available) {
            close.accepted = false
            win.hide()
        }
    }

    SystemTrayIcon {
        id: trayIcon
        visible: true
        icon.source: "qrc:/icons/resources/icons/Sphere.png"
        tooltip: "Invenesis Master Hub"
        
        menu: Menu {
            MenuItem {
                text: "Open Master Hub"
                onTriggered: {
                    win.show()
                    win.raise()
                    win.requestActivate()
                }
            }
            MenuItem {
                text: "Quit"
                onTriggered: Qt.quit()
            }
        }
        
        onActivated: (reason) => {
            if (reason === SystemTrayIcon.Trigger || reason === SystemTrayIcon.DoubleClick) {
                win.show()
                win.raise()
                win.requestActivate()
            }
        }
    }

    Material.theme: Style.isDark ? Material.Dark : Material.Light
    Material.accent: Style.accent
    Material.primary: Style.accent2

    color: Style.bg

    Component.onCompleted: {
        stack.replace(loginPage)
    }

    property alias appStack: stack
    property var dashboardWindowInstance: null
    property var standaloneWindowInstance: null
    property var datapointManagerWindowInstance: null
    property var tecanPlannerWindowInstance: null

    function launchTecanPlannerWindow() {
        if (tecanPlannerWindowInstance === null) {
            tecanPlannerWindowInstance = tecanPlannerWindowComponent.createObject(null)
            tecanPlannerWindowInstance.closing.connect(function() {
                tecanPlannerWindowInstance = null
            })
        }
        tecanPlannerWindowInstance.show()
        tecanPlannerWindowInstance.raise()
        tecanPlannerWindowInstance.requestActivate()
    }

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

    function launchStandaloneAnalysis() {
        if (standaloneWindowInstance === null) {
            standaloneWindowInstance = standaloneWindowComponent.createObject(null)
            standaloneWindowInstance.closing.connect(function() {
                standaloneWindowInstance = null
            })
        }
        standaloneWindowInstance.show()
        standaloneWindowInstance.raise()
        standaloneWindowInstance.requestActivate()
    }

    function launchDatapointManager() {
        if (datapointManagerWindowInstance === null) {
            datapointManagerWindowInstance = datapointManagerWindowComponent.createObject(null)
            datapointManagerWindowInstance.closing.connect(function() {
                datapointManagerWindowInstance = null
            })
        }
        datapointManagerWindowInstance.show()
        datapointManagerWindowInstance.raise()
        datapointManagerWindowInstance.requestActivate()
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

    Component {
        id: standaloneWindowComponent
        ApplicationWindow {
            id: standWin
            width: 1400
            height: 900
            visible: false
            title: "Standalone Offline Analysis"

            Material.theme: Style.isDark ? Material.Dark : Material.Light
            Material.accent: Style.accent
            Material.primary: Style.accent2

            color: Style.bg

            property alias appStack: stack

            StackView {
                id: stack
                anchors.fill: parent
                initialItem: "pages/StandaloneAnalysisPage.qml"
            }
        }
    }

    Component {
        id: datapointManagerWindowComponent
        ApplicationWindow {
            id: datapointWin
            width: 1200
            height: 800
            visible: false
            title: "Datapoint & Monoplicate Manager"

            Material.theme: Style.isDark ? Material.Dark : Material.Light
            Material.accent: Style.accent
            Material.primary: Style.accent2

            color: Style.bg

            property alias appStack: stack

            StackView {
                id: stack
                anchors.fill: parent
                initialItem: "pages/DatapointManagerPage.qml"
            }
        }
    }

    Component {
        id: tecanPlannerWindowComponent
        ApplicationWindow {
            id: tecanWin
            width: 1400
            height: 900
            visible: false
            title: "Daily Tecan Planner"

            Material.theme: Style.isDark ? Material.Dark : Material.Light
            Material.accent: Style.accent
            Material.primary: Style.accent2

            color: Style.bg

            property alias appStack: stack

            StackView {
                id: stack
                anchors.fill: parent
                initialItem: "pages/DailyTecanPlannerPage.qml"
            }
        }
    }
}
