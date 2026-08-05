#pragma once

#include <QString>
#include <QStringList>
#include <QDate>
#include <QVector>
#include <QMetaType>
#include <QVariantMap>

struct TecanAssayBatchDto {
    QString testCode;         // e.g. "INV-T-005"
    QString solvent;          // e.g. "DMSO"
    QString species;          // e.g. "H. contortus"
    QString projectCode;      // e.g. "PRJ-001"
    int compoundCount = 0;
    QStringList requestIds;
    QStringList compoundNames;

    QVariantMap toMap() const {
        QVariantMap map;
        map["testCode"] = testCode;
        map["solvent"] = solvent;
        map["species"] = species;
        map["projectCode"] = projectCode;
        map["compoundCount"] = compoundCount;
        map["requestIds"] = requestIds;
        map["compoundNames"] = compoundNames;
        return map;
    }
};

Q_DECLARE_METATYPE(TecanAssayBatchDto)
Q_DECLARE_METATYPE(QVector<TecanAssayBatchDto>)
