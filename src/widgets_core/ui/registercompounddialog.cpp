#include "registercompounddialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>
#include <QRegularExpression>
#include <QTabBar>
#include <QDebug>

RegisterCompoundDialog::RegisterCompoundDialog(const QVariantMap& compoundData, QWidget* parent)
    : QDialog(parent), m_compoundData(compoundData)
{
    m_compoundName = m_compoundData.value("compound_name").toString().trimmed().toUpper();
    m_trackedCompoundId = m_compoundData.value("tracked_compound_id").toString();
    m_batchId = m_compoundData.value("batch_id").toString();
    m_projectCode = m_compoundData.value("project_code").toString();

    parseAdditionalNotes();
    setupUI();

    setWindowTitle(tr("Register Compound - %1").arg(m_compoundName));
    resize(480, 500);

    // Set initial focus based on detected format
    if (m_isSolutionFromNotes) {
        formatComboBox->setCurrentIndex(0);
        solInvenesisIdEdit->setFocus();
    } else {
        formatComboBox->setCurrentIndex(1);
        pwdInvenesisIdEdit->setFocus();
    }
}

void RegisterCompoundDialog::parseAdditionalNotes()
{
    QString notes = m_compoundData.value("additional_notes").toString();
    
    // 1. Detect if it is a Solution or Powder
    if (notes.contains("Form: Solution", Qt::CaseInsensitive)) {
        m_isSolutionFromNotes = true;
    } else if (notes.contains("Form: Powder", Qt::CaseInsensitive)) {
        m_isSolutionFromNotes = false;
    } else {
        // Fallback checks
        m_isSolutionFromNotes = m_compoundData.value("stock_concentration").toDouble() > 0;
    }

    // 2. Parse molecular weight (e.g. MW: 350.5 g/mol)
    QRegularExpression mwRegex("MW:\\s*([0-9\\.]+)");
    auto mwMatch = mwRegex.match(notes);
    if (mwMatch.hasMatch()) {
        m_mw = mwMatch.captured(1).toDouble();
    }

    // 3. Parse purity (e.g. Purity: 98.5%)
    QRegularExpression purityRegex("Purity:\\s*([0-9\\.]+)");
    auto purityMatch = purityRegex.match(notes);
    if (purityMatch.hasMatch()) {
        m_purity = purityMatch.captured(1).toDouble();
    }

    // 4. Parse quantity and unit (e.g. Quantity: 150 uL or Quantity: 5 mg)
    QRegularExpression qtyRegex("Quantity:\\s*([0-9\\.]+)\\s*(\\w+)");
    auto qtyMatch = qtyRegex.match(notes);
    if (qtyMatch.hasMatch()) {
        m_quantity = qtyMatch.captured(1).toDouble();
        m_quantityUnit = qtyMatch.captured(2);
    } else {
        m_quantity = 0.0;
        m_quantityUnit = m_isSolutionFromNotes ? "uL" : "mg";
    }

    // 5. Parse client-submitted stock concentration
    m_clientStockConc = m_compoundData.value("stock_concentration").toDouble();
    m_clientStockConcUnit = m_compoundData.value("stock_concentration_unit").toString().trimmed();
    if (m_clientStockConc <= 0) {
        // Try parsing from notes (e.g. Concentration: 10 mM)
        QRegularExpression concRegex("Concentration:\\s*([0-9\\.]+)");
        auto concMatch = concRegex.match(notes);
        if (concMatch.hasMatch()) {
            m_clientStockConc = concMatch.captured(1).toDouble();
        } else {
            m_clientStockConc = 10.0; // standard fallback
        }
    }
    if (m_clientStockConcUnit.isEmpty()) {
        m_clientStockConcUnit = "mM";
    } else if (m_clientStockConcUnit.compare("uM", Qt::CaseInsensitive) == 0 ||
               m_clientStockConcUnit.compare("µM", Qt::CaseInsensitive) == 0 ||
               m_clientStockConcUnit.compare("μM", Qt::CaseInsensitive) == 0) {
        m_clientStockConcUnit = QString::fromUtf8("\xC2\xB5M");
    }
}

