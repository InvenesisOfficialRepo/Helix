#include "AnalysisModules.h"

#include <QDebug>
#include <QRegularExpression>
#include <QJsonArray>
#include <QJsonObject>
#include <QVariantMap>
#include <QVariantList>
#include <cmath>
#include <numeric>
#include <algorithm>
#include <limits>
#include <set>

// -------------------------------------------------------------
// MATH IMPLEMENTATION
// -------------------------------------------------------------
namespace AnalysisMath {

    double calculateMedian(std::vector<double> values) {
        if (values.empty()) return 0.0;
        std::sort(values.begin(), values.end());
        size_t size = values.size();
        if (size % 2 == 1) {
            return values[size / 2];
        } else {
            return (values[size / 2 - 1] + values[size / 2]) / 2.0;
        }
    }

    double calculateMad(const std::vector<double>& values, double medianVal) {
        if (values.empty()) return 0.0;
        std::vector<double> absDiffs;
        absDiffs.reserve(values.size());
        for (double val : values) {
            absDiffs.push_back(std::abs(val - medianVal));
        }
        return calculateMedian(absDiffs);
    }

    double calculateRobustMedian(const std::vector<double>& rawValues, double k) {
        if (rawValues.empty()) return 0.0;
        if (rawValues.size() <= 4) {
            return calculateMedian(rawValues);
        }

        double med = calculateMedian(rawValues);
        double m = calculateMad(rawValues, med);
        double sigma = m * 1.4826;

        if (sigma > 0.0) {
            std::vector<double> filtered;
            filtered.reserve(rawValues.size());
            for (double x : rawValues) {
                if (std::abs(x - med) <= k * sigma) {
                    filtered.push_back(x);
                }
            }
            if (!filtered.empty()) {
                return calculateMedian(filtered);
            }
        }
        return med;
    }

    // Helper evaluation function for 4-PL
    double fourPL(double x, double bottom, double top, double ec50, double hill) {
        if (x <= 0.0 || ec50 <= 0.0) return bottom;
        return bottom + (top - bottom) / (1.0 + std::pow(ec50 / x, hill));
    }

