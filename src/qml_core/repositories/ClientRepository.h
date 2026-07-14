#pragma once

#include <QObject>
#include <QVector>
#include <QString>
#include <QFuture>
#include <QVariantMap>
#include "domain/ClientDtos.h"
#include "db/DatabaseManager.h"

class ClientRepository : public QObject {
    Q_OBJECT
public:
    explicit ClientRepository(QObject *parent = nullptr);

    QFuture<QVector<domain::ClientDto>> fetchAllClients();
    QFuture<QVariantMap> fetchAnalyticsStats(const QString& clientCode = "");
    QFuture<bool> updateClientAnnualization(const QString& clientCode, bool isAnnualized);
    QFuture<bool> updateClientDatapoints(const QString& clientCode, int totalDatapoints, int usedDatapoints);
    
    QFuture<QVector<domain::ClientPricingOverrideDto>> fetchPricingOverrides(const QString& clientCode);
    QFuture<bool> savePricingOverride(const domain::ClientPricingOverrideDto& dto);
    QFuture<bool> deletePricingOverride(int id);

private:
};
