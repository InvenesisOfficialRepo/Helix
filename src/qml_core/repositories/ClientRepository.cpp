#include "ClientRepository.h"
#include "db/DbConfig.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QVariant>
#include <QtConcurrent>
#include <QUuid>
#include <QDebug>

// Helper to execute DB operations concurrently
template <typename T, typename Func>
QFuture<T> executeDb(Func func, T defaultReturn) {
    return QtConcurrent::run([func, defaultReturn]() -> T {
        DbConfig cfg;
        QString err;
        QString connName = "client_repo_" + QUuid::createUuid().toString();
        if (loadDbConfigFromEnv(cfg, err)) {
            if (DatabaseManager::openConnection(cfg, connName, &err)) {
                T result;
                {
                    QSqlDatabase db = DatabaseManager::db(connName);
                    result = func(db);
                }
                DatabaseManager::closeConnection(connName);
                return result;
            } else {
                qWarning() << "ClientRepository DB Open Error:" << err;
            }
        } else {
            qWarning() << "ClientRepository DB Config Error:" << err;
        }
        return defaultReturn;
    });
}

ClientRepository::ClientRepository(QObject *parent)
    : QObject(parent)
{
}

QFuture<QVariantMap> ClientRepository::fetchAnalyticsStats(const QString& clientCode) {
    return executeDb<QVariantMap>([clientCode](QSqlDatabase& db) {
        QVariantMap stats;
        stats["activeContracts"] = 0;
        stats["totalMonoplicatesPool"] = 0;
        stats["totalMonoplicatesConsumed"] = 0;
        
        QSqlQuery query(db);
        if (clientCode.isEmpty()) {
            if (query.exec("SELECT COUNT(*) FROM clients WHERE is_annualized = true")) {
                if (query.next()) {
                    stats["activeContracts"] = query.value(0).toInt();
                }
            }
            
            if (query.exec("SELECT SUM(total_datapoints), SUM(used_datapoints) FROM clients")) {
                if (query.next()) {
                    stats["totalMonoplicatesPool"] = query.value(0).toInt();
                    stats["totalMonoplicatesConsumed"] = query.value(1).toInt();
                }
            }
            
            QVariantList clientConsumption;
            if (query.exec("SELECT company_name, used_datapoints FROM clients WHERE used_datapoints > 0 ORDER BY used_datapoints DESC")) {
                while (query.next()) {
                    QVariantMap item;
                    item["name"] = query.value(0).toString();
                    item["value"] = query.value(1).toInt();
                    clientConsumption.append(item);
                }
            }
            stats["clientConsumptionStats"] = clientConsumption;
            
            QVariantList assayTypeStats;
            if (query.exec("SELECT assay_type, COUNT(*) FROM tracked_test_compounds WHERE assay_type IS NOT NULL GROUP BY assay_type")) {
                while (query.next()) {
                    QVariantMap item;
                    item["name"] = query.value(0).toString();
                    item["value"] = query.value(1).toInt();
                    assayTypeStats.append(item);
                }
            }
            stats["assayTypeStats"] = assayTypeStats;

            QVariantList testCategoryStats;
            if (query.exec("SELECT tv.assay_category, COUNT(*) "
                           "FROM tracked_test_compounds c "
                           "JOIN tracked_test_batches b ON c.batch_id = b.batch_id "
                           "JOIN tests_vocabulary tv ON b.test_code = tv.test_id "
                           "WHERE tv.assay_category IS NOT NULL "
                           "GROUP BY tv.assay_category")) {
                while (query.next()) {
                    QVariantMap item;
                    item["name"] = query.value(0).toString();
                    item["value"] = query.value(1).toInt();
                    testCategoryStats.append(item);
                }
            }
            stats["testCategoryStats"] = testCategoryStats;
        } else {
            // Filter by specific client
            query.prepare("SELECT is_annualized, total_datapoints, used_datapoints FROM clients WHERE client_code = :client_code");
            query.bindValue(":client_code", clientCode);
            if (query.exec() && query.next()) {
                stats["activeContracts"] = query.value(0).toBool() ? 1 : 0;
                stats["totalMonoplicatesPool"] = query.value(1).toInt();
                stats["totalMonoplicatesConsumed"] = query.value(2).toInt();
                
                QVariantList consumption;
                QVariantMap usedItem;
                usedItem["name"] = "Consumed";
                usedItem["value"] = stats["totalMonoplicatesConsumed"].toInt();
                consumption.append(usedItem);
                
                QVariantMap remainingItem;
                remainingItem["name"] = "Remaining Pool";
                int remaining = qMax(0, stats["totalMonoplicatesPool"].toInt() - stats["totalMonoplicatesConsumed"].toInt());
                remainingItem["value"] = remaining;
                consumption.append(remainingItem);
                
                stats["clientConsumptionStats"] = consumption;
            }
            
            QVariantList assayTypeStats;
            query.prepare("SELECT c.assay_type, COUNT(*) "
                          "FROM tracked_test_compounds c "
                          "JOIN tracked_test_batches b ON c.batch_id = b.batch_id "
                          "JOIN projects p ON b.project_code = p.project_code "
                          "WHERE c.assay_type IS NOT NULL AND p.client_code = :client_code "
                          "GROUP BY c.assay_type");
            query.bindValue(":client_code", clientCode);
            if (query.exec()) {
                while (query.next()) {
                    QVariantMap item;
                    item["name"] = query.value(0).toString();
                    item["value"] = query.value(1).toInt();
                    assayTypeStats.append(item);
                }
            }
            stats["assayTypeStats"] = assayTypeStats;

            QVariantList testCategoryStats;
            query.prepare("SELECT tv.assay_category, COUNT(*) "
                          "FROM tracked_test_compounds c "
                          "JOIN tracked_test_batches b ON c.batch_id = b.batch_id "
                          "JOIN projects p ON b.project_code = p.project_code "
                          "JOIN tests_vocabulary tv ON b.test_code = tv.test_id "
                          "WHERE p.client_code = :client_code AND tv.assay_category IS NOT NULL "
                          "GROUP BY tv.assay_category");
            query.bindValue(":client_code", clientCode);
            if (query.exec()) {
                while (query.next()) {
                    QVariantMap item;
                    item["name"] = query.value(0).toString();
                    item["value"] = query.value(1).toInt();
                    testCategoryStats.append(item);
                }
            }
            stats["testCategoryStats"] = testCategoryStats;
        }
        
        return stats;
    }, QVariantMap());
}

