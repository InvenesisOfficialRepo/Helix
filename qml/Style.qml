pragma Singleton
import QtQuick
import QtQml
import TestRequests 1.0

QtObject {
    id: root

    // Dynamic bindings to C++ ThemeManager
    readonly property bool isDark: App.theme.isDark

    // =========================================================
    // Fonts
    // =========================================================
    readonly property string fontPrimary:        "Inter"
    readonly property string fontPrimaryBold:    "Inter"

    readonly property string fontSecondary:      "JetBrains Mono"
    readonly property string fontSecondaryBold:  "JetBrains Mono"

    readonly property string iconFontFamily:     "Material Symbols Outlined"

    // Type scale
    readonly property int fs2xs: 10
    readonly property int fsXs:  11
    readonly property int fsSm:  12
    readonly property int fsMd:  13
    readonly property int fsLg:  15
    readonly property int fsXl:  18
    readonly property int fs2xl: 22

    // Spacing
    readonly property int sp1: 2
    readonly property int sp2: 4
    readonly property int sp3: 6
    readonly property int sp4: 8
    readonly property int sp5: 10
    readonly property int sp6: 12
    readonly property int sp7: 16
    readonly property int sp8: 20
    readonly property int sp9: 24
    readonly property int sp10: 32
    readonly property int sp11: 40
    readonly property int sp12: 48

    // Density (controls heights and paddings)
    readonly property string density: App.theme.density
    readonly property int controlH: density === "comfortable" ? 26 : 22
    readonly property int controlHSm: density === "comfortable" ? 22 : 18
    readonly property int padX: density === "comfortable" ? 10 : 8

    // Border radius
    readonly property int radiusXs: 2
    readonly property int radiusSm: 3
    readonly property int radiusMd: 4
    readonly property int radiusLg: 6
    readonly property int radiusPill: 999

    // Dynamic UI canvas colors bound to C++ active theme
    readonly property color surfaceWindow: App.theme.backgroundMain
    readonly property color surfacePanel:  App.theme.backgroundPanel
    readonly property color surfaceRaised: App.theme.backgroundPanel2
    readonly property color surfaceHeader: App.theme.surfaceHeader
    readonly property color surfaceInput:  App.theme.surfaceInput
    readonly property color surfaceHover:  App.theme.surfaceHover
    readonly property color surfaceActive: App.theme.surfaceActive

    readonly property color text:          App.theme.textPrimary
    readonly property color subText:       App.theme.textSecondary
    readonly property color textMuted:     App.theme.textMuted
    readonly property color textDisabled:  App.theme.textDisabled

    readonly property color border:        App.theme.divider
    readonly property color borderStrong:  App.theme.borderStrong
    readonly property color edge:          App.theme.edge

    // Categorical series palette (Hardcoded per Workstation UI spec)
    readonly property color s1: "#5aa9e6"
    readonly property color s2: "#e0774c"
    readonly property color s3: "#5cc19b"
    readonly property color s4: "#e8c04e"
    readonly property color s5: "#b98ce0"
    readonly property color s6: "#4fc4c4"
    readonly property color s7: "#e58fb0"
    readonly property color s8: "#9aa7b4"

    // Semantic colors
    readonly property color accent:  App.theme.brandAccent
    readonly property color accent2: App.theme.brandAccent2
    readonly property color ok:      App.theme.statusOk
    readonly property color warn:    App.theme.statusWarn
    readonly property color bad:     App.theme.statusBad

    // For backwards compatibility during transition (do not use in new components)
    readonly property color bg: surfaceWindow
    readonly property color panel: surfacePanel
    readonly property color panel2: surfaceRaised
    readonly property color divider: border

    // Old typography
    readonly property int fontXs: fsXs
    readonly property int fontSm: fsSm
    readonly property int fontMd: fsMd
    readonly property int fontLg: fsLg
    readonly property int fontXl: fsXl
    readonly property int fontXxl: fs2xl

    // Old spacing
    readonly property int padXs: sp3
    readonly property int padSm: sp5
    readonly property int pad:   sp6
    readonly property int padLg: sp7
    readonly property int padXl: sp9

    // Old radii
    readonly property int radius: radiusMd
    readonly property int borderWidth: 1

    // Old colors
    readonly property color teal:  App.theme.brandAccent
    readonly property color steel: App.theme.brandAccent2
    readonly property color green: App.theme.statusOk
    readonly property color navy:  surfaceWindow

    // Old interaction
    readonly property color hover: surfaceHover
    readonly property color press: surfaceActive
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
