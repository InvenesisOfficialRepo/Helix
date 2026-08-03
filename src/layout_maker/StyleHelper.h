#ifndef STYLEHELPER_H
#define STYLEHELPER_H

#include <QString>
#include <QColor>

enum class BrightnessLevel {
    Darkest,
    Dark,
    Medium,
    Light
};

enum class AccentColor {
    Blue,
    Orange,
    Teal,
    Violet
};

enum class DensityLevel {
    Comfortable,
    Compact
};

struct WorkstationTokens {
    QString surfaceWindow;
    QString surfacePanel;
    QString surfaceRaised;
    QString surfaceHeader;
    QString surfaceInput;
    QString surfaceHover;
    QString surfaceActive;
    QString border;
    QString borderStrong;
    QString edge;
    QString textPrimary;
    QString textSecondary;
    QString textMuted;
    QString textDisabled;

    QString accent;
    QString accentHover;
    QString accentPress;
    QString onAccent;

    int controlHeight;
    int controlHeightSm;
    int fontSize;
    int radius;
    int paddingX;
    int gap;
};

class StyleHelper {
public:
    static WorkstationTokens getTokens(BrightnessLevel b, AccentColor a, DensityLevel d) {
        WorkstationTokens t;

        // 1. Brightness Surfaces
        switch (b) {
        case BrightnessLevel::Darkest:
            t.surfaceWindow = "#1e1e1e"; t.surfacePanel = "#252525"; t.surfaceRaised = "#2f2f2f";
            t.surfaceHeader = "#1a1a1a"; t.surfaceInput = "#161616"; t.surfaceHover  = "#333333";
            t.surfaceActive = "#3a3a3a"; t.border       = "#121212"; t.borderStrong = "#0d0d0d"; t.edge = "#3a3a3a";
            t.textPrimary   = "#d2d2d2"; t.textSecondary = "#9a9a9a"; t.textMuted    = "#6a6a6a"; t.textDisabled = "#4e4e4e";
            break;
        case BrightnessLevel::Dark:
        default:
            t.surfaceWindow = "#2b2b2b"; t.surfacePanel = "#333333"; t.surfaceRaised = "#3d3d3d";
            t.surfaceHeader = "#262626"; t.surfaceInput = "#232323"; t.surfaceHover  = "#434343";
            t.surfaceActive = "#4a4a4a"; t.border       = "#1c1c1c"; t.borderStrong = "#161616"; t.edge = "#4a4a4a";
            t.textPrimary   = "#dddddd"; t.textSecondary = "#a4a4a4"; t.textMuted    = "#727272"; t.textDisabled = "#585858";
            break;
        case BrightnessLevel::Medium:
            t.surfaceWindow = "#525252"; t.surfacePanel = "#5b5b5b"; t.surfaceRaised = "#666666";
            t.surfaceHeader = "#484848"; t.surfaceInput = "#424242"; t.surfaceHover  = "#6b6b6b";
            t.surfaceActive = "#747474"; t.border       = "#373737"; t.borderStrong = "#2e2e2e"; t.edge = "#6f6f6f";
            t.textPrimary   = "#f2f2f2"; t.textSecondary = "#cccccc"; t.textMuted    = "#a2a2a2"; t.textDisabled = "#828282";
            break;
        case BrightnessLevel::Light:
            t.surfaceWindow = "#bdbdbd"; t.surfacePanel = "#c9c9c9"; t.surfaceRaised = "#d6d6d6";
            t.surfaceHeader = "#b0b0b0"; t.surfaceInput = "#e4e4e4"; t.surfaceHover  = "#d0d0d0";
            t.surfaceActive = "#c0c0c0"; t.border       = "#909090"; t.borderStrong = "#7c7c7c"; t.edge = "#a4a4a4";
            t.textPrimary   = "#1c1c1c"; t.textSecondary = "#464646"; t.textMuted    = "#6a6a6a"; t.textDisabled = "#9a9a9a";
            break;
        }

        // 2. Accents
        switch (a) {
        case AccentColor::Blue:
        default:
            t.accent = "#4d90d0"; t.accentHover = "#5c9fdd"; t.accentPress = "#3f7fbd"; t.onAccent = "#ffffff";
            break;
        case AccentColor::Orange:
            t.accent = "#e08a3c"; t.accentHover = "#ee9950"; t.accentPress = "#c9762c"; t.onAccent = "#1c1204";
            break;
        case AccentColor::Teal:
            t.accent = "#2fb3a6"; t.accentHover = "#3fc7b9"; t.accentPress = "#259689"; t.onAccent = "#04201d";
            break;
        case AccentColor::Violet:
            t.accent = "#9a7bd6"; t.accentHover = "#ab8ee2"; t.accentPress = "#8567c0"; t.onAccent = "#160b26";
            break;
        }

        // 3. Density
        if (d == DensityLevel::Compact) {
            t.controlHeight = 22;
            t.controlHeightSm = 18;
            t.fontSize = 11;
            t.radius = 2;
            t.paddingX = 8;
            t.gap = 6;
        } else {
            t.controlHeight = 26;
            t.controlHeightSm = 22;
            t.fontSize = 12;
            t.radius = 3;
            t.paddingX = 10;
            t.gap = 8;
        }

        return t;
    }

