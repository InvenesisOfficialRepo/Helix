#include "ThemeManager.h"
#include <QSettings>
#include <QFile>
#include <QDebug>

ThemeManager* ThemeManager::instance()
{
    static ThemeManager manager;
    return &manager;
}

ThemeManager::ThemeManager(QObject *parent)
    : QObject(parent)
    , currentThemeName_("Workstation Dark")
{
    setupPresets();
}

void ThemeManager::initialize(QApplication* app)
{
    app_ = app;
    
    // Load last saved preset theme
    QSettings settings("Invenesis", "MergeApp");
    QString savedThemeName = settings.value("themePreset", "Workstation Dark").toString();
    
    if (themes_.contains(savedThemeName)) {
        currentThemeName_ = savedThemeName;
    } else {
        currentThemeName_ = "Workstation Dark";
    }
    
    compileAndApplyQss();
}

QStringList ThemeManager::availableThemes() const
{
    return themes_.keys();
}

void ThemeManager::selectTheme(const QString &name)
{
    if (!themes_.contains(name) || currentThemeName_ == name) return;
    
    currentThemeName_ = name;
    
    // Save to settings
    QSettings settings("Invenesis", "MergeApp");
    settings.setValue("themePreset", name);
    
    // Recompile QSS for Widgets
    compileAndApplyQss();
    
    // Notify dynamic QML bindings
    emit themeChanged();
}

QString ThemeManager::currentTheme() const
{
    return currentThemeName_;
}

// Getters for dynamic color properties bound to QML
bool ThemeManager::isDark() const
{
    return themes_[currentThemeName_].isDark;
}

QColor ThemeManager::backgroundMain() const
{
    return themes_[currentThemeName_].backgroundMain;
}

QColor ThemeManager::backgroundPanel() const
{
    return themes_[currentThemeName_].backgroundPanel;
}

QColor ThemeManager::backgroundPanel2() const
{
    return themes_[currentThemeName_].backgroundPanel2;
}

QColor ThemeManager::borderCard() const
{
    return themes_[currentThemeName_].borderCard;
}

QColor ThemeManager::divider() const
{
    return themes_[currentThemeName_].divider;
}

QColor ThemeManager::textPrimary() const
{
    return themes_[currentThemeName_].textPrimary;
}

QColor ThemeManager::textSecondary() const
{
    return themes_[currentThemeName_].textSecondary;
}

QColor ThemeManager::brandAccent() const
{
    return themes_[currentThemeName_].brandAccent;
}

QColor ThemeManager::brandAccent2() const
{
    return themes_[currentThemeName_].brandAccent2;
}

QColor ThemeManager::statusOk() const
{
    return themes_[currentThemeName_].statusOk;
}

QColor ThemeManager::statusWarn() const
{
    return themes_[currentThemeName_].statusWarn;
}

QColor ThemeManager::statusBad() const
{
    return themes_[currentThemeName_].statusBad;
}


QColor ThemeManager::surfaceHeader() const { return themes_[currentThemeName_].surfaceHeader; }
QColor ThemeManager::surfaceInput() const { return themes_[currentThemeName_].surfaceInput; }
QColor ThemeManager::surfaceHover() const { return themes_[currentThemeName_].surfaceHover; }
QColor ThemeManager::surfaceActive() const { return themes_[currentThemeName_].surfaceActive; }
QColor ThemeManager::borderStrong() const { return themes_[currentThemeName_].borderStrong; }
QColor ThemeManager::edge() const { return themes_[currentThemeName_].edge; }
QColor ThemeManager::textMuted() const { return themes_[currentThemeName_].textMuted; }
QColor ThemeManager::textDisabled() const { return themes_[currentThemeName_].textDisabled; }

QString ThemeManager::density() const { return density_; }
void ThemeManager::setDensity(const QString &density) {
    if (density_ == density) return;
    density_ = density;
    emit themeChanged();
}