void RegisterCompoundDialog::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);

    // Format Selector
    auto* selectorLayout = new QFormLayout();
    formatComboBox = new QComboBox(this);
    formatComboBox->addItem(tr("Liquid Solution (Plate Well)"), 0);
    formatComboBox->addItem(tr("Dry Powder (Inventory Bottle)"), 1);
    selectorLayout->addRow(tr("Received Format:"), formatComboBox);
    mainLayout->addLayout(selectorLayout);

    // Tab Widget for forms
    stackTabs = new QTabWidget(this);
    stackTabs->tabBar()->hide(); // Hide tabs to force selection through formatComboBox
    mainLayout->addWidget(stackTabs);

    // --- Tab 1: Solution Form ---
    solutionTabWidget = new QWidget(stackTabs);
    auto* solLayout = new QFormLayout(solutionTabWidget);

    solCompoundNameEdit = new QLineEdit(m_compoundName, solutionTabWidget);
    solCompoundNameEdit->setReadOnly(true);
    solCompoundNameEdit->setStyleSheet("background-color: #f0f0f0; font-weight: bold;");
    solLayout->addRow(tr("Compound ID:"), solCompoundNameEdit);

    solInvenesisIdEdit = new QLineEdit(solutionTabWidget);
    solInvenesisIdEdit->setPlaceholderText(tr("Scan or enter Invenesis Solution ID"));
    solLayout->addRow(tr("Invenesis Solution ID:"), solInvenesisIdEdit);

    solBarcodeEdit = new QLineEdit(solutionTabWidget);
    solBarcodeEdit->setPlaceholderText(tr("Scan or enter matrix tube barcode"));
    solLayout->addRow(tr("Physical Barcode:"), solBarcodeEdit);

    solPlateBarcodeEdit = new QLineEdit(solutionTabWidget);
    solPlateBarcodeEdit->setPlaceholderText(tr("Scan or enter rack/plate ID"));
    solLayout->addRow(tr("Plate / Rack ID:"), solPlateBarcodeEdit);

    solWellCoordinateEdit = new QLineEdit(solutionTabWidget);
    solWellCoordinateEdit->setPlaceholderText(tr("e.g. A01, B05"));
    solLayout->addRow(tr("Well Coordinate:"), solWellCoordinateEdit);

    solSolventComboBox = new QComboBox(solutionTabWidget);
    solSolventComboBox->addItems(QStringList() << "DMSO" << "Water" << "EtOH" << "MeOH");
    solLayout->addRow(tr("Solvent:"), solSolventComboBox);

    // Volume input (with unit layout)
    auto* volLayout = new QHBoxLayout();
    solVolumeSpinBox = new QDoubleSpinBox(solutionTabWidget);
    solVolumeSpinBox->setRange(0.0, 1000000.0);
    solVolumeSpinBox->setDecimals(1);
    solVolumeSpinBox->setValue(m_isSolutionFromNotes ? m_quantity : 100.0);
    solVolumeUnitComboBox = new QComboBox(solutionTabWidget);
    solVolumeUnitComboBox->addItems(QStringList() << "uL" << "mL");
    solVolumeUnitComboBox->setCurrentText(m_isSolutionFromNotes ? m_quantityUnit : "uL");
    volLayout->addWidget(solVolumeSpinBox);
    volLayout->addWidget(solVolumeUnitComboBox);
    solLayout->addRow(tr("Solvent Volume:"), volLayout);

    // Concentration input (with unit layout)
    auto* concLayout = new QHBoxLayout();
    solConcSpinBox = new QDoubleSpinBox(solutionTabWidget);
    solConcSpinBox->setRange(0.0, 10000.0);
    solConcSpinBox->setDecimals(3);
    solConcSpinBox->setValue(m_clientStockConc);
    solConcUnitComboBox = new QComboBox(solutionTabWidget);
    solConcUnitComboBox->addItems(QStringList() << "mM" << QString::fromUtf8("\xC2\xB5M") << "nM" << "M" << "ppm");
    solConcUnitComboBox->setCurrentText(m_clientStockConcUnit);
    concLayout->addWidget(solConcSpinBox);
    concLayout->addWidget(solConcUnitComboBox);
    solLayout->addRow(tr("Stock Concentration:"), concLayout);

    stackTabs->addTab(solutionTabWidget, tr("Solution"));

    // --- Tab 2: Powder Form ---
    powderTabWidget = new QWidget(stackTabs);
    auto* pwdLayout = new QFormLayout(powderTabWidget);

    pwdCompoundNameEdit = new QLineEdit(m_compoundName, powderTabWidget);
    pwdCompoundNameEdit->setReadOnly(true);
    pwdCompoundNameEdit->setStyleSheet("background-color: #f0f0f0; font-weight: bold;");
    pwdLayout->addRow(tr("Compound ID:"), pwdCompoundNameEdit);

    pwdInvenesisIdEdit = new QLineEdit(powderTabWidget);
    pwdInvenesisIdEdit->setPlaceholderText(tr("Scan or enter Invenesis Bottle ID"));
    pwdLayout->addRow(tr("Invenesis Bottle ID:"), pwdInvenesisIdEdit);

    pwdBarcodeEdit = new QLineEdit(powderTabWidget);
    pwdBarcodeEdit->setPlaceholderText(tr("Scan or enter physical vial barcode"));
    pwdLayout->addRow(tr("Physical Barcode:"), pwdBarcodeEdit);

    // Quantity input
    auto* wtLayout = new QHBoxLayout();
    pwdQuantitySpinBox = new QDoubleSpinBox(powderTabWidget);
    pwdQuantitySpinBox->setRange(0.0, 1000000.0);
    pwdQuantitySpinBox->setDecimals(2);
    pwdQuantitySpinBox->setValue(!m_isSolutionFromNotes ? m_quantity : 5.0);
    pwdQuantityUnitComboBox = new QComboBox(powderTabWidget);
    pwdQuantityUnitComboBox->addItems(QStringList() << "mg" << "g");
    pwdQuantityUnitComboBox->setCurrentText(!m_isSolutionFromNotes ? m_quantityUnit : "mg");
    wtLayout->addWidget(pwdQuantitySpinBox);
    wtLayout->addWidget(pwdQuantityUnitComboBox);
    pwdLayout->addRow(tr("Received Quantity:"), wtLayout);

    pwdMwSpinBox = new QDoubleSpinBox(powderTabWidget);
    pwdMwSpinBox->setRange(0.0, 10000.0);
    pwdMwSpinBox->setDecimals(2);
    pwdMwSpinBox->setValue(m_mw);
    pwdLayout->addRow(tr("Molecular Weight (g/mol):"), pwdMwSpinBox);

    pwdPuritySpinBox = new QDoubleSpinBox(powderTabWidget);
    pwdPuritySpinBox->setRange(0.0, 100.0);
    pwdPuritySpinBox->setDecimals(2);
    pwdPuritySpinBox->setValue(m_purity);
    pwdLayout->addRow(tr("Purity (%):"), pwdPuritySpinBox);

    pwdStorageComboBox = new QComboBox(powderTabWidget);
    pwdStorageComboBox->addItems(QStringList() << "RT" << "4°C" << "-20°C" << "-80°C");
    pwdStorageComboBox->setCurrentText("-20°C");
    pwdLayout->addRow(tr("Storage Temperature:"), pwdStorageComboBox);

    pwdLocationLabEdit = new QLineEdit(powderTabWidget);
    pwdLocationLabEdit->setPlaceholderText(tr("e.g. Lab 1, Cabinet A"));
    pwdLayout->addRow(tr("Location Room/Cabinet:"), pwdLocationLabEdit);

    pwdLocationShelfEdit = new QLineEdit(powderTabWidget);
    pwdLocationShelfEdit->setPlaceholderText(tr("e.g. Shelf 2, Box B"));
    pwdLayout->addRow(tr("Location Shelf:"), pwdLocationShelfEdit);

    stackTabs->addTab(powderTabWidget, tr("Powder"));

    // Connect combo selector to tab switcher
    connect(formatComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &RegisterCompoundDialog::onFormatChanged);

    // Dialog Button Box (Submit / Cancel)
    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mainLayout->addWidget(buttonBox);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &RegisterCompoundDialog::onSubmit);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void RegisterCompoundDialog::onFormatChanged(int index)
{
    stackTabs->setCurrentIndex(index);
    if (index == 0) {
        solInvenesisIdEdit->setFocus();
    } else {
        pwdInvenesisIdEdit->setFocus();
    }
}

