#include "dissolvebottledialog.h"
#include "utils/UserSessionHelper.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QDate>

DissolveBottleDialog::DissolveBottleDialog(const QVariantMap& bottleData, QWidget* parent)
    : QDialog(parent), m_bottleData(bottleData)
{
    parseBottleData();
    setupUI();

    setWindowTitle(tr("Dissolve Bottle - %1").arg(m_compoundName));
    resize(480, 560);

    // Initial calculations
    onInputsChanged();

    // Set initial focus to solution barcode
    solBarcodeEdit->setFocus();
}

void DissolveBottleDialog::parseBottleData()
{
    m_compoundName = m_bottleData.value("product_name").toString().trimmed().toUpper();
    m_invenesisBottleId = m_bottleData.value("invenesis_bottle_id").toString();
    m_mw = m_bottleData.value("molecular_weight").toDouble();
    m_receivedAmount = m_bottleData.value("received_amount").toDouble();
    m_amountUnit = m_bottleData.value("amount_unit").toString().trimmed();
    if (m_amountUnit.isEmpty()) {
        m_amountUnit = "mg";
    }

    m_purity = m_bottleData.value("purity").toDouble();
    if (m_purity <= 0.0) {
        m_purity = 99.0; // Default to 99% if unknown or invalid
    }

    m_projectCode = m_bottleData.value("project_code").toString();
}

