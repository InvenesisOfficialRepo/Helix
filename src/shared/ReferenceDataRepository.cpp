#include "ReferenceDataRepository.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QVariant>
#include <QDebug>

ReferenceDataRepository::ReferenceDataRepository(const QSqlDatabase& db) : db_(db) {}

QJsonObject ReferenceDataRepository::getCatalogue() const {
    QJsonObject catalogue;
    if (!db_.isOpen()) return catalogue;

    QSqlQuery q(db_);
    q.prepare("SELECT test_id, concentration_unit, is_tarsal, vol_from_daughter_ul, total_well_vol_ul, well_area_m2 FROM test_catalogue");
    if (q.exec()) {
        while (q.next()) {
            QString testId = q.value("test_id").toString();
            QJsonObject testData;
            
            if (!q.isNull("concentration_unit")) {
                testData["concentration_unit"] = q.value("concentration_unit").toString();
            }
            if (!q.isNull("is_tarsal")) {
                testData["is_tarsal"] = q.value("is_tarsal").toBool();
            }
            if (!q.isNull("vol_from_daughter_ul")) {
                testData["vol_from_daughter_\u00b5l"] = q.value("vol_from_daughter_ul").toDouble();
            }
            if (!q.isNull("total_well_vol_ul")) {
                testData["total_well_vol_\u00b5l"] = q.value("total_well_vol_ul").toDouble();
            }
            if (!q.isNull("well_area_m2")) {
                testData["well_area_m2"] = q.value("well_area_m2").toDouble();
            }
            
            QJsonObject standardsObj;
            QSqlQuery qStd(db_);
            qStd.prepare("SELECT standard_name, top_dose_um, ec50_um, source FROM test_standards WHERE test_id = :testId");
            qStd.bindValue(":testId", testId);
            if (qStd.exec()) {
                while (qStd.next()) {
                    QString stdName = qStd.value("standard_name").toString();
                    QJsonObject stdData;
                    if (!qStd.isNull("top_dose_um")) stdData["top_dose_uM"] = qStd.value("top_dose_um").toDouble();
                    if (!qStd.isNull("ec50_um")) stdData["ec50_uM"] = qStd.value("ec50_um").toDouble();
                    if (!qStd.isNull("source")) stdData["source"] = qStd.value("source").toString();
                    standardsObj[stdName] = stdData;
                }
            }
            if (!standardsObj.isEmpty()) {
                testData["standards"] = standardsObj;
            }
            
            catalogue[testId] = testData;
        }
    } else {
        qWarning() << "Failed to fetch catalogue:" << q.lastError().text();
    }
    return catalogue;
}

QJsonObject ReferenceDataRepository::getQcPlates() const {
    QJsonObject qcPlates;
    if (!db_.isOpen()) return qcPlates;

    QSqlQuery q(db_);
    q.prepare("SELECT name, type, standard_name, concentration FROM qc_plates");
    if (q.exec()) {
        while (q.next()) {
            QString name = q.value("name").toString();
            QString type = q.value("type").toString();
            QString stdName = q.value("standard_name").toString();
            double conc = q.value("concentration").toDouble();
            
            QJsonObject nameObj = qcPlates.value(name).toObject();
            QJsonObject typeObj;
            typeObj["standard"] = stdName;
            typeObj["conc"] = conc;
            
            nameObj[type] = typeObj;
            qcPlates[name] = nameObj;
        }
    } else {
        qWarning() << "Failed to fetch qc_plates:" << q.lastError().text();
    }
    return qcPlates;
}

QJsonArray ReferenceDataRepository::getStandardsMatrix() const {
    QJsonArray matrix;
    if (!db_.isOpen()) return matrix;

    QSqlQuery q(db_);
    q.prepare("SELECT container_barcode, sample_alias, container_position, volume, volume_unit, concentration, concentration_unit, userdef_value_1, invenesis_solution_id FROM standards_matrix");
    if (q.exec()) {
        while (q.next()) {
            QJsonObject item;
            if (!q.isNull("container_barcode")) item["Containerbarcode"] = q.value("container_barcode").toString();
            if (!q.isNull("sample_alias")) item["Samplealias"] = q.value("sample_alias").toString();
            if (!q.isNull("container_position")) item["Containerposition"] = q.value("container_position").toString();
            if (!q.isNull("volume")) item["Volume"] = q.value("volume").toDouble();
            if (!q.isNull("volume_unit")) item["VolumeUnit"] = q.value("volume_unit").toString();
            if (!q.isNull("concentration")) item["Concentration"] = QString::number(q.value("concentration").toDouble());
            if (!q.isNull("concentration_unit")) item["ConcentrationUnit"] = q.value("concentration_unit").toString();
            if (!q.isNull("userdef_value_1")) item["UserdefValue1"] = q.value("userdef_value_1").toString();
            if (!q.isNull("invenesis_solution_id")) item["invenesis_solution_ID"] = q.value("invenesis_solution_id").toString();
            
            matrix.append(item);
        }
    } else {
        qWarning() << "Failed to fetch standards_matrix:" << q.lastError().text();
    }
    return matrix;
}

QJsonObject ReferenceDataRepository::getTestsVocabulary() const {
    QJsonObject vocab;
    if (!db_.isOpen()) return vocab;

    QSqlQuery q(db_);
    q.prepare("SELECT test_id, plate_size FROM tests_vocabulary");
    if (q.exec()) {
        while (q.next()) {
            QString testId = q.value("test_id").toString();
            if (!q.isNull("plate_size")) {
                vocab[testId] = q.value("plate_size").toInt();
            }
        }
    } else {
        qWarning() << "Failed to fetch tests_vocabulary:" << q.lastError().text();
    }
    return vocab;
}