void RegisterCompoundDialog::onSubmit()
{
    QString err;
    bool success = false;

    // Check which format mode is active
    if (formatComboBox->currentIndex() == 0) {
        // Solution Mode
        if (solInvenesisIdEdit->text().trimmed().isEmpty()) {
            QMessageBox::warning(this, tr("Validation Error"), tr("Please scan or enter the Invenesis Solution ID."));
            solInvenesisIdEdit->setFocus();
            return;
        }
        if (solBarcodeEdit->text().trimmed().isEmpty()) {
            QMessageBox::warning(this, tr("Validation Error"), tr("Please scan or enter the Physical Barcode."));
            solBarcodeEdit->setFocus();
            return;
        }
        if (solPlateBarcodeEdit->text().trimmed().isEmpty()) {
            QMessageBox::warning(this, tr("Validation Error"), tr("Please enter a Plate / Rack ID."));
            solPlateBarcodeEdit->setFocus();
            return;
        }
        if (solWellCoordinateEdit->text().trimmed().isEmpty()) {
            QMessageBox::warning(this, tr("Validation Error"), tr("Please enter the Well Coordinate."));
            solWellCoordinateEdit->setFocus();
            return;
        }

        success = saveAsSolution(&err);
    } else {
        // Powder Mode
        if (pwdInvenesisIdEdit->text().trimmed().isEmpty()) {
            QMessageBox::warning(this, tr("Validation Error"), tr("Please scan or enter the Invenesis Bottle ID."));
            pwdInvenesisIdEdit->setFocus();
            return;
        }
        if (pwdBarcodeEdit->text().trimmed().isEmpty()) {
            QMessageBox::warning(this, tr("Validation Error"), tr("Please scan or enter the physical bottle Barcode."));
            pwdBarcodeEdit->setFocus();
            return;
        }

        success = saveAsPowder(&err);
    }

    if (success) {
        if (updateTrackingStatus(&err)) {
            QMessageBox::information(this, tr("Success"), tr("Compound registered successfully into laboratory inventory!"));
            accept();
        } else {
            QMessageBox::critical(this, tr("Database Error"), tr("Compound saved to inventory, but failed to update tracking queue status:\n%1").arg(err));
        }
    } else {
        QMessageBox::critical(this, tr("Database Error"), tr("Failed to save compound details to inventory:\n%1").arg(err));
    }
}

