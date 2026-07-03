#include "AnalysisPipeline.h"
#include "AnalysisModules.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QTextStream>
#include <QUrl>
#include <QSqlQuery>
#include <QSqlError>
#include <QRegularExpression>
#include <cmath>
#include <algorithm>
#include "db/DbConfig.h"

static QString getCleanName(const QString& name) {
    QString clean = name.trimmed();
    // remove (Q1), (Q2), (Q3)
    clean.replace(QRegularExpression(R"(\s*\(Q\d+\))"), "");
    // remove (Rep 1), (Rep 2), (Rep 3) etc
    clean.replace(QRegularExpression(R"(\s*\(Rep\s*\d+\))"), "");
    return clean.trimmed();
}

static QString toLocalFileHelper(const QVariant& var) {
    if (var.typeId() == QMetaType::QUrl) {
        QUrl url = var.toUrl();
        QString localFile = url.toLocalFile();
        if (!localFile.isEmpty()) {
            return localFile;
        }
        // Fallback for QUrl parsed from raw Windows path (e.g. "C:\...")
        if (url.scheme().length() == 1) {
            QString path = url.scheme() + ":" + url.path();
            path.replace("\\", "/");
            return path;
        }
        return url.toString(QUrl::FullyDecoded);
    }
    
    // Fallback: the QVariant might be a string.
    QString str = var.toString();
    str.replace("\\", "/"); // Sanitize backslashes to forward slashes
    
    if (str.startsWith("file://", Qt::CaseInsensitive)) {
        return QUrl(str).toLocalFile();
    }
    
    // If it's something like "C:/path/...", just return it directly.
    return str;
}

static void parseWellTypeAndName(const QString& alias, const QString& userdef1, QString& outBaseName, QString& outType, int& outRep) {
    QString trimmedAlias = alias.trimmed();
    QString trimmedUserdef1 = userdef1.trimmed();

    // 1. Regex to extract base name and bracket content
    QRegularExpression rx(R"(^(.+?)\s*\((.+?)\)\s*$)");
    QRegularExpressionMatch match = rx.match(trimmedAlias);
    QString bracketContent;
    if (match.hasMatch()) {
        outBaseName = match.captured(1).trimmed();
        bracketContent = match.captured(2).trimmed().toUpper();
    } else {
        outBaseName = trimmedAlias;
    }

    // 2. Extract replicate/quadrant
    outRep = 1;
    QRegularExpression repRx(R"(\b(?:REP\s*|Q)(\d+))", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch repMatch = repRx.match(trimmedAlias);
    if (repMatch.hasMatch()) {
        outRep = repMatch.captured(1).toInt();
    }

    // 3. Classify well type
    QString upperBase = outBaseName.toUpper();
    if (upperBase == "DMSO" || trimmedUserdef1.startsWith("DMSO_", Qt::CaseInsensitive)) {
        outType = "placebo";
    } else if (upperBase == "STANDARD" || bracketContent.contains("STANDARD")) {
        outType = "standard";
    } else if (trimmedUserdef1.startsWith("QC_", Qt::CaseInsensitive) || 
               (bracketContent.contains("QC") && upperBase != "DMSO")) {
        outType = "qc";
    } else {
        outType = "test";
    }
}

AnalysisPipeline::AnalysisPipeline(QObject *parent)
    : QObject(parent)
{
}


// Helper: Normalize well coordinate (e.g. "A1" -> "A01", "A01" -> "A01")
static QString normalizeWellCoordinate(const QString& well) {
    QString w = well.trimmed().toUpper();
    if (w.isEmpty()) return "";
    QChar rowChar = w[0];
    if (!rowChar.isLetter()) return w;
    QString colStr = w.mid(1);
    bool ok;
    int col = colStr.toInt(&ok);
    if (ok) {
        return QString("%1%2").arg(rowChar).arg(col, 2, 10, QChar('0'));
    }
    return w;
}

// Helper: Convert 1-96 arena index to well coordinate
static QString arenaToWell(int arena) {
    if (arena < 1 || arena > 96) return "";
    int r = (arena - 1) / 12;
    int c = (arena - 1) % 12 + 1;
    char rowChar = 'A' + r;
    return QString("%1%2").arg(rowChar).arg(c, 2, 10, QChar('0'));
}

// Helper: Trim quotes from a field
static QString trimQuotes(const QString& s) {
    QString r = s.trimmed();
    if (r.startsWith('"') && r.endsWith('"')) {
        r = r.mid(1, r.length() - 2);
    }
    return r.trimmed();
}

// CSV Table parser
struct CsvTable {
    QStringList headers;
    QList<QStringList> rows;
};

static CsvTable parseCsv(const QString& filePath) {
    CsvTable table;
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "AnalysisPipeline: Failed to open CSV file:" << filePath;
        return table;
    }
    QTextStream stream(&file);
    QString headerLine = stream.readLine();
    if (headerLine.isNull()) return table;

    // Detect separator (comma or semicolon)
    char sep = ',';
    if (headerLine.count(';') > headerLine.count(',')) {
        sep = ';';
    }

    auto splitRow = [sep](const QString& line) -> QStringList {
        QStringList fields;
        QString currentField;
        bool inQuotes = false;
        for (int i = 0; i < line.length(); ++i) {
            QChar c = line[i];
            if (c == '"') {
                inQuotes = !inQuotes;
            } else if (c == sep && !inQuotes) {
                fields.append(trimQuotes(currentField));
                currentField.clear();
            } else {
                currentField.append(c);
            }
        }
        fields.append(trimQuotes(currentField));
        return fields;
    };

    table.headers = splitRow(headerLine);
    for (int i = 0; i < table.headers.size(); ++i) {
        table.headers[i] = trimQuotes(table.headers[i]);
    }

    while (!stream.atEnd()) {
        QString line = stream.readLine();
        if (line.trimmed().isEmpty()) continue;
        table.rows.append(splitRow(line));
    }
    file.close();
    return table;
}

