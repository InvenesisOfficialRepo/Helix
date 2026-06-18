#pragma once

#include <QDate>
#include <QDateTime>
#include <QMetaType>
#include <QString>
#include <QVector>
#include <QVariantMap>

#include "domain/Status.h"

struct CompoundDto {
    QString trackedCompoundId;     // uuid string
    QString compoundName;
    Status status = Status::Pending;
    QDate scheduledFor;            // can be invalid
    QString robotRequestId;        // can be empty
    bool hasSolution = false;
    int numberOfDilutions = 3;
    int numberOfReplicates = 3;
};

struct BatchDetailDto {
    QString batchId;               // uuid string
    QString projectCode;
    QString testCode;
    Status status = Status::Pending;
    QDate scheduledFor;            // batch scheduled_for
    QDateTime createdAt;
    QVector<CompoundDto> compounds;
    QString createdBy;
    bool hasReport = false;
    QVariantMap analysisResults;
};

Q_DECLARE_METATYPE(CompoundDto)
Q_DECLARE_METATYPE(QVector<CompoundDto>)
Q_DECLARE_METATYPE(BatchDetailDto)