QFuture<QVector<domain::ClientDto>> ClientRepository::fetchAllClients() {
    return executeDb<QVector<domain::ClientDto>>([](QSqlDatabase& db) {
        QVector<domain::ClientDto> results;
        QSqlQuery query(db);
        query.prepare("SELECT client_code, company_name, created_at, is_annualized, total_datapoints, used_datapoints "
                      "FROM clients ORDER BY company_name");
        
        if (query.exec()) {
            while (query.next()) {
                domain::ClientDto dto;
                dto.clientCode = query.value("client_code").toString();
                dto.companyName = query.value("company_name").toString();
                dto.createdAt = query.value("created_at").toDateTime();
                dto.isAnnualized = query.value("is_annualized").toBool();
                dto.totalDatapoints = query.value("total_datapoints").toInt();
                dto.usedDatapoints = query.value("used_datapoints").toInt();
                results.append(dto);
            }
        } else {
            qWarning() << "Failed to fetch clients:" << query.lastError().text();
        }
        return results;
    }, QVector<domain::ClientDto>());
}

QFuture<bool> ClientRepository::updateClientAnnualization(const QString& clientCode, bool isAnnualized) {
    return executeDb<bool>([clientCode, isAnnualized](QSqlDatabase& db) {
        QSqlQuery query(db);
        query.prepare("UPDATE clients SET is_annualized = :is_annualized WHERE client_code = :client_code");
        query.bindValue(":is_annualized", isAnnualized);
        query.bindValue(":client_code", clientCode);
        
        bool success = query.exec();
        if (!success) {
            qWarning() << "Failed to update client annualization:" << query.lastError().text();
        }
        return success;
    }, false);
}