void ThemeManager::setupPresets()
{
    // Teal Accent rgb(47, 179, 166) -> #2fb3a6
    // Darkest
    ThemeColors darkest;
    darkest.isDark = true;
    darkest.backgroundMain = QColor("#1e1e1e");
    darkest.backgroundPanel = QColor("#252525");
    darkest.backgroundPanel2 = QColor("#2f2f2f");
    darkest.surfaceHeader = QColor("#1a1a1a");
    darkest.surfaceInput = QColor("#141414");
    darkest.surfaceHover = QColor("#333333");
    darkest.surfaceActive = QColor("#3a3a3a");
    darkest.borderCard = QColor("#121212");
    darkest.divider = QColor("#121212");
    darkest.borderStrong = QColor("#0d0d0d");
    darkest.edge = QColor("#3a3a3a");
    darkest.textPrimary = QColor("#d2d2d2");
    darkest.textSecondary = QColor("#9a9a9a");
    darkest.textMuted = QColor("#6a6a6a");
    darkest.textDisabled = QColor("#4e4e4e");
    darkest.brandAccent = QColor("#2fb3a6");
    darkest.brandAccent2 = QColor("#259689");
    darkest.statusOk = QColor("#4fb06a");
    darkest.statusWarn = QColor("#d9a441");
    darkest.statusBad = QColor("#d95757");
    themes_.insert("Workstation Darkest", darkest);

    // Dark
    ThemeColors dark;
    dark.isDark = true;
    dark.backgroundMain = QColor("#2b2b2b");
    dark.backgroundPanel = QColor("#333333");
    dark.backgroundPanel2 = QColor("#3d3d3d");
    dark.surfaceHeader = QColor("#262626");
    dark.surfaceInput = QColor("#1c1c1c");
    dark.surfaceHover = QColor("#434343");
    dark.surfaceActive = QColor("#4a4a4a");
    dark.borderCard = QColor("#1c1c1c");
    dark.divider = QColor("#1c1c1c");
    dark.borderStrong = QColor("#161616");
    dark.edge = QColor("#4a4a4a");
    dark.textPrimary = QColor("#dddddd");
    dark.textSecondary = QColor("#a4a4a4");
    dark.textMuted = QColor("#727272");
    dark.textDisabled = QColor("#585858");
    dark.brandAccent = QColor("#2fb3a6");
    dark.brandAccent2 = QColor("#259689");
    dark.statusOk = QColor("#4fb06a");
    dark.statusWarn = QColor("#d9a441");
    dark.statusBad = QColor("#d95757");
    themes_.insert("Workstation Dark", dark);

    // Medium
    ThemeColors medium;
    medium.isDark = true;
    medium.backgroundMain = QColor("#525252");
    medium.backgroundPanel = QColor("#5b5b5b");
    medium.backgroundPanel2 = QColor("#666666");
    medium.surfaceHeader = QColor("#484848");
    medium.surfaceInput = QColor("#3a3a3a");
    medium.surfaceHover = QColor("#6b6b6b");
    medium.surfaceActive = QColor("#747474");
    medium.borderCard = QColor("#373737");
    medium.divider = QColor("#373737");
    medium.borderStrong = QColor("#2e2e2e");
    medium.edge = QColor("#6f6f6f");
    medium.textPrimary = QColor("#f2f2f2");
    medium.textSecondary = QColor("#cccccc");
    medium.textMuted = QColor("#a2a2a2");
    medium.textDisabled = QColor("#828282");
    medium.brandAccent = QColor("#2fb3a6");
    medium.brandAccent2 = QColor("#259689");
    medium.statusOk = QColor("#4fb06a");
    medium.statusWarn = QColor("#d9a441");
    medium.statusBad = QColor("#d95757");
    themes_.insert("Workstation Medium", medium);

    // Light
    ThemeColors light;
    light.isDark = false;
    light.backgroundMain = QColor("#bdbdbd");
    light.backgroundPanel = QColor("#c9c9c9");
    light.backgroundPanel2 = QColor("#d6d6d6");
    light.surfaceHeader = QColor("#b0b0b0");
    light.surfaceInput = QColor("#eef0f2");
    light.surfaceHover = QColor("#d0d0d0");
    light.surfaceActive = QColor("#c0c0c0");
    light.borderCard = QColor("#909090");
    light.divider = QColor("#909090");
    light.borderStrong = QColor("#7c7c7c");
    light.edge = QColor("#9a9a9a");
    light.textPrimary = QColor("#1c1c1c");
    light.textSecondary = QColor("#464646");
    light.textMuted = QColor("#6a6a6a");
    light.textDisabled = QColor("#9a9a9a");
    light.brandAccent = QColor("#2fb3a6");
    light.brandAccent2 = QColor("#259689");
    light.statusOk = QColor("#4fb06a");
    light.statusWarn = QColor("#d9a441");
    light.statusBad = QColor("#d95757");
    themes_.insert("Workstation Light", light);

    // Custom
    ThemeColors custom = dark;
    themes_.insert("Custom", custom);
}