void DissolveBottleDialog::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);

    // --- Part 1: Bottle Physical Properties (Inputs/Read-only) ---
    auto* bottleGroup = new QGroupBox(tr("Bottle Properties"), this);
    auto* bottleLayout = new QFormLayout(bottleGroup);

    compoundNameEdit = new QLineEdit(m_compoundName, this);
    compoundNameEdit->setReadOnly(true);
    compoundNameEdit->setStyleSheet("background-color: #f0f0f0; font-weight: bold;");
    bottleLayout->addRow(tr("Compound Name/ID:"), compoundNameEdit);

    bottleBarcodeEdit = new QLineEdit(m_invenesisBottleId, this);
    bottleBarcodeEdit->setReadOnly(true);
    bottleBarcodeEdit->setStyleSheet("background-color: #f0f0f0;");
    bottleLayout->addRow(tr("Vial Barcode:"), bottleBarcodeEdit);

    // Editable molecular weight
    mwSpinBox = new QDoubleSpinBox(this);
    mwSpinBox->setRange(0.0, 10000.0);
    mwSpinBox->setDecimals(2);
    mwSpinBox->setValue(m_mw);
    bottleLayout->addRow(tr("Molecular Weight (g/mol):"), mwSpinBox);

    // Editable purity
    puritySpinBox = new QDoubleSpinBox(this);
    puritySpinBox->setRange(0.0, 100.0);
    puritySpinBox->setDecimals(2);
    puritySpinBox->setValue(m_purity);
    bottleLayout->addRow(tr("Purity (%):"), puritySpinBox);

    // Quantity input
    auto* wtLayout = new QHBoxLayout();
    quantitySpinBox = new QDoubleSpinBox(this);
    quantitySpinBox->setRange(0.0, 1000000.0);
    quantitySpinBox->setDecimals(3);
    quantitySpinBox->setValue(m_receivedAmount);
    quantityUnitEdit = new QLineEdit(m_amountUnit, this);
    quantityUnitEdit->setReadOnly(true);
    quantityUnitEdit->setMaximumWidth(60);
    quantityUnitEdit->setStyleSheet("background-color: #f0f0f0;");
    wtLayout->addWidget(quantitySpinBox);
    wtLayout->addWidget(quantityUnitEdit);
    bottleLayout->addRow(tr("Dry Quantity:"), wtLayout);

    mainLayout->addWidget(bottleGroup);

    // --- Part 2: Target Solution Parameters & Plating ---
    auto* solGroup = new QGroupBox(tr("Target Solution & Coordinates"), this);
    auto* solLayout = new QFormLayout(solGroup);

    solBarcodeEdit = new QLineEdit(this);
    solBarcodeEdit->setPlaceholderText(tr("Scan or enter new solution barcode"));
    solLayout->addRow(tr("Solution Barcode:"), solBarcodeEdit);

    solPlateBarcodeEdit = new QLineEdit(this);
    solPlateBarcodeEdit->setPlaceholderText(tr("Scan or enter target rack barcode"));
    solLayout->addRow(tr("Plate / Rack Barcode:"), solPlateBarcodeEdit);

    solWellCoordinateEdit = new QLineEdit(this);
    solWellCoordinateEdit->setPlaceholderText(tr("e.g. A01, H12"));
    solLayout->addRow(tr("Well Coordinate:"), solWellCoordinateEdit);

    solSolventComboBox = new QComboBox(this);
    solSolventComboBox->addItems(QStringList() << "DMSO" << "Water" << "EtOH" << "MeOH");
    solLayout->addRow(tr("Solvent:"), solSolventComboBox);

    // Desired Stock Concentration
    auto* concLayout = new QHBoxLayout();
    solConcSpinBox = new QDoubleSpinBox(this);
    solConcSpinBox->setRange(0.0, 10000.0);
    solConcSpinBox->setDecimals(3);
    solConcSpinBox->setValue(10.0); // 10 mM is laboratory standard
    solConcUnitComboBox = new QComboBox(this);
    solConcUnitComboBox->addItems(QStringList() << "mM" << "uM" << "nM" << "M");
    solConcUnitComboBox->setCurrentText("mM");
    concLayout->addWidget(solConcSpinBox);
    concLayout->addWidget(solConcUnitComboBox);
    solLayout->addRow(tr("Target Concentration:"), concLayout);

    solPrepDateEdit = new QDateEdit(QDate::currentDate(), this);
    solPrepDateEdit->setCalendarPopup(true);
    solPrepDateEdit->setDisplayFormat("yyyy-MM-dd");
    solLayout->addRow(tr("Preparation Date:"), solPrepDateEdit);

    solExpDateEdit = new QDateEdit(QDate::currentDate().addYears(1), this);
    solExpDateEdit->setCalendarPopup(true);
    solExpDateEdit->setDisplayFormat("yyyy-MM-dd");
    solLayout->addRow(tr("Expiration Date:"), solExpDateEdit);

    solPreparedByEdit = new QLineEdit(SessionUtils::getCurrentUsername(), this);
    solLayout->addRow(tr("Prepared By:"), solPreparedByEdit);

    mainLayout->addWidget(solGroup);

    // --- Part 3: Calculation & Instructions Banner ---
    calculationLabel = new QLabel(this);
    calculationLabel->setWordWrap(true);
    calculationLabel->setMargin(6);
    calculationLabel->setFrameShape(QFrame::StyledPanel);
    calculationLabel->setFrameShadow(QFrame::Sunken);
    calculationLabel->setMinimumHeight(44);
    mainLayout->addWidget(calculationLabel);

    // Dialog Button Box (Ok / Cancel)
    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mainLayout->addWidget(buttonBox);

    // Connections
    connect(mwSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &DissolveBottleDialog::onInputsChanged);
    connect(puritySpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &DissolveBottleDialog::onInputsChanged);
    connect(quantitySpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &DissolveBottleDialog::onInputsChanged);
    connect(solSolventComboBox, &QComboBox::currentTextChanged, this, &DissolveBottleDialog::onInputsChanged);
    connect(solConcSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &DissolveBottleDialog::onInputsChanged);
    connect(solConcUnitComboBox, &QComboBox::currentTextChanged, this, &DissolveBottleDialog::onInputsChanged);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &DissolveBottleDialog::onSubmit);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void DissolveBottleDialog::onInputsChanged()
{
    double mg = quantitySpinBox->value();
    QString unit = quantityUnitEdit->text().trimmed().toLower();
    if (unit == "g") {
        mg *= 1000.0;
    }

    double mw = mwSpinBox->value();
    double purity = puritySpinBox->value();
    if (purity <= 0.0) {
        purity = 99.0;
    }

    double desiredConc = solConcSpinBox->value();
    QString concUnit = solConcUnitComboBox->currentText().trimmed().toLower();
    
    // Standardize desired concentration to mM:
    double desiredConc_mM = desiredConc;
    if (concUnit == "um") {
        desiredConc_mM = desiredConc / 1000.0;
    } else if (concUnit == "nm") {
        desiredConc_mM = desiredConc / 1000000.0;
    } else if (concUnit == "m") {
        desiredConc_mM = desiredConc * 1000.0;
    }

    if (mw > 0.0 && desiredConc_mM > 0.0) {
        // Scientific Volume: V (µL) = (mg * purity * 10000.0) / (mw * desiredConc_mM)
        m_calculatedVolume = (mg * purity * 10000.0) / (mw * desiredConc_mM);
    } else {
        m_calculatedVolume = 0.0;
    }

    QString solvent = solSolventComboBox->currentText();
    if (m_calculatedVolume > 0.0) {
        calculationLabel->setText(tr("Required Volume: Add <b>%1</b> µL of <b>%2</b> to achieve a <b>%3 %4</b> stock solution.")
                                  .arg(QString::number(m_calculatedVolume, 'f', 1))
                                  .arg(solvent)
                                  .arg(QString::number(desiredConc))
                                  .arg(solConcUnitComboBox->currentText()));
        calculationLabel->setStyleSheet("color: #30afb3; font-weight: bold; font-size: 10.5pt; background-color: #e8f7f8; border: 1px solid #c2eef0;");
    } else {
        calculationLabel->setText(tr("Invalid inputs for volume calculation. Please verify dry quantity and Molecular Weight."));
        calculationLabel->setStyleSheet("color: #c0392b; font-weight: bold; background-color: #fce4d6; border: 1px solid #f8cbad;");
    }
}

