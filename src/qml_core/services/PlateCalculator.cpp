#include "PlateCalculator.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QVector>
#include "../db/DatabaseManager.h"
#include "../../shared/ReferenceDataRepository.h"

int PlateCalculator::estimateTestPlatesCount(const QString& testType, const QList<CompoundInfo>& compounds) {
    if (compounds.isEmpty()) return 0;

    bool is384 = false;
    ReferenceDataRepository repo(DatabaseManager::db("QPSQL"));
    QJsonObject vocab = repo.getTestsVocabulary();
    if (vocab.value(testType).toInt() == 384) {
        is384 = true;
    }

    const int maxColumns = 12; // daughter plate is always 96 well (12 cols, 8 rows)
    const int numRows = 8;
    const bool isINV_T_031 = testType.contains(QLatin1String("INV-T-031"), Qt::CaseInsensitive) && !is384;

    const int standardCol = isINV_T_031 ? (maxColumns - 1) : -1;
    const int dmsoCol = isINV_T_031 ? maxColumns : -1;

    int maxDil = 3;
    for (const auto& cmpd : compounds) {
        if (cmpd.numberOfDilutions > maxDil)
            maxDil = cmpd.numberOfDilutions;
    }
    const int stdDil = qMax(maxDil, 6);

    struct PlateMask {
        QList<QList<bool>> rowUsed; // size: numRows x (maxColumns + 1)
    };

    QList<PlateMask> plateMasks;

    auto initPlateMask = [&](PlateMask &pm) {
        pm.rowUsed.resize(numRows);
        for (int r = 0; r < numRows; ++r) {
            pm.rowUsed[r].clear();
            for (int c = 0; c <= maxColumns; ++c) {
                pm.rowUsed[r].append(false);
            }
            
            if (isINV_T_031) {
                pm.rowUsed[r][standardCol] = true;
                pm.rowUsed[r][dmsoCol] = true;
            } else {
                if (r == numRows - 1) { // DMSO row is H
                    for (int c = 1; c <= maxColumns; ++c) {
                        pm.rowUsed[r][c] = true;
                    }
                }
                if (r == 0) { // Standard row is A
                    for (int c = 1; c <= stdDil && c <= maxColumns; ++c) {
                        pm.rowUsed[r][c] = true;
                    }
                }
            }
        }
    };

    auto findFirstSlot = [&](int dil) -> QPair<int, QPair<int, int>> {
        for (int p = 0; p < plateMasks.size(); ++p) {
            auto &pm = plateMasks[p];
            for (int r = 0; r < numRows; ++r) {
                int consecutiveCount = 0;
                int startCol = -1;
                for (int c = 1; c <= maxColumns; ++c) {
                    if (!pm.rowUsed[r][c]) {
                        if (startCol == -1) startCol = c;
                        consecutiveCount++;
                        if (consecutiveCount == dil) {
                            return qMakePair(p, qMakePair(r, startCol));
                        }
                    } else {
                        consecutiveCount = 0;
                        startCol = -1;
                    }
                }
            }
        }
        return qMakePair(-1, qMakePair(-1, -1));
    };

    for (const auto& cmpd : compounds) {
        const int dil = cmpd.numberOfDilutions > 0 ? cmpd.numberOfDilutions : 3;
        
        auto slot = findFirstSlot(dil);
        int pIdx = slot.first;
        int rIdx = slot.second.first;
        int cIdx = slot.second.second;
        
        if (pIdx == -1) {
            PlateMask newPm;
            initPlateMask(newPm);
            plateMasks.append(newPm);
            
            pIdx = plateMasks.size() - 1;
            auto newSlot = findFirstSlot(dil);
            rIdx = newSlot.second.first;
            cIdx = newSlot.second.second;
            
            if (rIdx == -1) {
                continue; // compound doesn't fit on empty plate (unlikely)
            }
        }
        
        auto &pm = plateMasks[pIdx];
        for (int d = 0; d < dil; ++d) {
            pm.rowUsed[rIdx][cIdx + d] = true;
        }
    }

    int numDaughterPlates = plateMasks.isEmpty() ? 1 : plateMasks.size();
    if (is384) {
        return numDaughterPlates; // 1 daughter = 1 test plate (3 replicates + QC packed into it)
    } else {
        int maxReps = 1;
        for (const auto& cmpd : compounds) {
            if (cmpd.numberOfReplicates > maxReps) {
                maxReps = cmpd.numberOfReplicates;
            }
        }
        return numDaughterPlates * maxReps; // 1 daughter = 1 test plate per replicate
    }
}