void ThemeManager::compileAndApplyQss()
{
    if (!app_) return;
    
    QFile styleFile(":/styles/resources/style_template.qss");
    if (!styleFile.open(QFile::ReadOnly | QFile::Text)) {
        qWarning() << "Failed to read style_template.qss";
        return;
    }
    
    QString qss = QLatin1String(styleFile.readAll());
    styleFile.close();
    
    const ThemeColors &colors = themes_[currentThemeName_];
    
    // Replace custom tokens with hex strings
    qss.replace("{{backgroundMain}}", colors.backgroundMain.name());
    qss.replace("{{backgroundPanel}}", colors.backgroundPanel.name());
    qss.replace("{{backgroundPanel2}}", colors.backgroundPanel2.name());
    qss.replace("{{borderCard}}", colors.borderCard.name());
    qss.replace("{{divider}}", colors.divider.name());
    qss.replace("{{textPrimary}}", colors.textPrimary.name());
    qss.replace("{{textSecondary}}", colors.textSecondary.name());
    qss.replace("{{brandAccent}}", colors.brandAccent.name());
    qss.replace("{{brandAccent2}}", colors.brandAccent2.name());
    qss.replace("{{statusOk}}", colors.statusOk.name());
    qss.replace("{{statusWarn}}", colors.statusWarn.name());
    qss.replace("{{statusBad}}", colors.statusBad.name());
    
    app_->setStyleSheet(qss);
}

void ThemeManager::setCustomColor(const QString &tokenName, const QString &hexColor, bool applyQss)
{
    QColor color(hexColor);
    if (!color.isValid()) return;

    if (!themes_.contains("Custom")) {
        themes_.insert("Custom", themes_[currentThemeName_]);
    }

    ThemeColors &custom = themes_["Custom"];

    if (tokenName == "backgroundMain") custom.backgroundMain = color;
    else if (tokenName == "backgroundPanel") custom.backgroundPanel = color;
    else if (tokenName == "backgroundPanel2") custom.backgroundPanel2 = color;
    else if (tokenName == "borderCard") custom.borderCard = color;
    else if (tokenName == "divider") custom.divider = color;
    else if (tokenName == "textPrimary") custom.textPrimary = color;
    else if (tokenName == "textSecondary") custom.textSecondary = color;
    else if (tokenName == "brandAccent") custom.brandAccent = color;
    else if (tokenName == "brandAccent2") custom.brandAccent2 = color;
    else if (tokenName == "statusOk") custom.statusOk = color;
    else if (tokenName == "statusWarn") custom.statusWarn = color;
    else if (tokenName == "statusBad") custom.statusBad = color;
    else return;

    custom.isDark = (custom.backgroundMain.lightness() < 128);

    // Save dynamic token value to settings
    QSettings settings("Invenesis", "MergeApp");
    settings.setValue("custom_" + tokenName, hexColor);

    if (applyQss) {
        // Force selection of the custom theme to reload stylesheet & broadcast to QML
        selectTheme("Custom");
    } else {
        // Lightweight update: bypass heavy QWidgets style recomputations, just notify QML for real-time dragging!
        currentThemeName_ = "Custom";
        emit themeChanged();
    }
}

void ThemeManager::resetCustomThemeToPreset(const QString &presetName)
{
    if (!themes_.contains(presetName) || presetName == "Custom") return;

    themes_["Custom"] = themes_[presetName];
    const ThemeColors &src = themes_[presetName];

    QSettings settings("Invenesis", "MergeApp");
    settings.setValue("custom_backgroundMain", src.backgroundMain.name());
    settings.setValue("custom_backgroundPanel", src.backgroundPanel.name());
    settings.setValue("custom_backgroundPanel2", src.backgroundPanel2.name());
    settings.setValue("custom_borderCard", src.borderCard.name());
    settings.setValue("custom_divider", src.divider.name());
    settings.setValue("custom_textPrimary", src.textPrimary.name());
    settings.setValue("custom_textSecondary", src.textSecondary.name());
    settings.setValue("custom_brandAccent", src.brandAccent.name());
    settings.setValue("custom_brandAccent2", src.brandAccent2.name());
    settings.setValue("custom_statusOk", src.statusOk.name());
    settings.setValue("custom_statusWarn", src.statusWarn.name());
    settings.setValue("custom_statusBad", src.statusBad.name());

    selectTheme("Custom");
}