bool RegisterCompoundDialog::saveAsSolution(QString* errOut)
{
    QSqlQuery query;
    query.prepare(
        "INSERT INTO public.solutions ("
        "    invenesis_solution_id, product_name, concentration, concentration_unit, solvent, "
        "    molecular_weight, quantity, quantity_unit, purity, solvent_volume, "
        "    container_id, well_id, matrix_tube_id, project_code, preparation_date"
        ") VALUES ("
        "    :invenesis_solution_id, :product_name, :concentration, :concentration_unit, :solvent, "
        "    :molecular_weight, :quantity, :quantity_unit, :purity, :solvent_volume, "
        "    :container_id, :well_id, :matrix_tube_id, :project_code, CURRENT_DATE"
        ")"
    );

    double mw_val = pwdMwSpinBox->value(); // share MW across modes to make it robust
    if (mw_val <= 0.0) {
        mw_val = m_mw;
    }

    double purity_val = pwdPuritySpinBox->value();
    if (purity_val <= 0.0) {
        purity_val = m_purity;
    }

    query.bindValue(":invenesis_solution_id", solInvenesisIdEdit->text().trimmed());
    query.bindValue(":product_name", m_compoundName);
    query.bindValue(":concentration", solConcSpinBox->value());
    query.bindValue(":concentration_unit", solConcUnitComboBox->currentText());
    query.bindValue(":solvent", solSolventComboBox->currentText());
    query.bindValue(":molecular_weight", mw_val > 0.0 ? QVariant(mw_val) : QVariant(QVariant::Double));
    query.bindValue(":quantity", QVariant(QVariant::Double)); // solutions don't have dry weight
    query.bindValue(":quantity_unit", "uL");
    query.bindValue(":purity", purity_val > 0.0 ? QVariant(purity_val) : QVariant(QVariant::Double));
    query.bindValue(":solvent_volume", solVolumeSpinBox->value());
    query.bindValue(":container_id", solPlateBarcodeEdit->text().trimmed().toUpper());
    query.bindValue(":well_id", solWellCoordinateEdit->text().trimmed().toUpper());
    query.bindValue(":matrix_tube_id", solBarcodeEdit->text().trimmed());
    query.bindValue(":project_code", m_projectCode.isEmpty() ? QVariant(QVariant::String) : m_projectCode);

    if (!query.exec()) {
        if (errOut) *errOut = query.lastError().text();
        return false;
    }
    return true;
}

