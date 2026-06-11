#pragma once

#include <QDate>
#include <QSqlDatabase>
#include <QString>
#include <QStringList>

#include "domain/BatchDetailDtos.h"

class BatchRepository
{
public:
    explicit BatchRepository(QSqlDatabase db);

    BatchDetailDto loadBatchDetail(const QString& batchId, QString* errorOut);

    bool sendSelectedCompounds(const QString& batchId,
                               const QStringList& trackedCompoundIds,
                               const QDate& scheduledFor,
                               QString* errorOut);

private:
    QSqlDatabase db_;
};