// Column index matching helper
static int findColumnIndex(const QStringList& headers, const QStringList& patterns) {
    for (int i = 0; i < headers.size(); ++i) {
        QString h = headers[i].toLower();
        for (const QString& pat : patterns) {
            if (h.contains(pat.toLower())) {
                return i;
            }
        }
    }
    return -1;
}

// Read raw signal with double conversion and fallback to last numeric column
static double getRawSignal(const QStringList& row, int meanIdx) {
    if (meanIdx >= 0 && meanIdx < row.size()) {
        bool ok;
        double val = row[meanIdx].toDouble(&ok);
        if (ok) return val;
    }
    for (int i = row.size() - 1; i >= 0; --i) {
        bool ok;
        double val = row[i].toDouble(&ok);
        if (ok) return val;
    }
    return 0.0;
}

static bool isPlaceboWell(const QString& rawCompound, const QString& type) {
    if (type.compare("placebo", Qt::CaseInsensitive) == 0 || type.compare("negative", Qt::CaseInsensitive) == 0) {
        return true;
    }
    QString upper = rawCompound.toUpper();
    return upper.contains("DMSO") || upper.contains("PLACEBO") || upper.contains("ETOH");
}

static bool isStandardWell(const QString& rawCompound, const QString& type) {
    if (type.compare("standard", Qt::CaseInsensitive) == 0) {
        return true;
    }
    QString upper = rawCompound.toUpper();
    return (upper.contains("STANDARD") || upper.contains("GABA")) && !upper.contains("QC");
}

static bool isQcWell(const QString& rawCompound, const QString& type) {
    if (type.compare("qc", Qt::CaseInsensitive) == 0) {
        return true;
    }
    QString upper = rawCompound.toUpper();
    return upper.contains("QC") || (upper.contains("CONTROL") && !upper.contains("STANDARD") && !upper.contains("DMSO") && !upper.contains("PLACEBO"));
}

