#pragma once

#include <QString>
#include <QList>

class PlateCalculator {
public:
    struct CompoundInfo {
        QString name;
        int numberOfDilutions;
        int numberOfReplicates;
    };

    static int estimateTestPlatesCount(const QString& testType, const QList<CompoundInfo>& compounds);
};
