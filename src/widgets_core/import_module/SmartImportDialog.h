#pragma once

#include <QWizard>
#include <QTableWidget>
#include <QComboBox>
#include <QLabel>
#include "SmartImporter.h"

namespace import_module {

class SmartImportDialog : public QWizard {
    Q_OBJECT
public:
    explicit SmartImportDialog(QWidget* parent = nullptr);
    ~SmartImportDialog() = default;

    void accept() override;
    bool validateCurrentPage() override;

private:
    SmartImporter m_importer;
    ImportTableSchema m_currentSchema;
    QString m_selectedFilePath;
    QMap<QString, QComboBox*> m_mappingCombos; // dbColumnName -> ComboBox

    // Pages
    QWizardPage* createSelectionPage();
    QWizardPage* createMappingPage();
    QWizardPage* createPreviewPage();

    // UI elements
    QComboBox* m_tableTypeCombo = nullptr;
    QLabel* m_fileLabel = nullptr;
    QTableWidget* m_mappingTable = nullptr;
    QTableWidget* m_previewTable = nullptr;
    QLabel* m_validationLabel = nullptr;

private slots:
    void browseFile();
    void initializeMappingPage();
    void initializePreviewPage();
};

} // namespace import_module
