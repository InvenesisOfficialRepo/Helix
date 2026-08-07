#ifndef INVENESIS_EXPERIMENTJSONSERIALIZER_H
#define INVENESIS_EXPERIMENTJSONSERIALIZER_H

#include <QJsonObject>
#include <QJsonArray>
#include <QAbstractItemModel>
#include <QStandardItemModel>
#include <QMap>
#include <QSet>
#include <QList>
#include <QString>

class DaughterPlateWidget; // Forward declaration

namespace ExperimentJsonSerializer {

    enum class DaughterPlateType {
        Plate96,
        Plate384
    };

    /**
     * @brief Builds the complex experiment JSON structure from the UI models and plates.
     */
    QJsonObject buildExperimentJson(
        const QString &experimentCode,
        const QString &username,
        const QAbstractItemModel *trModel,
        const QAbstractItemModel *cmpModel,
        const QMap<QString, QSet<QString>> &matrixPlateMap,
        const QList<DaughterPlateWidget*> &daughterPlates,
        const QList<DaughterPlateWidget*> &testPlates,
        const QList<DaughterPlateWidget*> &qcPlates,
        DaughterPlateType daughterPlateType,
        const QString &qcPlateType
    );

    /**
     * @brief Adds calculated concentrations to the experiment JSON root object.
     */
    void addConcentrationsToExperimentJson(QJsonObject &root, const QJsonObject& qcPlatesJson);

    /**
     * @brief Sanitizes an experiment JSON object for comparison by stripping derived data (e.g. concentrations, transient keys).
     */
    QJsonObject sanitizeForComparison(const QJsonObject &root);

    /**
     * @brief Checks if the current UI experiment state has changed compared to the last saved snapshot.
     */
    bool isExperimentModified(const QJsonObject &lastSaved, const QJsonObject &currentUiJson);

    // Parsing helpers
    QStandardItemModel* parseTestRequests(const QJsonArray &array, QObject* parent);
    QStandardItemModel* parseCompounds(const QJsonArray &array, QObject* parent);

} // namespace ExperimentJsonSerializer

#endif // INVENESIS_EXPERIMENTJSONSERIALIZER_H