QFuture<bool> ClientRepository::updateClientDatapoints(const QString& clientCode, int totalDatapoints, int usedDatapoints) {
    return executeDb<bool>([clientCode, totalDatapoints, usedDatapoints](QSqlDatabase& db) {
        QSqlQuery query(db);
        query.prepare("UPDATE clients SET total_datapoints = :total, used_datapoints = :used WHERE client_code = :client_code");
        query.bindValue(":total", totalDatapoints);
        query.bindValue(":used", usedDatapoints);
        query.bindValue(":client_code", clientCode);
        
        bool success = query.exec();
        if (!success) {
            qWarning() << "Failed to update client datapoints:" << query.lastError().text();
        }
        return success;
    }, false);
}

QFuture<QVector<domain::ClientPricingOverrideDto>> ClientRepository::fetchPricingOverrides(const QString& clientCode) {
    return executeDb<QVector<domain::ClientPricingOverrideDto>>([clientCode](QSqlDatabase& db) {
        QVector<domain::ClientPricingOverrideDto> results;
        QSqlQuery query(db);
        query.prepare("SELECT id, client_code, test_id, ratio_primary, ratio_secondary, testset_fee, min_per_request "
                      "FROM client_custom_pricing WHERE client_code = :client_code");
        query.bindValue(":client_code", clientCode);
        
        if (query.exec()) {
            while (query.next()) {
                domain::ClientPricingOverrideDto dto;
                dto.id = query.value("id").toInt();
                dto.clientCode = query.value("client_code").toString();
                dto.testId = query.value("test_id").toString();
                dto.ratioPrimary = query.value("ratio_primary").toDouble();
                dto.ratioSecondary = query.value("ratio_secondary").toDouble();
                dto.testsetFee = query.value("testset_fee").toInt();
                dto.minPerRequest = query.value("min_per_request").toInt();
                results.append(dto);
            }
        } else {
            qWarning() << "Failed to fetch pricing overrides:" << query.lastError().text();
        }
        return results;
    }, QVector<domain::ClientPricingOverrideDto>());
}

QFuture<bool> ClientRepository::savePricingOverride(const domain::ClientPricingOverrideDto& dto) {
    return executeDb<bool>([dto](QSqlDatabase& db) {
        QSqlQuery query(db);
        if (dto.id > 0) {
            query.prepare("UPDATE client_custom_pricing SET test_id = :test_id, ratio_primary = :ratio_primary, "
                          "ratio_secondary = :ratio_secondary, testset_fee = :testset_fee, min_per_request = :min_per_request "
                          "WHERE id = :id");
            query.bindValue(":id", dto.id);
        } else {
            query.prepare("INSERT INTO client_custom_pricing (client_code, test_id, ratio_primary, ratio_secondary, testset_fee, min_per_request) "
                          "VALUES (:client_code, :test_id, :ratio_primary, :ratio_secondary, :testset_fee, :min_per_request) "
                          "ON CONFLICT (client_code, test_id) DO UPDATE SET "
                          "ratio_primary = EXCLUDED.ratio_primary, ratio_secondary = EXCLUDED.ratio_secondary, "
                          "testset_fee = EXCLUDED.testset_fee, min_per_request = EXCLUDED.min_per_request");
            query.bindValue(":client_code", dto.clientCode);
        }
        
        query.bindValue(":test_id", dto.testId);
        query.bindValue(":ratio_primary", dto.ratioPrimary);
        query.bindValue(":ratio_secondary", dto.ratioSecondary);
        query.bindValue(":testset_fee", dto.testsetFee);
        query.bindValue(":min_per_request", dto.minPerRequest);
        
        bool success = query.exec();
        if (!success) {
            qWarning() << "Failed to save pricing override:" << query.lastError().text();
        }
        return success;
    }, false);
}

QFuture<bool> ClientRepository::deletePricingOverride(int id) {
    return executeDb<bool>([id](QSqlDatabase& db) {
        QSqlQuery query(db);
        query.prepare("DELETE FROM client_custom_pricing WHERE id = :id");
        query.bindValue(":id", id);
        
        bool success = query.exec();
        if (!success) {
            qWarning() << "Failed to delete pricing override:" << query.lastError().text();
        }
        return success;
    }, false);
}

