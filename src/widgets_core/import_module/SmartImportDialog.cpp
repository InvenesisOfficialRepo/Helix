#include "SmartImportDialog.h"
#include "ImportDataAccess.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QHeaderView>
#include <QLineEdit>

namespace import_module {

enum {
    Page_Selection = 0,
    Page_Mapping = 1,
    Page_Preview = 2
};

SmartImportDialog::SmartImportDialog(QWidget* parent) : QWizard(parent) {
    setWindowTitle(tr("Smart Inventory Importer"));
    setFixedSize(800, 600);
    setWizardStyle(QWizard::ModernStyle);

    setPage(Page_Selection, createSelectionPage());
    setPage(Page_Mapping, createMappingPage());
    setPage(Page_Preview, createPreviewPage());

    connect(this, &QWizard::currentIdChanged, this, [this](int id) {
        if (id == Page_Mapping) {
            initializeMappingPage();
        } else if (id == Page_Preview) {
            initializePreviewPage();
        }
    });
}

QWizardPage* SmartImportDialog::createSelectionPage() {
    QWizardPage* page = new QWizardPage;
    page->setTitle(tr("Select File and Target"));
    page->setSubTitle(tr("Choose an Excel (.xlsx) or CSV file, and select the inventory target."));

    QVBoxLayout* layout = new QVBoxLayout(page);

    QHBoxLayout* fileLayout = new QHBoxLayout;
    QPushButton* browseBtn = new QPushButton(tr("Browse..."));
    m_fileLabel = new QLabel(tr("No file selected."));
    fileLayout->addWidget(browseBtn);
    fileLayout->addWidget(m_fileLabel, 1);
    layout->addLayout(fileLayout);

    connect(browseBtn, &QPushButton::clicked, this, &SmartImportDialog::browseFile);

    QHBoxLayout* targetLayout = new QHBoxLayout;
    targetLayout->addWidget(new QLabel(tr("Target Table:")));
    m_tableTypeCombo = new QComboBox;
    m_tableTypeCombo->addItem(tr("Bottles"), "bottles");
    m_tableTypeCombo->addItem(tr("Solutions"), "solutions");
    targetLayout->addWidget(m_tableTypeCombo, 1);
    layout->addLayout(targetLayout);

    return page;
}

QWizardPage* SmartImportDialog::createMappingPage() {
    QWizardPage* page = new QWizardPage;
    page->setTitle(tr("Map Columns"));
    page->setSubTitle(tr("Review the auto-mapped columns and correct them if necessary."));

    QVBoxLayout* layout = new QVBoxLayout(page);
    
    m_mappingTable = new QTableWidget;
    m_mappingTable->setColumnCount(3);
    m_mappingTable->setHorizontalHeaderLabels({tr("Database Column"), tr("Requirement"), tr("Mapped File Header")});
    m_mappingTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_mappingTable->setSelectionMode(QAbstractItemView::NoSelection);
    
    layout->addWidget(m_mappingTable);

    return page;
}

QWizardPage* SmartImportDialog::createPreviewPage() {
    QWizardPage* page = new QWizardPage;
    page->setTitle(tr("Preview & Validate"));
    page->setSubTitle(tr("Review the data before committing it to the database."));

    QVBoxLayout* layout = new QVBoxLayout(page);
    
    m_validationLabel = new QLabel;
    m_validationLabel->setStyleSheet("color: red; font-weight: bold;");
    layout->addWidget(m_validationLabel);

    m_previewTable = new QTableWidget;
    m_previewTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    layout->addWidget(m_previewTable);

    return page;
}

void SmartImportDialog::browseFile() {
    QString path = QFileDialog::getOpenFileName(this, tr("Open Import File"), QString(), tr("Data Files (*.xlsx *.csv)"));
    if (!path.isEmpty()) {
        m_selectedFilePath = path;
        m_fileLabel->setText(QFileInfo(path).fileName());
    }
}

bool SmartImportDialog::validateCurrentPage() {
    if (currentId() == Page_Selection) {
        if (m_selectedFilePath.isEmpty()) {
            QMessageBox::warning(this, tr("No File Selected"), tr("Please select a data file to import before proceeding."));
            return false;
        }
    }
    return QWizard::validateCurrentPage();
}

void SmartImportDialog::initializeMappingPage() {
    QString target = m_tableTypeCombo->currentData().toString();
    if (target == "bottles") {
        m_currentSchema = SchemaFactory::createBottlesSchema();
    } else {
        m_currentSchema = SchemaFactory::createSolutionsSchema();
    }

    QString err;
    if (!m_importer.loadFile(m_selectedFilePath, &err)) {
        QMessageBox::critical(this, tr("Error loading file"), err);
        return;
    }

    QMap<QString, int> autoMap = m_importer.autoMapHeaders(m_currentSchema);
    const ParsedData& data = m_importer.getRawData();

    m_mappingTable->setRowCount(m_currentSchema.columns.size());
    m_mappingCombos.clear();

    QStringList comboOptions;
    comboOptions << tr("-- Do Not Import --");
    for (const QString& header : data.headers) {
        comboOptions << header;
    }

    int row = 0;
    for (const ImportColumn& col : m_currentSchema.columns) {
        QTableWidgetItem* dbItem = new QTableWidgetItem(col.displayName);
        dbItem->setFlags(Qt::ItemIsEnabled);
        m_mappingTable->setItem(row, 0, dbItem);

        QTableWidgetItem* reqItem = new QTableWidgetItem(col.isRequired ? tr("Required") : tr("Optional"));
        if (col.isRequired) reqItem->setForeground(QBrush(Qt::red));
        reqItem->setFlags(Qt::ItemIsEnabled);
        m_mappingTable->setItem(row, 1, reqItem);

        QComboBox* headerCombo = new QComboBox;
        headerCombo->addItems(comboOptions);
        
        int mappedIdx = autoMap.value(col.dbColumnName, -1);
        if (mappedIdx >= 0) {
            headerCombo->setCurrentIndex(mappedIdx + 1); // +1 because index 0 is "-- Do Not Import --"
        }
        
        m_mappingCombos[col.dbColumnName] = headerCombo;
        m_mappingTable->setCellWidget(row, 2, headerCombo);
        
        row++;
    }
}

void SmartImportDialog::initializePreviewPage() {
    m_validationLabel->clear();
    const ParsedData& data = m_importer.getRawData();
    
    // Validate mapping
    QStringList errors;
    QMap<QString, int> finalMapping;
    for (const ImportColumn& col : m_currentSchema.columns) {
        QComboBox* combo = m_mappingCombos.value(col.dbColumnName);
        if (!combo) continue;
        
        int idx = combo->currentIndex() - 1; // -1 to map back to data.headers index
        if (idx >= 0) {
            finalMapping[col.dbColumnName] = idx;
        } else if (col.isRequired) {
            errors << tr("Required column '%1' is not mapped.").arg(col.displayName);
        }
    }

    if (!errors.isEmpty()) {
        m_validationLabel->setText(errors.join("\n"));
        m_previewTable->clear();
        m_previewTable->setRowCount(0);
        m_previewTable->setColumnCount(0);
        button(QWizard::FinishButton)->setEnabled(false);
        return;
    }

    button(QWizard::FinishButton)->setEnabled(true);

    // Build preview table
    QStringList previewHeaders;
    QList<QString> dbColsMapped;
    for (const ImportColumn& col : m_currentSchema.columns) {
        if (finalMapping.contains(col.dbColumnName)) {
            previewHeaders << col.displayName;
            dbColsMapped << col.dbColumnName;
        }
    }

    m_previewTable->setColumnCount(previewHeaders.size());
    m_previewTable->setHorizontalHeaderLabels(previewHeaders);
    
    int previewRows = qMin(data.rows.size(), 20); // Show max 20 rows
    m_previewTable->setRowCount(previewRows);

    for (int r = 0; r < previewRows; ++r) {
        const QVariantList& rowData = data.rows[r];
        for (int c = 0; c < dbColsMapped.size(); ++c) {
            int fileColIdx = finalMapping[dbColsMapped[c]];
            QString val = fileColIdx < rowData.size() ? rowData[fileColIdx].toString() : QString();
            QTableWidgetItem* item = new QTableWidgetItem(val);
            m_previewTable->setItem(r, c, item);
        }
    }
    m_previewTable->resizeColumnsToContents();
}

void SmartImportDialog::accept() {
    // Collect mapped data
    const ParsedData& data = m_importer.getRawData();
    QList<QVariantMap> mappedRows;

    QMap<QString, int> finalMapping;
    for (const ImportColumn& col : m_currentSchema.columns) {
        QComboBox* combo = m_mappingCombos.value(col.dbColumnName);
        int idx = combo->currentIndex() - 1;
        if (idx >= 0) finalMapping[col.dbColumnName] = idx;
    }

    for (const QVariantList& rowData : data.rows) {
        QVariantMap rowMap;
        for (auto it = finalMapping.begin(); it != finalMapping.end(); ++it) {
            int fileColIdx = it.value();
            if (fileColIdx < rowData.size()) {
                rowMap[it.key()] = rowData[fileColIdx];
            }
        }
        mappedRows.append(rowMap);
    }

    ImportDataAccess dao;
    QString err;
    int insertedCount = dao.importData(m_currentSchema.tableName, mappedRows, &err);
    
    if (insertedCount >= 0) {
        QMessageBox::information(this, tr("Import Successful"), tr("Successfully imported %1 records.").arg(insertedCount));
        QWizard::accept();
    } else {
        QMessageBox::critical(this, tr("Import Failed"), err);
    }
}

} // namespace import_module