bool RegisterCompoundDialog::saveAsPowder(QString* errOut)
{
    QSqlQuery query;
    query.prepare(
        "INSERT INTO public.bottles ("
        "    invenesis_bottle_id, product_name, molecular_weight, received_amount, amount_unit, "
        "    purity, storage, receival_date, location_lab, location_shelf, project_code, matrix_tube_id"
        ") VALUES ("
        "    :invenesis_bottle_id, :product_name, :molecular_weight, :received_amount, :amount_unit, "
        "    :purity, :storage, CURRENT_DATE, :location_lab, :location_shelf, :project_code, :matrix_tube_id"
        ")"
    );

    query.bindValue(":invenesis_bottle_id", pwdInvenesisIdEdit->text().trimmed());
    query.bindValue(":product_name", m_compoundName);
    query.bindValue(":molecular_weight", pwdMwSpinBox->value() > 0.0 ? QVariant(pwdMwSpinBox->value()) : QVariant(QVariant::Double));
    query.bindValue(":received_amount", pwdQuantitySpinBox->value());
    query.bindValue(":amount_unit", pwdQuantityUnitComboBox->currentText());
    query.bindValue(":purity", pwdPuritySpinBox->value() > 0.0 ? QVariant(pwdPuritySpinBox->value()) : QVariant(QVariant::Double));
    query.bindValue(":storage", pwdStorageComboBox->currentText());
    query.bindValue(":location_lab", pwdLocationLabEdit->text().trimmed().isEmpty() ? QVariant(QVariant::String) : pwdLocationLabEdit->text().trimmed());
    query.bindValue(":location_shelf", pwdLocationShelfEdit->text().trimmed().isEmpty() ? QVariant(QVariant::String) : pwdLocationShelfEdit->text().trimmed());
    query.bindValue(":project_code", m_projectCode.isEmpty() ? QVariant(QVariant::String) : m_projectCode);
    query.bindValue(":matrix_tube_id", pwdBarcodeEdit->text().trimmed());

    if (!query.exec()) {
        if (errOut) *errOut = query.lastError().text();
        return false;
    }
    return true;
}

bool RegisterCompoundDialog::updateTrackingStatus(QString* errOut)
{
    Q_UNUSED(errOut);
    // Keep compound status as 'pending' to allow QML app to send it later
    return true;
}
