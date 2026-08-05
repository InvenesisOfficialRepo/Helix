#pragma once

#include <QDate>
#include <QSqlDatabase>
#include <QVariantMap>
#include <QVector>

#include "domain/TecanPlannerDtos.h"

class TecanPlannerRepository
{
public:
    explicit TecanPlannerRepository(QSqlDatabase db);

    // Loads pending test requests scheduled for a specific date, grouped by (test_code, solvent, species, project_code)
    QVector<TecanAssayBatchDto> loadDailyAssayBatches(const QDate& date, QString* errorOut);

    // Quick summary count of scheduled pending batches and compounds for a specific date
    void loadDateSummary(const QDate& date, int* outBatchCount, int* outCompoundCount, QString* errorOut);

private:
    QSqlDatabase db_;
};
