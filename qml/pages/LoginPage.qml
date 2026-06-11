import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import TestRequests 1.0
import "../components"
import "../controls"

Page {
    id: root
    focus: true

    // App background
    background: Rectangle { color: Style.bg }

    function attemptLogin() {
        if (App.session.busy || !App.dbReady) return
        App.session.login(usernameField.text, passwordField.text)
        passwordField.text = ""
    }

    Component.onCompleted: usernameField.forceActiveFocus()

    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            spacing: Style.pad
            Layout.topMargin: Style.padXs

            Label {
                text: "Login"
                font.family: Style.fontPrimaryBold
                font.pixelSize: Style.fontLg
                color: Style.text
                Layout.leftMargin: Style.padLg
            }

            Item { Layout.fillWidth: true }

            // DB status pill
            DbStatusPill {
                Layout.rightMargin: Style.padLg
                Layout.topMargin: 3
                Layout.bottomMargin: 3
            }
        }
    }

    // Optional: Enter anywhere while a field is focused attempts login
    Keys.onPressed: function(e) {
        if ((e.key === Qt.Key_Return || e.key === Qt.Key_Enter) &&
            (usernameField.activeFocus || passwordField.activeFocus)) {
            attemptLogin()
            e.accepted = true
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Style.padLg
        spacing: Style.padLg

        Item { Layout.fillHeight: true } // top spacer

        Image {
            id: logo
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 100
            Layout.preferredHeight: 100
            source: Qt.resolvedUrl("../images/sphere.png")
            fillMode: Image.PreserveAspectFit
            smooth: true
            mipmap: true
            antialiasing: true
        }

        Item { height: Style.padSm } // small gap under logo

        // Card container
        Frame {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 440
            padding: Style.pad

            background: Rectangle {
                radius: Style.radiusLg
                color: Style.panel
                border.color: Style.border
                border.width: Style.borderWidth
            }

            ColumnLayout {
                spacing: Style.pad
                anchors.fill: parent

                Label {
                    text: "Test Request Tracking"
                    font.family: Style.fontPrimaryBold
                    font.pixelSize: Style.fontXl
                    color: Style.text
                    Layout.fillWidth: true
                }

                DbConfigBanner {
                    severity: "error"
                }

                TextField {
                    id: usernameField
                    Layout.fillWidth: true
                    placeholderText: "Username"
                    enabled: !App.session.busy

                    // Enter on username moves to password
                    onAccepted: passwordField.forceActiveFocus()
                }

                TextField {
                    id: passwordField
                    Layout.fillWidth: true
                    placeholderText: "Password"
                    echoMode: showPw.checked ? TextInput.Normal : TextInput.Password
                    enabled: !App.session.busy

                    // Enter on password triggers login
                    onAccepted: attemptLogin()
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Style.padSm

                    CheckBox {
                        id: showPw
                        text: "Show password"
                        enabled: !App.session.busy
                    }

                    Item { Layout.fillWidth: true }

                    BusyIndicator {
                        running: App.session.busy
                        visible: App.session.busy
                        Layout.preferredWidth: 18
                        Layout.preferredHeight: 18
                    }
                }

                // Login error banner
                Rectangle {
                    Layout.fillWidth: true
                    visible: App.session.errorMessage && App.session.errorMessage.length > 0
                    radius: Style.radiusSm
                    color: Qt.rgba(Style.bad.r, Style.bad.g, Style.bad.b, 0.12)
                    border.color: Qt.rgba(Style.bad.r, Style.bad.g, Style.bad.b, 0.55)
                    border.width: Style.borderWidth
                    implicitHeight: errText.implicitHeight + Style.padSm * 2

                    Text {
                        id: errText
                        anchors.margins: Style.padSm
                        anchors.fill: parent
                        text: App.session.errorMessage
                        wrapMode: Text.Wrap
                        color: Style.text
                        font.family: Style.fontSecondary
                        font.pixelSize: Style.fontSm
                    }
                }

                Button {
                    id: loginBtn
                    Layout.fillWidth: true
                    text: App.session.busy ? "Signing in..." : "Sign in"
                    enabled: !App.session.busy && App.dbReady
                    onClicked: attemptLogin()
                }

                Label {
                    Layout.fillWidth: true
                    text: App.dbReady
                          ? "Enter your credentials to continue."
                          : "Database not ready: check INV_DB_* environment variables."
                    color: Style.subText
                    wrapMode: Text.Wrap
                    font.family: Style.fontSecondary
                    font.pixelSize: Style.fontSm
                }
            }
        }

        Item { Layout.fillHeight: true } // bottom spacer
    }
}
