#include "DatapointManagerViewModel.h"
#include <QDebug>

DatapointManagerViewModel::DatapointManagerViewModel(QObject *parent)
    : QObject(parent)
{
}

bool DatapointManagerViewModel::isLoading() const {
    return m_isLoading;
}

QVariantList DatapointManagerViewModel::clients() const {
    return m_clients;
}

QVariantList DatapointManagerViewModel::selectedClientPricing() const {
    return m_selectedClientPricing;
}

QString DatapointManagerViewModel::selectedAnalyticsClient() const {
    return m_selectedAnalyticsClient;
}

int DatapointManagerViewModel::activeContracts() const {
    return m_activeContracts;
}

int DatapointManagerViewModel::totalMonoplicatesPool() const {
    return m_totalMonoplicatesPool;
}

int DatapointManagerViewModel::totalMonoplicatesConsumed() const {
    return m_totalMonoplicatesConsumed;
}

QVariantList DatapointManagerViewModel::clientConsumptionStats() const {
    return m_clientConsumptionStats;
}

QVariantList DatapointManagerViewModel::assayTypeStats() const {
    return m_assayTypeStats;
}

QVariantList DatapointManagerViewModel::testCategoryStats() const {
    return m_testCategoryStats;
}


void DatapointManagerViewModel::setLoading(bool loading) {
    if (m_isLoading != loading) {
        m_isLoading = loading;
        emit isLoadingChanged();
    }
}

void DatapointManagerViewModel::loadClients() {
    setLoading(true);
    
    auto watcher = new QFutureWatcher<QVector<domain::ClientDto>>(this);
    connect(watcher, &QFutureWatcher<QVector<domain::ClientDto>>::finished, this, [this, watcher]() {
        QVector<domain::ClientDto> results = watcher->result();
        m_clients.clear();
        for (const auto& dto : results) {
            QVariantMap map;
            map["clientCode"] = dto.clientCode;
            map["companyName"] = dto.companyName;
            map["isAnnualized"] = dto.isAnnualized;
            map["totalDatapoints"] = dto.totalDatapoints;
            map["usedDatapoints"] = dto.usedDatapoints;
            map["createdAt"] = dto.createdAt;
            m_clients.append(map);
        }
        emit clientsChanged();
        setLoading(false);
        watcher->deleteLater();
    });
    
    watcher->setFuture(m_repository.fetchAllClients());
}

void DatapointManagerViewModel::setAnalyticsClient(const QString& clientCode) {
    if (m_selectedAnalyticsClient != clientCode) {
        m_selectedAnalyticsClient = clientCode;
        emit selectedAnalyticsClientChanged();
        loadAnalytics();
    }
}

void DatapointManagerViewModel::loadAnalytics() {
    setLoading(true);
    
    auto watcher = new QFutureWatcher<QVariantMap>(this);
    connect(watcher, &QFutureWatcher<QVariantMap>::finished, this, [this, watcher]() {
        QVariantMap results = watcher->result();
        
        m_activeContracts = results.value("activeContracts", 0).toInt();
        m_totalMonoplicatesPool = results.value("totalMonoplicatesPool", 0).toInt();
        m_totalMonoplicatesConsumed = results.value("totalMonoplicatesConsumed", 0).toInt();
        m_clientConsumptionStats = results.value("clientConsumptionStats").toList();
        m_assayTypeStats = results.value("assayTypeStats").toList();
        m_testCategoryStats = results.value("testCategoryStats").toList();
        m_allTestSpecificStats = results.value("testSpecificStats").toList();
        
        updateTestSpecificStats();
        
        emit analyticsChanged();
        setLoading(false);
        watcher->deleteLater();
    });
    
    watcher->setFuture(m_repository.fetchAnalyticsStats(m_selectedAnalyticsClient));
}

QVariantList DatapointManagerViewModel::testSpecificStats() const {
    return m_testSpecificStats;
}

QString DatapointManagerViewModel::selectedCategory() const {
    return m_selectedCategory;
}

void DatapointManagerViewModel::selectCategory(const QString& category) {
    if (m_selectedCategory != category) {
        m_selectedCategory = category;
        emit selectedCategoryChanged();
        updateTestSpecificStats();
    }
}