    static QString getAppStylesheet(BrightnessLevel b = BrightnessLevel::Dark,
                                   AccentColor a = AccentColor::Blue,
                                   DensityLevel d = DensityLevel::Comfortable) {
        WorkstationTokens t = getTokens(b, a, d);

        return QString(R"(
            QMainWindow {
                background-color: %1;
                color: %2;
                font-family: 'Inter', 'Segoe UI', system-ui, sans-serif;
                font-size: %3px;
            }
            QWidget {
                color: %2;
                font-family: 'Inter', 'Segoe UI', system-ui, sans-serif;
                font-size: %3px;
            }
            QGroupBox {
                border: 1px solid %4;
                border-radius: %5px;
                margin-top: 6px;
                padding-top: 8px;
                font-weight: 600;
                font-size: %6px;
                text-transform: uppercase;
                letter-spacing: 0.5px;
                color: %7;
                background-color: %8;
            }
            QGroupBox::title {
                subcontrol-origin: margin;
                subcontrol-position: top left;
                padding: 1px 4px;
                color: %9;
            }
            QPushButton {
                background-color: %8;
                border: 1px solid %4;
                border-radius: %5px;
                color: %2;
                padding: 2px %10px;
                height: %11px;
                font-weight: 600;
                font-size: %3px;
            }
            QPushButton:hover {
                background-color: %12;
                border-color: %13;
                color: %2;
            }
            QPushButton:pressed {
                background-color: %14;
            }
            QPushButton#primaryBtn {
                background-color: %9;
                border: 1px solid %9;
                color: %15;
                font-weight: 700;
            }
            QPushButton#primaryBtn:hover {
                background-color: %16;
                border-color: %16;
            }
            QPushButton#dangerBtn {
                background-color: #d95757;
                border: 1px solid #b84343;
                color: #ffffff;
            }
            QPushButton#dangerBtn:hover {
                background-color: #ee6666;
            }
            QComboBox {
                background-color: %17;
                border: 1px solid %4;
                border-radius: %5px;
                padding: 1px 6px;
                height: %11px;
                color: %2;
                font-size: %3px;
                font-weight: 500;
            }
            QComboBox:hover {
                border-color: %9;
            }
            QComboBox::drop-down {
                border: none;
                width: 14px;
            }
            QComboBox QAbstractItemView {
                background-color: %8;
                border: 1px solid %4;
                selection-background-color: %9;
                selection-color: %15;
                color: %2;
            }
            QRadioButton {
                color: %2;
                font-size: %3px;
                font-weight: 500;
                spacing: 4px;
            }
            QRadioButton::indicator {
                width: 12px;
                height: 12px;
                border-radius: 6px;
                border: 1px solid %13;
                background-color: %17;
            }
            QRadioButton::indicator:checked {
                border-color: %9;
                background-color: %9;
            }
            QTabWidget::pane {
                border: 1px solid %4;
                border-radius: %5px;
                background-color: %8;
            }
            QTabBar::tab {
                background-color: %1;
                border: 1px solid %4;
                border-bottom: none;
                border-top-left-radius: %5px;
                border-top-right-radius: %5px;
                padding: 4px 10px;
                color: %7;
                font-weight: 600;
                font-size: %3px;
                margin-right: 2px;
            }
            QTabBar::tab:selected {
                background-color: %8;
                color: %9;
                border-bottom: 2px solid %9;
            }
            QTabBar::tab:hover:!selected {
                background-color: %12;
                color: %2;
            }
            QTableWidget {
                background-color: %17;
                border: 1px solid %4;
                gridline-color: %8;
                color: %2;
                selection-background-color: %9;
                selection-color: %15;
                border-radius: %5px;
            }
            QHeaderView::section {
                background-color: %18;
                color: %9;
                padding: 3px 6px;
                border: 1px solid %4;
                font-weight: 700;
                font-size: %6px;
            }
            QLineEdit {
                background-color: %17;
                border: 1px solid %4;
                border-radius: %5px;
                padding: 2px 6px;
                height: %11px;
                color: %2;
                font-size: %3px;
            }
            QLineEdit:focus {
                border-color: %9;
            }
            QToolTip {
                background-color: %1;
                color: %2;
                border: 1px solid %9;
                border-radius: %5px;
                padding: 3px 6px;
                font-size: %6px;
            }
            QScrollBar:vertical, QScrollBar:horizontal {
                background: %1;
                border: none;
                width: 8px;
                height: 8px;
            }
            QScrollBar::handle:vertical, QScrollBar::handle:horizontal {
                background: %13;
                border-radius: 4px;
            }
            QScrollBar::handle:vertical:hover, QScrollBar::handle:horizontal:hover {
                background: %7;
            }
        )")
        .arg(t.surfaceWindow)      // %1
        .arg(t.textPrimary)        // %2
        .arg(t.fontSize)           // %3
        .arg(t.border)             // %4
        .arg(t.radius)             // %5
        .arg(t.fontSize - 1)       // %6
        .arg(t.textMuted)          // %7
        .arg(t.surfacePanel)       // %8
        .arg(t.accent)             // %9
        .arg(t.paddingX)           // %10
        .arg(t.controlHeight)      // %11
        .arg(t.surfaceHover)       // %12
        .arg(t.edge)               // %13
        .arg(t.surfaceActive)      // %14
        .arg(t.onAccent)           // %15
        .arg(t.accentHover)        // %16
        .arg(t.surfaceInput)       // %17
        .arg(t.surfaceHeader);     // %18
    }
};

#endif // STYLEHELPER_H
