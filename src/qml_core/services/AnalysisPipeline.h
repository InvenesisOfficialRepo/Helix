#pragma once

#include <QObject>
#include <QString>
#include <QVariantMap>
#include <QVariantList>
#include <QUrl>

class AnalysisPipeline : public QObject
{
    Q_OBJECT
public:
    explicit AnalysisPipeline(QObject *parent = nullptr);

    /**
     * Runs the analysis pipeline by parsing the experiment JSON, merging layout and raw data,
     * exporting a merged CSV per plate, and calculating compound-dose efficacies.
     */
    Q_INVOKABLE QVariantMap runAnalysis(const QVariantList& layoutCsvs,
                                        const QVariantList& rawDataCsvs,
                                        const QUrl& expJsonUrl,
                                        const QString& timepoint,
                                        const QString& batchId,
                                        const QVariantList& feedingDataCsvs = QVariantList(),
                                        const QString& overrideTestId = QString());
};
