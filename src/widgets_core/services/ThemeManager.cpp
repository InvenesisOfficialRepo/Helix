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
    , currentThemeName_("Clinical Dark")
{
    setupPresets();
}

void ThemeManager::initialize(QApplication* app)
{
    app_ = app;
    
    // Load last saved preset theme
    QSettings settings("Invenesis", "MergeApp");
    QString savedThemeName = settings.value("themePreset", "Clinical Dark").toString();
    
    if (themes_.contains(savedThemeName)) {
        currentThemeName_ = savedThemeName;
    } else {
        currentThemeName_ = "Clinical Dark";
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

void ThemeManager::setupPresets()
{
    // 1. Clinical Dark (Default Slate/Navy theme)
    ThemeColors dark;
    dark.isDark = true;
    dark.backgroundMain = QColor("#0B1220");
    dark.backgroundPanel = QColor("#0F172A");
    dark.backgroundPanel2 = QColor("#111C33");
    dark.borderCard = QColor("#324466");
    dark.divider = QColor("#1E2A41");
    dark.textPrimary = QColor("#E7EEF8");
    dark.textSecondary = QColor("#909EB0");
    dark.brandAccent = QColor("#30afb3");
    dark.brandAccent2 = QColor("#394861");
    dark.statusOk = QColor("#98c417");
    dark.statusWarn = QColor("#E5A100");
    dark.statusBad = QColor("#D64545");
    themes_.insert("Clinical Dark", dark);

    // 2. Swiss Light (Clean White/Gray theme)
    ThemeColors light;
    light.isDark = false;
    light.backgroundMain = QColor("#F2F0EF");
    light.backgroundPanel = QColor("#F8FAFC");
    light.backgroundPanel2 = QColor("#F1F5F9");
    light.borderCard = QColor("#D6DCE6");
    light.divider = QColor("#E2E8F0");
    light.textPrimary = QColor("#212934");
    light.textSecondary = QColor("#5D6A7E");
    light.brandAccent = QColor("#30afb3");
    light.brandAccent2 = QColor("#394861");
    light.statusOk = QColor("#98c417");
    light.statusWarn = QColor("#E5A100");
    light.statusBad = QColor("#D64545");
    themes_.insert("Swiss Light", light);

    // 3. Cyberpunk Neon (Cool Deep Violet/Magenta theme)
    ThemeColors cyberpunk;
    cyberpunk.isDark = true;
    cyberpunk.backgroundMain = QColor("#0F081D");
    cyberpunk.backgroundPanel = QColor("#1A0B2E");
    cyberpunk.backgroundPanel2 = QColor("#251240");
    cyberpunk.borderCard = QColor("#D946EF");
    cyberpunk.divider = QColor("#A21CAF");
    cyberpunk.textPrimary = QColor("#FDF4FF");
    cyberpunk.textSecondary = QColor("#D8B4FE");
    cyberpunk.brandAccent = QColor("#F43F5E");
    cyberpunk.brandAccent2 = QColor("#A21CAF");
    cyberpunk.statusOk = QColor("#10B981");
    cyberpunk.statusWarn = QColor("#F59E0B");
    cyberpunk.statusBad = QColor("#EF4444");
    themes_.insert("Cyberpunk Neon", cyberpunk);

    // 4. Forest Laboratory (Clean Forest/Mint theme)
    ThemeColors forest;
    forest.isDark = true;
    forest.backgroundMain = QColor("#0B1E19");
    forest.backgroundPanel = QColor("#0F2922");
    forest.backgroundPanel2 = QColor("#163D33");
    forest.borderCard = QColor("#206E5B");
    forest.divider = QColor("#1A4C40");
    forest.textPrimary = QColor("#F0FDF4");
    forest.textSecondary = QColor("#A7F3D0");
    forest.brandAccent = QColor("#10B981");
    forest.brandAccent2 = QColor("#206E5B");
    forest.statusOk = QColor("#34D399");
    forest.statusWarn = QColor("#F59E0B");
    forest.statusBad = QColor("#F87171");
    themes_.insert("Forest Laboratory", forest);

    // 5. Custom theme (User personalized)
    ThemeColors custom;
    QSettings settings("Invenesis", "MergeApp");
    custom.backgroundMain = QColor(settings.value("custom_backgroundMain", "#0B1220").toString());
    custom.backgroundPanel = QColor(settings.value("custom_backgroundPanel", "#0F172A").toString());
    custom.backgroundPanel2 = QColor(settings.value("custom_backgroundPanel2", "#111C33").toString());
    custom.borderCard = QColor(settings.value("custom_borderCard", "#324466").toString());
    custom.divider = QColor(settings.value("custom_divider", "#1E2A41").toString());
    custom.textPrimary = QColor(settings.value("custom_textPrimary", "#E7EEF8").toString());
    custom.textSecondary = QColor(settings.value("custom_textSecondary", "#909EB0").toString());
    custom.brandAccent = QColor(settings.value("custom_brandAccent", "#30afb3").toString());
    custom.brandAccent2 = QColor(settings.value("custom_brandAccent2", "#394861").toString());
    custom.statusOk = QColor(settings.value("custom_statusOk", "#98c417").toString());
    custom.statusWarn = QColor(settings.value("custom_statusWarn", "#E5A100").toString());
    custom.statusBad = QColor(settings.value("custom_statusBad", "#D64545").toString());
    custom.isDark = (custom.backgroundMain.lightness() < 128);
    themes_.insert("Custom", custom);

    // 6. Load user saved custom named themes
    QStringList customThemeNames = settings.value("customThemeNames").toStringList();
    for (const QString &name : customThemeNames) {
        ThemeColors c;
        c.backgroundMain = QColor(settings.value("customtheme_" + name + "_backgroundMain", "#0B1220").toString());
        c.backgroundPanel = QColor(settings.value("customtheme_" + name + "_backgroundPanel", "#0F172A").toString());
        c.backgroundPanel2 = QColor(settings.value("customtheme_" + name + "_backgroundPanel2", "#111C33").toString());
        c.borderCard = QColor(settings.value("customtheme_" + name + "_borderCard", "#324466").toString());
        c.divider = QColor(settings.value("customtheme_" + name + "_divider", "#1E2A41").toString());
        c.textPrimary = QColor(settings.value("customtheme_" + name + "_textPrimary", "#E7EEF8").toString());
        c.textSecondary = QColor(settings.value("customtheme_" + name + "_textSecondary", "#909EB0").toString());
        c.brandAccent = QColor(settings.value("customtheme_" + name + "_brandAccent", "#30afb3").toString());
        c.brandAccent2 = QColor(settings.value("customtheme_" + name + "_brandAccent2", "#394861").toString());
        c.statusOk = QColor(settings.value("customtheme_" + name + "_statusOk", "#98c417").toString());
        c.statusWarn = QColor(settings.value("customtheme_" + name + "_statusWarn", "#E5A100").toString());
        c.statusBad = QColor(settings.value("customtheme_" + name + "_statusBad", "#D64545").toString());
        c.isDark = (c.backgroundMain.lightness() < 128);
        themes_.insert(name, c);
    }
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
    if (presetName.isEmpty() || presetName == "Custom" || presetName == "Clinical Dark" ||
        presetName == "Swiss Light" || presetName == "Cyberpunk Neon" || presetName == "Forest Laboratory") {
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
    if (presetName.isEmpty() || presetName == "Custom" || presetName == "Clinical Dark" ||
        presetName == "Swiss Light" || presetName == "Cyberpunk Neon" || presetName == "Forest Laboratory") {
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
    selectTheme("Clinical Dark");
}
