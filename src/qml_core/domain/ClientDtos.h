#pragma once

#include <QString>
#include <QDateTime>
#include <QVector>

namespace domain {

struct ClientPricingOverrideDto {
    int id{0};
    QString clientCode;
    QString testId;
    double ratioPrimary{1.0};
    double ratioSecondary{1.0};
    int testsetFee{0};
    int minPerRequest{0};
};

struct ClientDto {
    QString clientCode;
    QString companyName;
    QDateTime createdAt;
    bool isAnnualized{false};
    int totalDatapoints{0};
    int usedDatapoints{0};
    
    QVector<ClientPricingOverrideDto> pricingOverrides;
};

} // namespace domain