void DatapointManagerViewModel::updateTestSpecificStats() {
    QVariantList filtered;
    for (const QVariant& v : m_allTestSpecificStats) {
        QVariantMap item = v.toMap();
        if (m_selectedCategory.isEmpty() || item["category"].toString() == m_selectedCategory) {
            filtered.append(item);
        }
    }
    m_testSpecificStats = filtered;
    emit testSpecificStatsChanged();
}

void DatapointManagerViewModel::toggleAnnualization(const QString& clientCode, bool isAnnualized) {
    setLoading(true);
    auto watcher = new QFutureWatcher<bool>(this);
    connect(watcher, &QFutureWatcher<bool>::finished, this, [this, watcher, clientCode, isAnnualized]() {
        if (watcher->result()) {
            emit successMessage(QString("Annualization status for %1 updated successfully.").arg(clientCode));
            loadClients(); // Reload to refresh list
        } else {
            emit errorOccurred("Failed to update annualization status.");
            setLoading(false);
        }
        watcher->deleteLater();
    });
    watcher->setFuture(m_repository.updateClientAnnualization(clientCode, isAnnualized));
}

void DatapointManagerViewModel::updateDatapoints(const QString& clientCode, int total, int used) {
    setLoading(true);
    auto watcher = new QFutureWatcher<bool>(this);
    connect(watcher, &QFutureWatcher<bool>::finished, this, [this, watcher, clientCode]() {
        if (watcher->result()) {
            emit successMessage(QString("Datapoints for %1 updated successfully.").arg(clientCode));
            loadClients();
        } else {
            emit errorOccurred("Failed to update datapoints.");
            setLoading(false);
        }
        watcher->deleteLater();
    });
    watcher->setFuture(m_repository.updateClientDatapoints(clientCode, total, used));
}

void DatapointManagerViewModel::loadPricingOverrides(const QString& clientCode) {
    setLoading(true);
    
    auto watcher = new QFutureWatcher<QVector<domain::ClientPricingOverrideDto>>(this);
    connect(watcher, &QFutureWatcher<QVector<domain::ClientPricingOverrideDto>>::finished, this, [this, watcher]() {
        QVector<domain::ClientPricingOverrideDto> results = watcher->result();
        m_selectedClientPricing.clear();
        for (const auto& dto : results) {
            QVariantMap map;
            map["id"] = dto.id;
            map["clientCode"] = dto.clientCode;
            map["testId"] = dto.testId;
            map["ratioPrimary"] = dto.ratioPrimary;
            map["ratioSecondary"] = dto.ratioSecondary;
            map["testsetFee"] = dto.testsetFee;
            map["minPerRequest"] = dto.minPerRequest;
            m_selectedClientPricing.append(map);
        }
        emit selectedClientPricingChanged();
        setLoading(false);
        watcher->deleteLater();
    });
    
    watcher->setFuture(m_repository.fetchPricingOverrides(clientCode));
}

void DatapointManagerViewModel::savePricingOverride(int id, const QString& clientCode, const QString& testId, 
                                                    double ratioPrimary, double ratioSecondary, 
                                                    int testsetFee, int minPerRequest) {
    setLoading(true);
    
    domain::ClientPricingOverrideDto dto;
    dto.id = id;
    dto.clientCode = clientCode;
    dto.testId = testId;
    dto.ratioPrimary = ratioPrimary;
    dto.ratioSecondary = ratioSecondary;
    dto.testsetFee = testsetFee;
    dto.minPerRequest = minPerRequest;
    
    auto watcher = new QFutureWatcher<bool>(this);
    connect(watcher, &QFutureWatcher<bool>::finished, this, [this, watcher, clientCode]() {
        if (watcher->result()) {
            emit successMessage("Pricing override saved successfully.");
            loadPricingOverrides(clientCode);
        } else {
            emit errorOccurred("Failed to save pricing override.");
            setLoading(false);
        }
        watcher->deleteLater();
    });
    watcher->setFuture(m_repository.savePricingOverride(dto));
}

void DatapointManagerViewModel::deletePricingOverride(int id, const QString& clientCode) {
    setLoading(true);
    auto watcher = new QFutureWatcher<bool>(this);
    connect(watcher, &QFutureWatcher<bool>::finished, this, [this, watcher, clientCode]() {
        if (watcher->result()) {
            emit successMessage("Pricing override deleted.");
            loadPricingOverrides(clientCode);
        } else {
            emit errorOccurred("Failed to delete pricing override.");
            setLoading(false);
        }
        watcher->deleteLater();
    });
    watcher->setFuture(m_repository.deletePricingOverride(id));
}
