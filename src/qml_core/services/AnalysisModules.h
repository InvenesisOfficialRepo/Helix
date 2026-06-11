#pragma once

#include <QObject>
#include <QString>
#include <QList>
#include <QVariantMap>
#include <QJsonObject>
#include <QJsonArray>
#include <memory>
#include <optional>
#include <vector>

// Structure holding the merged data for a single well
struct MergedWellData {
    QString well;
    QString compound;
    QString cleanCompound;
    double concentration = 0.0;
    QString unit;
    QString solutionId;
    QString projectId;
    double rawSignal = 0.0;
    double efficacy = 0.0;
    QString type; // "test", "placebo", "standard", "qc"
    int replicate = 1;
    bool hasFeeding = false;
    double feedingDensity = 0.0;
    double feedingReduction = 0.0;
};

// Structure holding the merged data for an entire plate
struct MergedPlateData {
    QString barcode;
    QList<MergedWellData> wells;
    QString error;
};

// Math helpers for robust calculations and curve fitting
namespace AnalysisMath {
    double calculateMedian(std::vector<double> values);
    double calculateMad(const std::vector<double>& values, double medianVal);
    double calculateRobustMedian(const std::vector<double>& rawValues, double k = 3.0);

    struct FitResult {
        double bottom = 0.0;
        double top = 100.0;
        double ec50 = 0.0;
        double hillslope = 1.0;
        double ssr = 0.0;
        bool success = false;
    };

    // 4-PL Nelder-Mead Simplex Fitter
    FitResult fit4PL(const std::vector<double>& xs,
                     const std::vector<double>& ys,
                     std::optional<double> fixTop = 100.0,
                     std::optional<double> fixBottom = 0.0);
}

// Interface for strategy analysis modules
class IAnalysisModule {
public:
    virtual ~IAnalysisModule() = default;

    /**
     * Executes the plate-specific analysis strategy:
     * 1. Placebo robust median calculation
     * 2. Efficacy calculation for each well
     * 3. Replicate aggregation
     * 4. Nelder-Mead curve fitting
     * 5. Populates outResult with "compounds", "wells", "curves"
     */
    virtual bool runAnalysis(QList<MergedPlateData>& plates,
                             const QJsonObject& expJson,
                             const QString& timepoint,
                             QVariantMap& outResult) = 0;
};

// Module for 384-well plate tests (integrated QC and DMSO controls)
class Test384AnalysisModule : public IAnalysisModule {
public:
    bool runAnalysis(QList<MergedPlateData>& plates,
                     const QJsonObject& expJson,
                     const QString& timepoint,
                     QVariantMap& outResult) override;
};

// Module for 96-well plate tests (separate QC and DMSO plates)
class Test96AnalysisModule : public IAnalysisModule {
public:
    bool runAnalysis(QList<MergedPlateData>& plates,
                     const QJsonObject& expJson,
                     const QString& timepoint,
                     QVariantMap& outResult) override;
};

// Factory to instantiate the appropriate module
class AnalysisModuleFactory {
public:
    static std::unique_ptr<IAnalysisModule> createModule(const QString& testId, int plateFormat);
};