void DissolveBottleDialog::onSubmit()
{
    if (solBarcodeEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("Validation Error"), tr("Please scan or enter the physical barcode for the new solution."));
        solBarcodeEdit->setFocus();
        return;
    }
    if (solPlateBarcodeEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("Validation Error"), tr("Please enter a target Plate / Rack ID."));
        solPlateBarcodeEdit->setFocus();
        return;
    }
    if (solWellCoordinateEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("Validation Error"), tr("Please enter a target Well Coordinate."));
        solWellCoordinateEdit->setFocus();
        return;
    }
    if (m_calculatedVolume <= 0.0) {
        QMessageBox::warning(this, tr("Validation Error"), tr("Cannot dissolve: Calculated solvent volume is invalid. Please check MW and dry quantity parameters."));
        return;
    }

    QString err;
    if (saveAsSolution(&err)) {
        QMessageBox::information(this, tr("Success"), tr("Vial dissolved successfully!\nAdded %1 µL of %2.\nSolution registered into laboratory inventory.")
                                 .arg(QString::number(m_calculatedVolume, 'f', 1))
                                 .arg(solSolventComboBox->currentText()));
        accept();
    } else {
        QMessageBox::critical(this, tr("Database Error"), tr("Failed to register solution details to inventory:\n%1").arg(err));
    }
}

bool DissolveBottleDialog::saveAsSolution(QString* errOut)
{
    QSqlQuery query;
    query.prepare(
        "INSERT INTO public.solutions ("
        "    invenesis_solution_id, product_name, concentration, concentration_unit, solvent, "
        "    molecular_weight, quantity, quantity_unit, purity, solvent_volume, "
        "    container_id, well_id, matrix_tube_id, project_code, preparation_date, expiration_date, prepared_by"
        ") VALUES ("
        "    :invenesis_solution_id, :product_name, :concentration, :concentration_unit, :solvent, "
        "    :molecular_weight, :quantity, :quantity_unit, :purity, :solvent_volume, "
        "    :container_id, :well_id, :matrix_tube_id, :project_code, :preparation_date, :expiration_date, :prepared_by"
        ")"
    );

    query.bindValue(":invenesis_solution_id", solBarcodeEdit->text().trimmed());
    query.bindValue(":product_name", m_compoundName);
    query.bindValue(":concentration", solConcSpinBox->value());
    query.bindValue(":concentration_unit", solConcUnitComboBox->currentText());
    query.bindValue(":solvent", solSolventComboBox->currentText());
    query.bindValue(":molecular_weight", mwSpinBox->value() > 0.0 ? QVariant(mwSpinBox->value()) : QVariant(QVariant::Double));
    query.bindValue(":quantity", m_calculatedVolume);
    query.bindValue(":quantity_unit", "uL");
    query.bindValue(":purity", puritySpinBox->value() > 0.0 ? QVariant(puritySpinBox->value()) : QVariant(QVariant::Double));
    query.bindValue(":solvent_volume", m_calculatedVolume);
    query.bindValue(":container_id", solPlateBarcodeEdit->text().trimmed().toUpper());
    query.bindValue(":well_id", solWellCoordinateEdit->text().trimmed().toUpper());
    query.bindValue(":matrix_tube_id", solBarcodeEdit->text().trimmed());
    query.bindValue(":project_code", m_projectCode.isEmpty() ? QVariant(QVariant::String) : m_projectCode);
    query.bindValue(":preparation_date", solPrepDateEdit->date());
    query.bindValue(":expiration_date", solExpDateEdit->date());

    QString prepBy = solPreparedByEdit->text().trimmed();
    if (prepBy.isEmpty()) {
        prepBy = SessionUtils::getCurrentUsername();
    }
    query.bindValue(":prepared_by", prepBy);

    if (!query.exec()) {
        if (errOut) *errOut = query.lastError().text();
        return false;
    }
    return true;
}