    // 4-PL Nelder-Mead Fitter
    FitResult fit4PL(const std::vector<double>& xs,
                     const std::vector<double>& ys,
                     std::optional<double> fixTop,
                     std::optional<double> fixBottom) {
        FitResult res;
        if (xs.size() < 3) return res;

        // Find min and max response
        double yMin = ys[0];
        double yMax = ys[0];
        for (double y : ys) {
            if (y < yMin) yMin = y;
            if (y > yMax) yMax = y;
        }

        // Filter positive doses
        std::vector<double> xPos;
        xPos.reserve(xs.size());
        for (double x : xs) {
            if (x > 0.0) xPos.push_back(x);
        }
        if (xPos.size() < 2) return res;

        double logEC50_init = std::log10(calculateMedian(xPos));

        std::vector<double> init;
        if (!fixBottom.has_value()) {
            init.push_back(std::min(0.0, yMin));
        }
        if (!fixTop.has_value()) {
            init.push_back(std::max(100.0, yMax));
        }
        init.push_back(logEC50_init);
        init.push_back(1.0); // Hill slope init

        size_t n = init.size();
        double tol = 1e-7;
        int maxIter = 2000;
        double alpha = 1.0;
        double gamma = 2.0;
        double rho = 0.5;
        double sigma = 0.5;

        // Loss function (SSR)
        auto fn = [&](const std::vector<double>& params) -> double {
            size_t idx = 0;
            double bottom = fixBottom.has_value() ? *fixBottom : params[idx++];
            double top = fixTop.has_value() ? *fixTop : params[idx++];
            double logEC50 = params[idx++];
            double hill = params[idx++];
            if (hill <= 0.0 || logEC50 < -10.0 || logEC50 > 10.0) {
                return std::numeric_limits<double>::infinity();
            }
            double ec50 = std::pow(10.0, logEC50);
            double ssr = 0.0;
            for (size_t i = 0; i < xs.size(); ++i) {
                double diff = ys[i] - fourPL(xs[i], bottom, top, ec50, hill);
                ssr += diff * diff;
            }
            return ssr;
        };

        // Simplex initialization
        std::vector<std::vector<double>> simplex;
        simplex.reserve(n + 1);
        simplex.push_back(init);
        for (size_t i = 0; i < n; ++i) {
            std::vector<double> p = init;
            p[i] = p[i] + (p[i] != 0.0 ? 0.05 * std::abs(p[i]) : 0.05);
            simplex.push_back(p);
        }

        std::vector<double> values(n + 1);
        for (size_t i = 0; i <= n; ++i) {
            values[i] = fn(simplex[i]);
        }

        for (int iter = 0; iter < maxIter; ++iter) {
            // Sort simplex vertices by value
            std::vector<size_t> order(n + 1);
            std::iota(order.begin(), order.end(), 0);
            std::sort(order.begin(), order.end(), [&](size_t a, size_t b) {
                return values[a] < values[b];
            });

            std::vector<std::vector<double>> sortedSimplex(n + 1);
            std::vector<double> sortedValues(n + 1);
            for (size_t i = 0; i <= n; ++i) {
                sortedSimplex[i] = simplex[order[i]];
                sortedValues[i] = values[order[i]];
            }
            simplex = sortedSimplex;
            values = sortedValues;

            if (values[n] - values[0] < tol) {
                break;
            }

            // Calculate centroid
            std::vector<double> centroid(n, 0.0);
            for (size_t i = 0; i < n; ++i) {
                for (size_t j = 0; j < n; ++j) {
                    centroid[j] += simplex[i][j];
                }
            }
            for (size_t j = 0; j < n; ++j) {
                centroid[j] /= n;
            }

            // Reflection
            std::vector<double> reflected(n);
            for (size_t j = 0; j < n; ++j) {
                reflected[j] = centroid[j] + alpha * (centroid[j] - simplex[n][j]);
            }
            double fr = fn(reflected);

            if (fr < values[0]) {
                // Expansion
                std::vector<double> expanded(n);
                for (size_t j = 0; j < n; ++j) {
                    expanded[j] = centroid[j] + gamma * (reflected[j] - centroid[j]);
                }
                double fe = fn(expanded);
                if (fe < fr) {
                    simplex[n] = expanded;
                    values[n] = fe;
                } else {
                    simplex[n] = reflected;
                    values[n] = fr;
                }
            } else if (fr < values[n - 1]) {
                simplex[n] = reflected;
                values[n] = fr;
            } else {
                // Contraction
                std::vector<double> target = (fr < values[n]) ? reflected : simplex[n];
                std::vector<double> contracted(n);
                for (size_t j = 0; j < n; ++j) {
                    contracted[j] = centroid[j] + rho * (target[j] - centroid[j]);
                }
                double fc = fn(contracted);

                if (fc < values[n]) {
                    simplex[n] = contracted;
                    values[n] = fc;
                } else {
                    // Shrink
                    for (size_t i = 1; i <= n; ++i) {
                        for (size_t j = 0; j < n; ++j) {
                            simplex[i][j] = simplex[0][j] + sigma * (simplex[i][j] - simplex[0][j]);
                        }
                        values[i] = fn(simplex[i]);
                    }
                }
            }
        }

        size_t idx = 0;
        res.bottom = fixBottom.has_value() ? *fixBottom : simplex[0][idx++];
        res.top = fixTop.has_value() ? *fixTop : simplex[0][idx++];
        double logEC50 = simplex[0][idx++];
        res.hillslope = simplex[0][idx++];
        res.ec50 = std::pow(10.0, logEC50);
        res.ssr = values[0];
        res.success = std::isfinite(res.ec50) && res.ec50 > 0.0 && res.hillslope > 0.0;
        return res;
    }
}

// -------------------------------------------------------------
// MODULE HELPERS
// -------------------------------------------------------------
static QString getCleanName(const QString& name) {
    QString clean = name.trimmed();
    // remove (Q1), (Q2), (Q3)
    clean.replace(QRegularExpression(R"(\s*\(Q\d+\))"), "");
    // remove (Rep 1), (Rep 2), (Rep 3) etc
    clean.replace(QRegularExpression(R"(\s*\(Rep\s*\d+\))"), "");
    return clean.trimmed();
}

