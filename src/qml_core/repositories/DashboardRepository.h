#pragma once

#include <QSqlDatabase>
#include <QString>
#include <QVector>

#include "domain/DashboardDtos.h"

class DashboardRepository
{
public:
    explicit DashboardRepository(QSqlDatabase db);

    QVector<TestCardDto> loadDashboardSnapshot(QString* errorOut);
    QVector<TestCardDto> loadDoneDashboardSnapshot(QString* errorOut);
    QVector<TestCardDto> loadArchiveDashboardSnapshot(QString* errorOut);

private:
    QVector<BatchDto> loadBatchesForTest(const QString& testCode, QString* errorOut);
    QVector<BatchDto> loadDoneBatchesForTest(const QString& testCode, QString* errorOut);
    QVector<BatchDto> loadArchiveBatchesForTest(const QString& testCode, QString* errorOut);

    QSqlDatabase db_;
};
