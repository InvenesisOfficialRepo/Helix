#pragma once

#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include <QFutureWatcher>
#include "repositories/ClientRepository.h"

class DatapointManagerViewModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isLoading READ isLoading NOTIFY isLoadingChanged)
    Q_PROPERTY(QVariantList clients READ clients NOTIFY clientsChanged)
    Q_PROPERTY(QVariantList selectedClientPricing READ selectedClientPricing NOTIFY selectedClientPricingChanged)

    // Analytics properties
    Q_PROPERTY(QString selectedAnalyticsClient READ selectedAnalyticsClient WRITE setAnalyticsClient NOTIFY selectedAnalyticsClientChanged)
    Q_PROPERTY(int activeContracts READ activeContracts NOTIFY analyticsChanged)
    Q_PROPERTY(int totalMonoplicatesPool READ totalMonoplicatesPool NOTIFY analyticsChanged)
    Q_PROPERTY(int totalMonoplicatesConsumed READ totalMonoplicatesConsumed NOTIFY analyticsChanged)
    Q_PROPERTY(QVariantList clientConsumptionStats READ clientConsumptionStats NOTIFY analyticsChanged)
    Q_PROPERTY(QVariantList assayTypeStats READ assayTypeStats NOTIFY analyticsChanged)
    Q_PROPERTY(QVariantList testCategoryStats READ testCategoryStats NOTIFY analyticsChanged)

public:
    explicit DatapointManagerViewModel(QObject *parent = nullptr);

    bool isLoading() const;
    QVariantList clients() const;
    QVariantList selectedClientPricing() const;
    
    QString selectedAnalyticsClient() const;
    
    int activeContracts() const;
    int totalMonoplicatesPool() const;
    int totalMonoplicatesConsumed() const;
    QVariantList clientConsumptionStats() const;
    QVariantList assayTypeStats() const;
    QVariantList testCategoryStats() const;

    Q_INVOKABLE void loadClients();
    Q_INVOKABLE void loadAnalytics();
    Q_INVOKABLE void setAnalyticsClient(const QString& clientCode);
    Q_INVOKABLE void toggleAnnualization(const QString& clientCode, bool isAnnualized);
    Q_INVOKABLE void updateDatapoints(const QString& clientCode, int total, int used);
    
    Q_INVOKABLE void loadPricingOverrides(const QString& clientCode);
    Q_INVOKABLE void savePricingOverride(int id, const QString& clientCode, const QString& testId, 
                                         double ratioPrimary, double ratioSecondary, 
                                         int testsetFee, int minPerRequest);
    Q_INVOKABLE void deletePricingOverride(int id, const QString& clientCode);

signals:
    void isLoadingChanged();
    void clientsChanged();
    void selectedClientPricingChanged();
    void selectedAnalyticsClientChanged();
    void analyticsChanged();
    void errorOccurred(const QString& message);
    void successMessage(const QString& message);

private:
    void setLoading(bool loading);
    
    bool m_isLoading = false;
    QVariantList m_clients;
    QVariantList m_selectedClientPricing;
    
    int m_activeContracts = 0;
    int m_totalMonoplicatesPool = 0;
    int m_totalMonoplicatesConsumed = 0;
    QVariantList m_clientConsumptionStats;
    QVariantList m_assayTypeStats;
    QVariantList m_testCategoryStats;
    QString m_selectedAnalyticsClient;
    
    ClientRepository m_repository;
};
