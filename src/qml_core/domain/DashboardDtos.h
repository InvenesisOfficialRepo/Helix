#pragma once

#include <QDate>
#include <QDateTime>
#include <QMetaType>
#include <QString>
#include <QVector>

#include "Status.h"

struct BatchDto {
    QString batchId;          // uuid as string
    QString projectCode;
    QString testCode;
    Status status = Status::Pending;
    QDate scheduledFor;       // invalid if NULL
    QDateTime createdAt;      // tz timestamp from DB
    int pendingCount = 0;
    int sentCount = 0;
    bool hasReport = false;
};

struct TestCardDto {
    QString testCode;
    int pendingCount = 0;
    int sentCount = 0;
    QVector<BatchDto> batches;
};

Q_DECLARE_METATYPE(BatchDto)
Q_DECLARE_METATYPE(QVector<BatchDto>)
Q_DECLARE_METATYPE(TestCardDto)
Q_DECLARE_METATYPE(QVector<TestCardDto>)
