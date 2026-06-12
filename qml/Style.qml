pragma Singleton
import QtQuick
import QtQml
import TestRequests 1.0

QtObject {
    id: root

    // Dynamic bindings to C++ ThemeManager
    readonly property bool isDark: App.theme.isDark

    // =========================================================
    // Brand metadata
    // =========================================================
    readonly property string brandTagline: "Swiss quality data"
    readonly property var brandValues: ["Innovation", "Flexibility", "Reliability"]
    readonly property string brandAesthetic: "Sleek • Modern • Clinical • Minimalist • Professional"
    readonly property string toneOfVoice: "Professional • Scientific • Precise • Authoritative"

    // =========================================================
    // Fonts
    // =========================================================
    readonly property var _senRegular: FontLoader { source: "qrc:/qt/qml/TestRequests/qml/fonts/Sen-Regular.ttf" }
    readonly property var _senBold:    FontLoader { source: "qrc:/qt/qml/TestRequests/qml/fonts/Sen-Bold.ttf" }

    readonly property var _mavenRegular: FontLoader { source: "qrc:/qt/qml/TestRequests/qml/fonts/MavenPro-Regular.ttf" }
    readonly property var _mavenBold:    FontLoader { source: "qrc:/qt/qml/TestRequests/qml/fonts/MavenPro-Bold.ttf" }

    readonly property var _iconFont: FontLoader { source: "qrc:/qt/qml/TestRequests/qml/fonts/MaterialSymbolsOutlined.ttf" }

    readonly property string fontPrimary:
        (_senRegular.name && _senRegular.name.length > 0) ? _senRegular.name : Qt.application.font.family
    readonly property string fontPrimaryBold:
        (_senBold.name && _senBold.name.length > 0) ? _senBold.name : fontPrimary

    readonly property string fontSecondary:
        (_mavenRegular.name && _mavenRegular.name.length > 0) ? _mavenRegular.name : fontPrimary
    readonly property string fontSecondaryBold:
        (_mavenBold.name && _mavenBold.name.length > 0) ? _mavenBold.name : fontSecondary

    readonly property string iconFontFamily:
        (_iconFont.name && _iconFont.name.length > 0) ? _iconFont.name : fontPrimary

    // Type scale
    readonly property int fontXs: 11
    readonly property int fontSm: 12
    readonly property int fontMd: 14
    readonly property int fontLg: 18
    readonly property int fontXl: 22
    readonly property int fontXxl: 28

    // Spacing / geometry
    readonly property int padXs: 6
    readonly property int padSm: 10
    readonly property int pad:   12
    readonly property int padLg: 16
    readonly property int padXl: 24

    // Border radius / borders
    readonly property int radiusSm: 8
    readonly property int radius:   12
    readonly property int radiusLg: 16
    readonly property int borderWidth: 1

    // Dynamic brand colors bound to C++ active theme
    readonly property color teal:  App.theme.brandAccent
    readonly property color steel: App.theme.brandAccent2
    readonly property color green: App.theme.statusOk
    readonly property color navy:  "#212934"

    // Semantic colors
    readonly property color accent:  teal
    readonly property color accent2: steel
    readonly property color ok:      green
    readonly property color warn:    App.theme.statusWarn
    readonly property color bad:     App.theme.statusBad

    // Dynamic UI canvas colors bound to C++ active theme
    readonly property color bg:      App.theme.backgroundMain
    readonly property color panel:   App.theme.backgroundPanel
    readonly property color panel2:  App.theme.backgroundPanel2

    readonly property color text:    App.theme.textPrimary
    readonly property color subText: App.theme.textSecondary

    readonly property color border:  App.theme.borderCard
    readonly property color divider: App.theme.divider

    // Interactive states
    readonly property color hover: Qt.rgba(accent.r, accent.g, accent.b, isDark ? 0.12 : 0.08)
    readonly property color press: Qt.rgba(accent.r, accent.g, accent.b, isDark ? 0.20 : 0.14)
    readonly property color focus: Qt.rgba(accent.r, accent.g, accent.b, isDark ? 0.45 : 0.32)

    function wash(alphaLight, alphaDark) {
        var a = isDark ? alphaDark : alphaLight
        return Qt.rgba(accent.r, accent.g, accent.b, a)
    }

    // Domain helper
    function statusColor(statusInt) {
        if (statusInt === 2) return ok
        if (statusInt === 1) return accent
        return accent2
    }

    function statusBg(statusInt) {
        var c = statusColor(statusInt)
        return Qt.rgba(c.r, c.g, c.b, isDark ? 0.18 : 0.12)
    }
}
