#pragma once

#include <QDialog>
#include <QVariantMap>
#include <QString>
#include <QComboBox>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QDateEdit>
#include <QLabel>
#include <QDialogButtonBox>

class DissolveBottleDialog : public QDialog {
    Q_OBJECT

public:
    explicit DissolveBottleDialog(const QVariantMap& bottleData, QWidget* parent = nullptr);
    ~DissolveBottleDialog() override = default;

private slots:
    void onInputsChanged();
    void onSubmit();

private:
    void setupUI();
    void parseBottleData();
    bool saveAsSolution(QString* errOut);

    QVariantMap m_bottleData;
    QString m_invenesisBottleId;
    QString m_compoundName;
    double m_mw = 0.0;
    double m_purity = 99.0;
    double m_receivedAmount = 0.0;
    QString m_amountUnit = "mg";
    QString m_projectCode;
    double m_calculatedVolume = 0.0;

    // GUI widgets
    QLineEdit* compoundNameEdit;
    QLineEdit* bottleBarcodeEdit;
    QDoubleSpinBox* quantitySpinBox;
    QLineEdit* quantityUnitEdit;
    QDoubleSpinBox* mwSpinBox;
    QDoubleSpinBox* puritySpinBox;

    QLineEdit* solBarcodeEdit;
    QLineEdit* solPlateBarcodeEdit;
    QLineEdit* solWellCoordinateEdit;
    QComboBox* solSolventComboBox;
    QDoubleSpinBox* solConcSpinBox;
    QComboBox* solConcUnitComboBox;
    QDateEdit* solPrepDateEdit;
    QDateEdit* solExpDateEdit;
    QLineEdit* solPreparedByEdit;

    QLabel* calculationLabel;
    QDialogButtonBox* buttonBox;
};