void ThemeManager::saveCustomTheme(const QString &presetName)
{
    if (presetName.isEmpty() || presetName == "Custom" || presetName == "Workstation Dark" ||
        presetName == "Workstation Light" || presetName == "Workstation Medium" || presetName == "Workstation Darkest") {
        return;
    }

    if (!themes_.contains("Custom")) return;

    // Clone colors to themes_
    themes_[presetName] = themes_["Custom"];
    const ThemeColors &src = themes_["Custom"];

    // Write to QSettings
    QSettings settings("Invenesis", "MergeApp");
    
    // Save color tokens
    settings.setValue("customtheme_" + presetName + "_backgroundMain", src.backgroundMain.name());
    settings.setValue("customtheme_" + presetName + "_backgroundPanel", src.backgroundPanel.name());
    settings.setValue("customtheme_" + presetName + "_backgroundPanel2", src.backgroundPanel2.name());
    settings.setValue("customtheme_" + presetName + "_borderCard", src.borderCard.name());
    settings.setValue("customtheme_" + presetName + "_divider", src.divider.name());
    settings.setValue("customtheme_" + presetName + "_textPrimary", src.textPrimary.name());
    settings.setValue("customtheme_" + presetName + "_textSecondary", src.textSecondary.name());
    settings.setValue("customtheme_" + presetName + "_brandAccent", src.brandAccent.name());
    settings.setValue("customtheme_" + presetName + "_brandAccent2", src.brandAccent2.name());
    settings.setValue("customtheme_" + presetName + "_statusOk", src.statusOk.name());
    settings.setValue("customtheme_" + presetName + "_statusWarn", src.statusWarn.name());
    settings.setValue("customtheme_" + presetName + "_statusBad", src.statusBad.name());

    // Update list of custom theme names
    QStringList customThemeNames = settings.value("customThemeNames").toStringList();
    if (!customThemeNames.contains(presetName)) {
        customThemeNames.append(presetName);
        settings.setValue("customThemeNames", customThemeNames);
    }

    // Apply the saved custom theme
    selectTheme(presetName);
}

void ThemeManager::deleteCustomTheme(const QString &presetName)
{
    if (presetName.isEmpty() || presetName == "Custom" || presetName == "Workstation Dark" ||
        presetName == "Workstation Light" || presetName == "Workstation Medium" || presetName == "Workstation Darkest") {
        return;
    }

    if (!themes_.contains(presetName)) return;

    // Remove from active map
    themes_.remove(presetName);

    // Remove from QSettings
    QSettings settings("Invenesis", "MergeApp");
    QStringList customThemeNames = settings.value("customThemeNames").toStringList();
    customThemeNames.removeAll(presetName);
    settings.setValue("customThemeNames", customThemeNames);

    // Clear individual token keys from settings
    settings.remove("customtheme_" + presetName + "_backgroundMain");
    settings.remove("customtheme_" + presetName + "_backgroundPanel");
    settings.remove("customtheme_" + presetName + "_backgroundPanel2");
    settings.remove("customtheme_" + presetName + "_borderCard");
    settings.remove("customtheme_" + presetName + "_divider");
    settings.remove("customtheme_" + presetName + "_textPrimary");
    settings.remove("customtheme_" + presetName + "_textSecondary");
    settings.remove("customtheme_" + presetName + "_brandAccent");
    settings.remove("customtheme_" + presetName + "_brandAccent2");
    settings.remove("customtheme_" + presetName + "_statusOk");
    settings.remove("customtheme_" + presetName + "_statusWarn");
    settings.remove("customtheme_" + presetName + "_statusBad");

    // Fall back to default theme
    selectTheme("Workstation Dark");
}
