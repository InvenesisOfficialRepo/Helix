#pragma once

#include <QDialog>
#include <QVariantMap>
#include <QString>
#include <QComboBox>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QLabel>
#include <QTabWidget>
#include <QPushButton>
#include <QDialogButtonBox>

class RegisterCompoundDialog : public QDialog {
    Q_OBJECT

public:
    explicit RegisterCompoundDialog(const QVariantMap& compoundData, QWidget* parent = nullptr);
    ~RegisterCompoundDialog() override = default;

private slots:
    void onFormatChanged(int index);
    void onSubmit();

private:
    void setupUI();
    void parseAdditionalNotes();
    bool saveAsSolution(QString* errOut);
    bool saveAsPowder(QString* errOut);
    bool updateTrackingStatus(QString* errOut);

    // Initial compound database fields
    QVariantMap m_compoundData;
    QString m_compoundName;
    QString m_trackedCompoundId;
    QString m_batchId;
    QString m_projectCode;
    
    // Extracted properties from notes
    double m_mw = 0.0;
    double m_purity = 100.0;
    double m_quantity = 0.0;
    QString m_quantityUnit = "mg";
    bool m_isSolutionFromNotes = false;
    double m_clientStockConc = 10.0;
    QString m_clientStockConcUnit = "mM";

    // Common widgets
    QComboBox* formatComboBox;
    QTabWidget* stackTabs;
    QWidget* solutionTabWidget;
    QWidget* powderTabWidget;
    QDialogButtonBox* buttonBox;

    // Solution Mode inputs
    QLineEdit* solCompoundNameEdit;
    QLineEdit* solInvenesisIdEdit;
    QLineEdit* solBarcodeEdit;
    QLineEdit* solPlateBarcodeEdit;
    QLineEdit* solWellCoordinateEdit;
    QComboBox* solSolventComboBox;
    QDoubleSpinBox* solVolumeSpinBox;
    QComboBox* solVolumeUnitComboBox;
    QDoubleSpinBox* solConcSpinBox;
    QComboBox* solConcUnitComboBox;

    // Powder Mode inputs
    QLineEdit* pwdCompoundNameEdit;
    QLineEdit* pwdInvenesisIdEdit;
    QLineEdit* pwdBarcodeEdit;
    QDoubleSpinBox* pwdWeightSpinBox;
    QComboBox* pwdWeightUnitComboBox;
    QDoubleSpinBox* pwdMwSpinBox;
    QDoubleSpinBox* pwdPuritySpinBox;
    QComboBox* pwdStorageComboBox;
    QLineEdit* pwdLocationLabEdit;
    QLineEdit* pwdLocationShelfEdit;
};
