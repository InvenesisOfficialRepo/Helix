#pragma once

#include <QObject>
#include <QColor>
#include <QStringList>
#include <QMap>
#include <QApplication>

class ThemeManager : public QObject
{
    Q_OBJECT
    
    Q_PROPERTY(bool isDark READ isDark NOTIFY themeChanged)
    Q_PROPERTY(QColor backgroundMain READ backgroundMain NOTIFY themeChanged)
    Q_PROPERTY(QColor backgroundPanel READ backgroundPanel NOTIFY themeChanged)
    Q_PROPERTY(QColor backgroundPanel2 READ backgroundPanel2 NOTIFY themeChanged)
    Q_PROPERTY(QColor borderCard READ borderCard NOTIFY themeChanged)
    Q_PROPERTY(QColor divider READ divider NOTIFY themeChanged)
    Q_PROPERTY(QColor textPrimary READ textPrimary NOTIFY themeChanged)
    Q_PROPERTY(QColor textSecondary READ textSecondary NOTIFY themeChanged)
    Q_PROPERTY(QColor brandAccent READ brandAccent NOTIFY themeChanged)
    Q_PROPERTY(QColor brandAccent2 READ brandAccent2 NOTIFY themeChanged)
    Q_PROPERTY(QColor statusOk READ statusOk NOTIFY themeChanged)
    Q_PROPERTY(QColor statusWarn READ statusWarn NOTIFY themeChanged)
    Q_PROPERTY(QColor statusBad READ statusBad NOTIFY themeChanged)
    Q_PROPERTY(QStringList availableThemes READ availableThemes NOTIFY themeChanged)
    Q_PROPERTY(QString currentTheme READ currentTheme NOTIFY themeChanged)

public:
    static ThemeManager* instance();

    void initialize(QApplication* app);

    Q_INVOKABLE QStringList availableThemes() const;
    Q_INVOKABLE void selectTheme(const QString &name);
    Q_INVOKABLE QString currentTheme() const;
    Q_INVOKABLE void setCustomColor(const QString &tokenName, const QString &hexColor, bool applyQss = true);
    Q_INVOKABLE void resetCustomThemeToPreset(const QString &presetName);
    Q_INVOKABLE void saveCustomTheme(const QString &presetName);
    Q_INVOKABLE void deleteCustomTheme(const QString &presetName);

    // Getters for dynamic properties
    bool isDark() const;
    QColor backgroundMain() const;
    QColor backgroundPanel() const;
    QColor backgroundPanel2() const;
    QColor borderCard() const;
    QColor divider() const;
    QColor textPrimary() const;
    QColor textSecondary() const;
    QColor brandAccent() const;
    QColor brandAccent2() const;
    QColor statusOk() const;
    QColor statusWarn() const;
    QColor statusBad() const;

signals:
    void themeChanged();

private:
    explicit ThemeManager(QObject *parent = nullptr);
    ~ThemeManager() override = default;

    QApplication* app_ = nullptr;
    QString currentThemeName_;
    
    struct ThemeColors {
        bool isDark;
        QColor backgroundMain;
        QColor backgroundPanel;
        QColor backgroundPanel2;
        QColor borderCard;
        QColor divider;
        QColor textPrimary;
        QColor textSecondary;
        QColor brandAccent;
        QColor brandAccent2;
        QColor statusOk;
        QColor statusWarn;
        QColor statusBad;
    };

    QMap<QString, ThemeColors> themes_;
    void setupPresets();
    void compileAndApplyQss();
};