QVariantMap AnalysisPipeline::runAnalysis(const QVariantList& layoutCsvs,
                                          const QVariantList& rawDataCsvs,
                                          const QUrl& expJsonUrl,
                                          const QString& timepoint,
                                          const QString& batchId,
                                          const QVariantList& feedingDataCsvs,
                                          const QString& overrideTestId)
{
    (void)batchId;
    QVariantMap result;
    result["success"] = false;
    result["error"] = "";

    // Parse feeding data CSVs if provided
    QMap<QString, QMap<QString, double>> plateFeedingValues; // plate barcode -> well -> feeding density
    for (const QVariant& feedingUrlVar : feedingDataCsvs) {
        QString feedingFile = toLocalFileHelper(feedingUrlVar);
        QString feedingName = QFileInfo(feedingFile).baseName(); // e.g., "raw_PHC006641"
        QString plateBarcode = feedingName;
        if (plateBarcode.startsWith("raw_", Qt::CaseInsensitive)) {
            plateBarcode = plateBarcode.mid(4);
        }

        CsvTable feedingTable = parseCsv(feedingFile);
        int fWellIdx = findColumnIndex(feedingTable.headers, {"well"});
        int fMeanIdx = findColumnIndex(feedingTable.headers, {"mean", "value"});

        QMap<QString, double> wellMap;
        for (const QStringList& row : feedingTable.rows) {
            QString feedingWell;
            if (fWellIdx >= 0 && fWellIdx < row.size()) {
                feedingWell = normalizeWellCoordinate(row[fWellIdx]);
            }
            if (feedingWell.isEmpty()) continue;

            double val = getRawSignal(row, fMeanIdx);
            wellMap[feedingWell] = val;
        }
        plateFeedingValues[plateBarcode.toUpper()] = wellMap;
    }

    if (layoutCsvs.isEmpty()) {
        result["error"] = "No Layout CSV files selected.";
        return result;
    }
    if (rawDataCsvs.isEmpty()) {
        result["error"] = "No Raw Data CSV files selected.";
        return result;
    }

    // A) Retrieve project code and compounds in the test request from the database
    QString projectCode;
    QStringList dbCompounds;
    if (!batchId.isEmpty()) {
        DbConfig cfg;
        QString cfgErr;
        if (loadDbConfigFromEnv(cfg, cfgErr)) {
            QString connName = "analysis_pipeline_conn";
            QSqlDatabase db;
            if (QSqlDatabase::contains(connName)) {
                db = QSqlDatabase::database(connName);
            } else {
                db = QSqlDatabase::addDatabase("QPSQL", connName);
                db.setHostName(cfg.host);
                db.setPort(cfg.port);
                db.setDatabaseName(cfg.dbName);
                db.setUserName(cfg.user);
                db.setPassword(cfg.password);
            }
            if (!db.isOpen()) {
                if (!db.open()) {
                    qWarning() << "AnalysisPipeline: Failed to open database connection:" << db.lastError().text();
                }
            }
            if (db.isOpen()) {
                QSqlQuery q(db);
                q.prepare("SELECT project_code FROM public.tracked_test_batches WHERE batch_id = :batch_id::uuid");
                q.bindValue(":batch_id", batchId);
                if (q.exec() && q.next()) {
                    projectCode = q.value(0).toString().trimmed();
                    qDebug() << "AnalysisPipeline: Loaded project code:" << projectCode;
                }

                q.prepare("SELECT compound_name FROM public.tracked_test_compounds WHERE batch_id = :batch_id::uuid");
                q.bindValue(":batch_id", batchId);
                if (q.exec()) {
                    while (q.next()) {
                        QString name = q.value(0).toString().trimmed();
                        if (!name.isEmpty() && !dbCompounds.contains(name)) {
                            dbCompounds.append(name);
                        }
                    }
                }
            }
        } else {
            qWarning() << "AnalysisPipeline: Failed to load DB config:" << cfgErr;
        }
    }


    // 1) Read experiment.json if available
    QJsonObject expObj;
    QString expJsonPath = expJsonUrl.toLocalFile();
    if (expJsonPath.isEmpty()) {
        QString str = expJsonUrl.toString();
        if (str.startsWith("file://", Qt::CaseInsensitive)) {
            expJsonPath = QUrl(str).toLocalFile();
        } else {
            expJsonPath = str;
        }
    }
    
    if (!expJsonPath.isEmpty()) {
        QFile jsonFile(expJsonPath);
        if (jsonFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QJsonDocument doc = QJsonDocument::fromJson(jsonFile.readAll());
            if (doc.isObject()) {
                expObj = doc.object();
            }
            jsonFile.close();
        }
    }

    // Determine test ID and Plate Format before parsing layout
    QString testId = overrideTestId;
    if (testId.isEmpty() && expObj.contains("test_id")) {
        testId = expObj.value("test_id").toString();
    }
    
    int plateFormat = 96;
    QFile vocabFile(":/data/resources/data/tests_vocabulary.json");
    if (vocabFile.open(QIODevice::ReadOnly)) {
        QJsonObject vocab = QJsonDocument::fromJson(vocabFile.readAll()).object();
        if (vocab.value(testId).toString() == "384") {
            plateFormat = 384;
        }
    }

    // 2) Parse layout plates and match with raw files
    QList<MergedPlateData> plates;
    QMap<QString, QString> layoutToRawMap; // layout base name -> raw file path

    for (const QVariant& layoutUrlVar : layoutCsvs) {
        QString layoutFile = toLocalFileHelper(layoutUrlVar);
        QString layoutName = QFileInfo(layoutFile).baseName(); // e.g. "PHC006641"

        // Search for matching raw data file
        QString matchedRawFile;
        QStringList candidateRawFiles;
        for (const QVariant& rawUrlVar : rawDataCsvs) {
            QString rawFile = toLocalFileHelper(rawUrlVar);
            QString rawName = QFileInfo(rawFile).baseName();
            if (rawName.contains(layoutName, Qt::CaseInsensitive) || layoutName.contains(rawName, Qt::CaseInsensitive)) {
                candidateRawFiles.append(rawFile);
            }
        }

        if (!candidateRawFiles.isEmpty()) {
            // Prefer candidate containing selected timepoint
            for (const QString& cand : candidateRawFiles) {
                QString base = QFileInfo(cand).baseName();
                if (base.contains(timepoint, Qt::CaseInsensitive)) {
                    matchedRawFile = cand;
                    break;
                }
            }
            if (matchedRawFile.isEmpty()) {
                matchedRawFile = candidateRawFiles.first();
            }
        } else {
            // Fallbacks:
            if (rawDataCsvs.size() == 1) {
                matchedRawFile = toLocalFileHelper(rawDataCsvs.first());
            } else if (rawDataCsvs.size() == layoutCsvs.size()) {
                int idx = layoutCsvs.indexOf(layoutUrlVar);
                if (idx >= 0 && idx < rawDataCsvs.size()) {
                    matchedRawFile = toLocalFileHelper(rawDataCsvs.at(idx));
                }
            }
            
            if (matchedRawFile.isEmpty()) {
                result["error"] = QString("No matching raw data CSV found for layout plate: %1").arg(layoutName);
                return result;
            }
            qWarning() << "AnalysisPipeline: Using fallback raw file for layout" << layoutName << ":" << matchedRawFile;
        }

        layoutToRawMap[layoutName] = matchedRawFile;

        // Parse layout
        CsvTable layoutTable = parseCsv(layoutFile);
        int lWellIdx = findColumnIndex(layoutTable.headers, {"position", "well"});
        int lCmpIdx = findColumnIndex(layoutTable.headers, {"alias", "compound"});
        int lConcIdx = findColumnIndex(layoutTable.headers, {"concentration", "dose"});
        int lUnitIdx = findColumnIndex(layoutTable.headers, {"unit"});
        int lSolIdx = findColumnIndex(layoutTable.headers, {"solution", "value2"});
        int lProjIdx = findColumnIndex(layoutTable.headers, {"project", "value5"});
        int lTypeIdx = findColumnIndex(layoutTable.headers, {"type", "sampletype"});
        int lUserdef1Idx = findColumnIndex(layoutTable.headers, {"userdefvalue1", "value1"});

        if (lWellIdx < 0 || lCmpIdx < 0) {
            QString typeName = layoutUrlVar.typeName();
            QString varStr = layoutUrlVar.toString();
            QString urlStr = layoutUrlVar.toUrl().toString();
            result["error"] = QString("Layout CSV '%1.csv' is missing required columns (Well, Compound). [DEBUG: layoutFile='%2', type='%3', str='%4', urlStr='%5']")
                                .arg(layoutName).arg(layoutFile).arg(typeName).arg(varStr).arg(urlStr);
            return result;
        }

        MergedPlateData plateData;
        plateData.barcode = layoutName;

        for (const QStringList& row : layoutTable.rows) {
            if (row.size() <= std::max(lWellIdx, lCmpIdx)) continue;

            MergedWellData well;
            well.well = normalizeWellCoordinate(row[lWellIdx]);
            well.compound = row[lCmpIdx].trimmed();
            
            QString userdef1;
            if (lUserdef1Idx >= 0 && lUserdef1Idx < row.size()) {
                userdef1 = row[lUserdef1Idx].trimmed();
            }

            // Call parseWellTypeAndName
            QString baseName, type;
            int rep = 1;
            parseWellTypeAndName(well.compound, userdef1, baseName, type, rep);

            if (type == "qc") {
                well.cleanCompound = baseName + " (QC)";
            } else if (type == "standard") {
                well.cleanCompound = baseName + " (Standard)";
            } else {
                well.cleanCompound = baseName;
            }
            well.type = type;
            well.replicate = rep;

            if (lConcIdx >= 0 && lConcIdx < row.size()) {
                well.concentration = row[lConcIdx].toDouble();
            }
            if (lUnitIdx >= 0 && lUnitIdx < row.size()) {
                well.unit = row[lUnitIdx].trimmed();
            }
            if (lSolIdx >= 0 && lSolIdx < row.size()) {
                well.solutionId = row[lSolIdx].trimmed();
            }
            if (lProjIdx >= 0 && lProjIdx < row.size()) {
                well.projectId = row[lProjIdx].trimmed();
            }
            if (lTypeIdx >= 0 && lTypeIdx < row.size()) {
                // If explicit type is present in layout sheet, we can honor it
                QString explicitType = row[lTypeIdx].trimmed().toLower();
                if (!explicitType.isEmpty()) {
                    well.type = explicitType;
                }
            }

            // FILTER BY PROJECT:
            // If the well specifies a project and it doesn't match the active project code, exclude it!
            if (!well.projectId.isEmpty() && !projectCode.isEmpty()) {
                if (well.projectId.compare(projectCode, Qt::CaseInsensitive) != 0) {
                    continue; // Skip this well (from another project)
                }
            }

            plateData.wells.append(well);
        }

        // Parse raw data and merge
        CsvTable rawTable = parseCsv(matchedRawFile);
        int rWellIdx = findColumnIndex(rawTable.headers, {"well"});
        int rArenaIdx = findColumnIndex(rawTable.headers, {"arena"});
        int rRowIdx = findColumnIndex(rawTable.headers, {"row"});
        int rColIdx = findColumnIndex(rawTable.headers, {"col", "column"});
        int rMeanIdx = findColumnIndex(rawTable.headers, {"mean", "signal", "raw", "value", "rfu"});

        QMap<QString, double> rawValuesMap;
        for (const QStringList& row : rawTable.rows) {
            QString rawWell;
            if (rWellIdx >= 0 && rWellIdx < row.size()) {
                rawWell = normalizeWellCoordinate(row[rWellIdx]);
            } else if (rArenaIdx >= 0 && rArenaIdx < row.size()) {
                QString arenaVal = row[rArenaIdx].trimmed();
                if (!arenaVal.isEmpty() && arenaVal[0].isLetter()) {
                    rawWell = normalizeWellCoordinate(arenaVal);
                } else {
                    bool ok;
                    int arena = arenaVal.toInt(&ok);
                    if (ok) {
                        rawWell = arenaToWell(arena);
                    }
                }
            } else if (rRowIdx >= 0 && rColIdx >= 0 && rRowIdx < row.size() && rColIdx < row.size()) {
                QString rVal = row[rRowIdx].trimmed();
                QString cVal = row[rColIdx].trimmed();
                if (!rVal.isEmpty() && !cVal.isEmpty()) {
                    bool isNum = false;
                    int rNum = rVal.toInt(&isNum);
                    QString rowLet;
                    if (isNum && rNum > 0 && rNum <= 26) {
                        rowLet = QChar('A' + rNum - 1);
                    } else {
                        rowLet = rVal.toUpper();
                    }
                    
                    bool isColNum = false;
                    int cNum = cVal.toInt(&isColNum);
                    if (isColNum && cNum > 0) {
                        rawWell = QString("%1%2").arg(rowLet).arg(cNum, 2, 10, QChar('0'));
                    }
                }
            }
            if (rawWell.isEmpty()) continue;

            double val = getRawSignal(row, rMeanIdx);
            rawValuesMap[rawWell] = val;
        }

        // Assign raw signals to plate wells
        for (auto& well : plateData.wells) {
            if (rawValuesMap.contains(well.well)) {
                well.rawSignal = rawValuesMap[well.well];
            } else {
                well.rawSignal = -1.0; // Indicate missing data
            }
            if (plateFeedingValues.contains(plateData.barcode) && plateFeedingValues[plateData.barcode].contains(well.well)) {
                well.feedingDensity = plateFeedingValues[plateData.barcode][well.well];
                well.hasFeeding = true;
            }
        }

        // --- ADD MISSING WELLS ---
        int maxR = (plateFormat == 384) ? 16 : 8;
        int maxC = (plateFormat == 384) ? 24 : 12;

        for (int r = 0; r < maxR; ++r) {
            char rowChar = 'A' + r;
            for (int c = 1; c <= maxC; ++c) {
                QString w = QString("%1%2").arg(rowChar).arg(c, 2, 10, QChar('0'));
                bool found = false;
                for (auto& well : plateData.wells) {
                    if (well.well == w) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    MergedWellData well;
                    well.well = w;
                    well.compound = "Empty";
                    well.cleanCompound = "Empty";
                    well.type = "empty";
                    well.replicate = 1;
                    well.rawSignal = rawValuesMap.value(w, -1.0);
                    if (plateFeedingValues.contains(plateData.barcode) && plateFeedingValues[plateData.barcode].contains(w)) {
                        well.feedingDensity = plateFeedingValues[plateData.barcode][w];
                        well.hasFeeding = true;
                    }
                    plateData.wells.append(well);
                }
            }
        }

        // Sort wells to ensure consistent output order (e.g. A01, A02 ... P24)
        std::sort(plateData.wells.begin(), plateData.wells.end(), [](const MergedWellData& a, const MergedWellData& b) {
            if (a.well.length() != b.well.length()) return a.well.length() < b.well.length();
            return a.well < b.well;
        });

        plates.append(plateData);
    }

    // Calculate feeding reduction if feeding data was provided
    std::vector<double> placeboFeedingValues;
    for (const auto& plate : plates) {
        for (const auto& well : plate.wells) {
            if (well.hasFeeding && isPlaceboWell(well.compound, well.type)) {
                placeboFeedingValues.push_back(well.feedingDensity);
            }
        }
    }

    double referenceFeedingDensity = 0.0;
    if (placeboFeedingValues.size() >= 3) {
        referenceFeedingDensity = AnalysisMath::calculateMedian(placeboFeedingValues);
    } else {
        std::vector<double> allFeedingValues;
        for (const auto& plate : plates) {
            for (const auto& well : plate.wells) {
                if (well.hasFeeding) {
                    allFeedingValues.push_back(well.feedingDensity);
                }
            }
        }
        if (!allFeedingValues.empty()) {
            std::sort(allFeedingValues.begin(), allFeedingValues.end(), std::greater<double>());
            int count = std::max(3, static_cast<int>(std::ceil(allFeedingValues.size() * 0.1)));
            std::vector<double> topValues;
            for (int i = 0; i < std::min(count, static_cast<int>(allFeedingValues.size())); ++i) {
                topValues.push_back(allFeedingValues[i]);
            }
            referenceFeedingDensity = AnalysisMath::calculateMedian(topValues);
        }
    }

    if (referenceFeedingDensity > 0.0) {
        for (auto& plate : plates) {
            for (auto& well : plate.wells) {
                if (well.hasFeeding) {
                    double r = (1.0 - well.feedingDensity / referenceFeedingDensity) * 100.0;
                    well.feedingReduction = std::max(0.0, std::min(100.0, r));
                }
            }
        }
    }

    // Validate plateFormat against actual data max just in case
    int maxWells = 0;
    for (const auto& p : plates) {
        if (p.wells.size() > maxWells) maxWells = p.wells.size();
    }
    if (maxWells > 96 && plateFormat == 96) {
        plateFormat = 384;
    }

    // 3) Call module factory
    std::unique_ptr<IAnalysisModule> module = AnalysisModuleFactory::createModule(testId, plateFormat);
    if (!module) {
        result["error"] = "Failed to create analysis module for layout and plate format.";
        return result;
    }

    // Run Strategy
    QVariantMap strategyRes;
    bool success = module->runAnalysis(plates, expObj, timepoint, strategyRes);
    if (!success) {
        result["error"] = strategyRes.contains("error") ? strategyRes["error"].toString() : "Analysis strategy execution failed.";
        return result;
    }

    // 4) Write merged CSV files to raw data directory
    QVariantList mergedPaths;
    for (const auto& plate : plates) {
        QString matchedRawFile = layoutToRawMap.value(plate.barcode);
        QString rawDir = QFileInfo(matchedRawFile).absolutePath();
        QString mergedFilePath = rawDir + "/merged_" + plate.barcode + ".csv";

        QFile outFile(mergedFilePath);
        if (outFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream stream(&outFile);
            
            bool hasFeedingData = false;
            for (const auto& well : plate.wells) {
                if (well.hasFeeding) {
                    hasFeedingData = true;
                    break;
                }
            }

            if (hasFeedingData) {
                stream << "Well,Compound,Concentration_uM,RawSignal_RFU,Efficacy_Percent,Feeding_Density_Percent,Feeding_Reduction_Percent\n";
                for (const auto& well : plate.wells) {
                    stream << well.well << ","
                           << well.compound << ","
                           << QString::number(well.concentration, 'f', 6) << ","
                           << QString::number(well.rawSignal, 'f', 2) << ","
                           << QString::number(well.efficacy, 'f', 2) << ","
                           << QString::number(well.feedingDensity, 'f', 2) << ","
                           << QString::number(well.feedingReduction, 'f', 2) << "\n";
                }
            } else {
                stream << "Well,Compound,Concentration_uM,RawSignal_RFU,Efficacy_Percent\n";
                for (const auto& well : plate.wells) {
                    stream << well.well << ","
                           << well.compound << ","
                           << QString::number(well.concentration, 'f', 6) << ","
                           << QString::number(well.rawSignal, 'f', 2) << ","
                           << QString::number(well.efficacy, 'f', 2) << "\n";
                }
            }
            outFile.close();
            mergedPaths.append(QUrl::fromLocalFile(mergedFilePath).toString());
            qDebug() << "AnalysisPipeline: Exported merged CSV to:" << mergedFilePath;
        } else {
            qWarning() << "AnalysisPipeline: Failed to write merged CSV:" << mergedFilePath;
        }
    }

    // 5) Ensure ALL compounds from the test request are listed in the results (fill in placeholders if missing)
    QVariantList compList = strategyRes["compounds"].toList();
    QStringList existingNames;
    for (const QVariant& cv : compList) {
        existingNames.append(getCleanName(cv.toMap().value("compound_name").toString()).toUpper());
    }

    for (const QString& dbComp : dbCompounds) {
        QString cleanName = getCleanName(dbComp);
        if (cleanName.isEmpty()) continue;
        if (isPlaceboWell(cleanName, "") || isStandardWell(cleanName, "") || isQcWell(cleanName, "")) continue;

        if (!existingNames.contains(cleanName.toUpper())) {
            QVariantMap cmpMap;
            cmpMap["compound_name"] = cleanName;
            cmpMap["dose_responses"] = QVariantList();
            cmpMap["ec50"] = 0.0;
            cmpMap["status"] = "No Data";
            cmpMap["is_standard"] = false;
            cmpMap["is_qc"] = false;
            compList.append(cmpMap);
        }
    }
    strategyRes["compounds"] = compList;

    // Build final output
    result = strategyRes;
    result["merged_files"] = mergedPaths;
    result["plate_format"] = plateFormat;
    result["success"] = true;

    return result;
}