static bool isPlaceboWell(const QString& rawCompound, const QString& type) {
    (void)rawCompound;
    return type.compare("placebo", Qt::CaseInsensitive) == 0;
}

static bool isStandardWell(const QString& rawCompound, const QString& type) {
    (void)rawCompound;
    return type.compare("standard", Qt::CaseInsensitive) == 0;
}

static bool isQcWell(const QString& rawCompound, const QString& type) {
    (void)rawCompound;
    return type.compare("qc", Qt::CaseInsensitive) == 0;
}

// Common logic for running analysis across both modules
static bool runCommonAnalysis(QList<MergedPlateData>& plates, QVariantMap& outResult) {
    // 1) Pool all placebo wells globally across all plates
    std::vector<double> placeboSignals;
    for (const auto& plate : plates) {
        for (const auto& well : plate.wells) {
            if (isPlaceboWell(well.compound, well.type)) {
                placeboSignals.push_back(well.rawSignal);
            }
        }
    }

    if (placeboSignals.empty()) {
        qWarning() << "Analysis Modules: No placebo (DMSO) wells found. Using mock baseline of 12000.0";
        placeboSignals.push_back(12000.0);
    }

    // Calculate robust median of placebo values
    double placeboMedian = AnalysisMath::calculateRobustMedian(placeboSignals, 3.0);
    qDebug() << "Analysis Modules: Placebo global median = " << placeboMedian << "(from" << placeboSignals.size() << "wells)";

    // 2) Normalize efficacy for every well
    QVariantList wellList;
    // compound -> dose -> list of efficacy values
    QMap<QString, QMap<double, QList<double>>> compoundDoseValues;

    // Keep track of which compound clean name is a standard or QC control
    QMap<QString, bool> isStandardMap;
    QMap<QString, bool> isQcMap;

    int plateNum = 1;
    for (auto& plate : plates) {
        for (auto& well : plate.wells) {
            double raw = well.rawSignal;
            // Efficacy = (1 - raw / placebo_median) * 100
            double eff = (1.0 - raw / placeboMedian) * 100.0;
            // Clamp efficacy 0-100%
            eff = std::max(0.0, std::min(100.0, eff));
            well.efficacy = eff;

            QString cleanName = well.cleanCompound;
            
            // Register control categories
            if (isStandardWell(well.compound, well.type)) {
                isStandardMap[cleanName] = true;
            } else if (isQcWell(well.compound, well.type)) {
                isQcMap[cleanName] = true;
            }

            if (!cleanName.isEmpty() && !isPlaceboWell(well.compound, well.type) && cleanName.compare("Empty", Qt::CaseInsensitive) != 0) {
                compoundDoseValues[cleanName][well.concentration].append(eff);
            }

            // Populate heat map data
            QVariantMap wellMap;
            wellMap["well"] = well.well;
            wellMap["plate"] = plateNum;
            wellMap["compound"] = well.compound; // Keep original name in heatmap
            wellMap["dose"] = well.concentration;
            wellMap["efficacy"] = eff;
            wellMap["raw"] = raw;
            wellMap["type"] = well.type;
            if (well.hasFeeding) {
                wellMap["feeding_density"] = well.feedingDensity;
                wellMap["feeding_reduction"] = well.feedingReduction;
            }
            wellList.append(wellMap);
        }
        plateNum++;
    }

    // 3) Fit curves and aggregate replicate responses
    QVariantList compoundList;
    QVariantMap curvesData;

    for (auto it = compoundDoseValues.constBegin(); it != compoundDoseValues.constEnd(); ++it) {
        QString cleanName = it.key();
        
        QVariantList doseResponseList;
        std::vector<double> fitXs;
        std::vector<double> fitYs;

        // Get doses sorted
        QList<double> doses = it.value().keys();
        std::sort(doses.begin(), doses.end());

        double maxEff = 0.0;
        double ec50Value = 0.0;
        double minDiff = 999.0;

        QVariantList replicatePoints;

        for (double dose : doses) {
            QList<double> effs = it.value().value(dose);
            double sum = 0.0;
            for (double e : effs) {
                sum += e;
                QVariantMap repPt;
                repPt["x"] = dose;
                repPt["y"] = e;
                replicatePoints.append(repPt);
            }
            double avgEff = effs.isEmpty() ? 0.0 : sum / effs.size();

            QVariantMap drPoint;
            drPoint["dose"] = dose;
            drPoint["efficacy"] = avgEff;
            doseResponseList.append(drPoint);

            if (avgEff > maxEff) {
                maxEff = avgEff;
            }

            if (dose > 0.0) {
                // Collect for fitting
                for (double e : effs) {
                    fitXs.push_back(dose);
                    fitYs.push_back(e);
                }

                // Approximate EC50
                double diff = std::abs(avgEff - 50.0);
                if (diff < minDiff) {
                    minDiff = diff;
                    ec50Value = dose;
                }
            }
        }

        // Fit 4-PL
        AnalysisMath::FitResult fitRes = AnalysisMath::fit4PL(fitXs, fitYs, 100.0, 0.0); // Fit with top fixed to 100, bottom fixed to 0

        QVariantList curvePoints;
        bool isActive = false;

        // If fit succeeded and max efficacy >= 35%
        if (fitRes.success && maxEff >= 35.0) {
            isActive = true;
            ec50Value = fitRes.ec50;

            // Generate 100 curve points along log concentration
            double minPosDose = 999.0;
            double maxPosDose = 0.0;
            for (double d : fitXs) {
                if (d > 0.0) {
                    if (d < minPosDose) minPosDose = d;
                    if (d > maxPosDose) maxPosDose = d;
                }
            }

            if (minPosDose < maxPosDose) {
                double logMin = std::log10(minPosDose);
                double logMax = std::log10(maxPosDose);
                double step = (logMax - logMin) / 99.0;
                for (int i = 0; i < 100; ++i) {
                    double logD = logMin + i * step;
                    double d = std::pow(10.0, logD);
                    double fittedY = AnalysisMath::fourPL(d, fitRes.bottom, fitRes.top, fitRes.ec50, fitRes.hillslope);

                    QVariantMap pt;
                    pt["x"] = d;
                    pt["y"] = fittedY;
                    curvePoints.append(pt);
                }
            }
        } else {
            // Fit failed or max response too low -> Inactive
            isActive = (maxEff >= 35.0); // If max efficacy >= 35%, still call active but set approximate EC50
            if (!isActive) {
                ec50Value = 0.0;
            }
        }

        // Add curves to output
        if (isActive && !curvePoints.isEmpty()) {
            curvesData[cleanName] = curvePoints;
        }

        double ec80Value = 0.0;
        if (fitRes.success && fitRes.hillslope > 0.0 && isActive) {
            ec80Value = fitRes.ec50 * std::pow(4.0, 1.0 / fitRes.hillslope);
        }

        QVariantMap cmpMap;
        cmpMap["compound_name"] = cleanName;
        cmpMap["dose_responses"] = doseResponseList;
        cmpMap["ec50"] = ec50Value;
        cmpMap["ec80"] = ec80Value;
        cmpMap["status"] = isActive ? "Active" : "Inactive";
        cmpMap["is_standard"] = isStandardMap.value(cleanName, false);
        cmpMap["is_qc"] = isQcMap.value(cleanName, false);
        cmpMap["replicate_points"] = replicatePoints;
        compoundList.append(cmpMap);
    }

    // 4) Compute Quality Control Metrics
    double qcBleedThresh = 8.0; // 8%
    double qcLowThresh = 15.0;

    std::vector<double> placeboEfficacies;
    struct PlateQcStats {
        QString barcode;
        std::vector<double> raws;
        std::vector<double> effs;
    };
    QList<PlateQcStats> plateQcs;
    
    int pIdx = 1;
    for (const auto& plate : plates) {
        PlateQcStats pStats;
        pStats.barcode = plate.barcode.isEmpty() ? QString("Plate %1").arg(pIdx) : plate.barcode;
        for (const auto& well : plate.wells) {
            if (isPlaceboWell(well.compound, well.type)) {
                placeboEfficacies.push_back(well.efficacy);
                pStats.raws.push_back(well.rawSignal);
                pStats.effs.push_back(well.efficacy);
            }
        }
        plateQcs.append(pStats);
        pIdx++;
    }

    int nPlacebo = placeboEfficacies.size();
    int nBleed = 0;
    for (double eff : placeboEfficacies) {
        if (eff > 80.0) nBleed++;
    }
    double pctBleed = nPlacebo > 0 ? ((double)nBleed / nPlacebo * 100.0) : 0.0;
    
    double meanRaw = 0.0;
    if (!placeboSignals.empty()) {
        double sum = 0.0;
        for (double s : placeboSignals) sum += s;
        meanRaw = sum / placeboSignals.size();
    }
    double medRaw = placeboMedian;

    QVariantMap qcMap;
    qcMap["pctBleed"] = pctBleed;
    qcMap["nBleed"] = nBleed;
    qcMap["nPlacebo"] = nPlacebo;
    qcMap["meanRaw"] = meanRaw;
    qcMap["medRaw"] = medRaw;

    QVariantList perPlateList;
    for (const auto& pQc : plateQcs) {
        int pNBleed = 0;
        for (double eff : pQc.effs) {
            if (eff > 80.0) pNBleed++;
        }
        double pPctBleed = pQc.effs.size() > 0 ? ((double)pNBleed / pQc.effs.size() * 100.0) : 0.0;
        
        double pMeanRaw = 0.0;
        if (!pQc.raws.empty()) {
            double sum = 0.0;
            for (double s : pQc.raws) sum += s;
            pMeanRaw = sum / pQc.raws.size();
        }
        double pMedRaw = pQc.raws.empty() ? 0.0 : AnalysisMath::calculateMedian(pQc.raws);

        // Status determination
        bool isBleedFail = pPctBleed > qcBleedThresh;
        bool isLowFail = pMeanRaw < qcLowThresh;
        QString status = "PASS";
        if (isBleedFail || isLowFail) {
            status = "FAIL";
        } else {
            bool isBleedWarn = pPctBleed > (qcBleedThresh * 0.6);
            bool isLowWarn = pMeanRaw < (qcLowThresh * 1.3);
            if (isBleedWarn || isLowWarn) {
                status = "WARN";
            }
        }

        QVariantMap pQcMap;
        pQcMap["barcode"] = pQc.barcode;
        pQcMap["n"] = (int)pQc.effs.size();
        pQcMap["pctBleed"] = pPctBleed;
        pQcMap["nBleed"] = pNBleed;
        pQcMap["meanRaw"] = pMeanRaw;
        pQcMap["medRaw"] = pMedRaw;
        pQcMap["status"] = status;
        perPlateList.append(pQcMap);
    }
    qcMap["per_plate"] = perPlateList;
    
    outResult["qc"] = qcMap;
    outResult["success"] = true;
    outResult["compounds"] = compoundList;
    outResult["wells"] = wellList;
    outResult["curves"] = curvesData;
    return true;
}

