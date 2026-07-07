#include "standardselectiondialog.h"
#include "ui_standardselectiondialog.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QDebug>
#include <QSqlDatabase>
#include "../../shared/ReferenceDataRepository.h"

StandardSelectionDialog::StandardSelectionDialog(QWidget *parent)
    : QDialog(parent),
    ui(new Ui::StandardSelectionDialog)
{
    ui->setupUi(this);
    ui->comboBox->clear();
    loadStandardJson();

    // Show detail on selection
    connect(ui->comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &StandardSelectionDialog::displayStandardDetails);
}

StandardSelectionDialog::~StandardSelectionDialog()
{
    delete ui;
}

void StandardSelectionDialog::loadStandardJson()
{
    ReferenceDataRepository repo(QSqlDatabase::database());
    QJsonArray standards = repo.getStandardsMatrix();
    if (standards.isEmpty()) {
        qWarning() << "Failed to load standards from database.";
        ui->comboBox->addItem("Error loading standards");
        return;
    }
    for (const QJsonValue &val : standards) {
        QJsonObject obj = val.toObject();
        QString name = obj["Samplealias"].toString().trimmed();
        QString well = obj["Containerposition"].toString();
        QString conc = obj["Concentration"].toString();
        QString unit = obj["ConcentrationUnit"].toString();
        QString barcode = obj["Containerbarcode"].toString();

        if (name.isEmpty() || well.isEmpty() || conc.isEmpty())
            continue;

        QString label = QString("%1 – Well: %2 – %3 %4 – Barcode: %5")
                            .arg(name, well, conc, unit, barcode);

        ui->comboBox->addItem(label);
        standardObjects.append(obj);
    }

    if (!standardObjects.isEmpty())
        displayStandardDetails(0);
}

void StandardSelectionDialog::displayStandardDetails(int index)
{
    if (index >= 0 && index < standardObjects.size()) {
        const QJsonObject &selected = standardObjects.at(index);
        ui->textEdit->setPlainText(QJsonDocument(selected).toJson(QJsonDocument::Indented));
    }
}

QString StandardSelectionDialog::selectedStandard() const
{
    return ui->comboBox->currentText();
}

QString StandardSelectionDialog::notes() const
{
    return ui->textEdit->toPlainText();
}

QJsonObject StandardSelectionDialog::selectedStandardJson() const
{
    int idx = ui->comboBox->currentIndex();
    return (idx >= 0 && idx < standardObjects.size()) ? standardObjects[idx] : QJsonObject();
}

void StandardSelectionDialog::setAvailableStandards(const QStringList &standards)
{
    ui->comboBox->clear();
    ui->comboBox->addItems(standards);
}