// -------------------------------------------------------------
// TEST STRATEGY MODULES
// -------------------------------------------------------------

bool Test384AnalysisModule::runAnalysis(QList<MergedPlateData>& plates,
                                        const QJsonObject& expJson,
                                        const QString& timepoint,
                                        QVariantMap& outResult) {
    qDebug() << "Running Test384AnalysisModule Strategy for timepoint:" << timepoint;
    (void)expJson;
    return runCommonAnalysis(plates, outResult);
}

bool Test96AnalysisModule::runAnalysis(QList<MergedPlateData>& plates,
                                       const QJsonObject& expJson,
                                       const QString& timepoint,
                                       QVariantMap& outResult) {
    qDebug() << "Running Test96AnalysisModule Strategy for timepoint:" << timepoint;
    (void)expJson;
    // Note: DMSO plates are pooled globally by runCommonAnalysis, which covers the 96-well extra plate requirements.
    return runCommonAnalysis(plates, outResult);
}

// -------------------------------------------------------------
// STRATEGY FACTORY
// -------------------------------------------------------------
std::unique_ptr<IAnalysisModule> AnalysisModuleFactory::createModule(const QString& testId, int plateFormat) {
    qDebug() << "AnalysisModuleFactory: Creating module for testId:" << testId << "plateFormat:" << plateFormat;
    // Default fallback based on plate format
    if (plateFormat == 384) {
        return std::make_unique<Test384AnalysisModule>();
    } else {
        return std::make_unique<Test96AnalysisModule>();
    }
}
