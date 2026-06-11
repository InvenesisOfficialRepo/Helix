#include "FeedingAnalysisProcessor.h"

#include <QDebug>
#include <QImage>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QPointF>
#include <QVector>
#include <QMediaPlayer>
#include <QVideoSink>
#include <QVideoFrame>
#include <QEventLoop>
#include <QTimer>
#include <cmath>

FeedingAnalysisProcessor::FeedingAnalysisProcessor(QObject *parent)
    : QObject(parent)
{
}

QString FeedingAnalysisProcessor::extractFrameFromVideo(const QString& videoPath, double timeInSeconds)
{
    qDebug() << "FeedingAnalysisProcessor: Extracting frame from" << videoPath << "at" << timeInSeconds << "seconds";
    
    QMediaPlayer player;
    QVideoSink sink;
    player.setVideoSink(&sink);
    
    QUrl videoUrl;
    if (videoPath.startsWith("qrc:/") || videoPath.startsWith("file:///")) {
        videoUrl = QUrl(videoPath);
    } else {
        videoUrl = QUrl::fromLocalFile(videoPath);
    }
    player.setSource(videoUrl);
    
    QEventLoop loop;
    QImage frameImage;
    bool frameCaptured = false;
    
    // Connect to videoSink's frameChanged signal
    QObject::connect(&sink, &QVideoSink::videoFrameChanged, [&](const QVideoFrame& frame) {
        if (!frameCaptured && frame.isValid()) {
            frameImage = frame.toImage();
            if (!frameImage.isNull()) {
                frameCaptured = true;
                loop.quit();
            }
        }
    });

    // Start loading and seek to the target time in milliseconds
    player.setPosition(static_cast<qint64>(timeInSeconds * 1000.0));
    player.play(); // Play to trigger frame decoding
    
    // Safety timeout after 5 seconds to prevent hanging
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start(5000); 
    
    loop.exec();
    player.stop();
    
    if (frameImage.isNull()) {
        qWarning() << "FeedingAnalysisProcessor: Failed to extract frame from video:" << videoPath;
        return "";
    }
    
    // Save the frame to a temporary PNG file in QDir::tempPath()
    QString tempDir = QDir::tempPath();
    QString baseName = QFileInfo(videoPath).baseName();
    QString fileName = baseName + "_frame.png";
    QString outPath = tempDir + "/" + fileName;
    
    if (frameImage.save(outPath, "PNG")) {
        qDebug() << "FeedingAnalysisProcessor: Saved extracted frame to" << outPath;
        return outPath;
    } else {
        qWarning() << "FeedingAnalysisProcessor: Failed to save extracted frame to" << outPath;
        return "";
    }
}

QVariantMap FeedingAnalysisProcessor::processPlateImage(const QString& imagePath,
                                                        const QPointF& a1,
                                                        const QPointF& h12,
                                                        double radius,
                                                        double innerRatio,
                                                        const QString& threshMode,
                                                        int fixedThresh,
                                                        const QString& polarity)
{
    qDebug() << "FeedingAnalysisProcessor: Processing image:" << imagePath;
    QVariantMap result;

    QImage img;
    // Handle file URL prefixes if any
    QString localPath = imagePath;
    if (localPath.startsWith("file:///")) {
        localPath = localPath.mid(8);
    } else if (localPath.startsWith("file://")) {
        localPath = localPath.mid(7);
    }
    
    if (!img.load(localPath)) {
        qWarning() << "FeedingAnalysisProcessor: Failed to load image:" << localPath;
        return result;
    }

    int width = img.width();
    int height = img.height();
    
    int rows = 8;
    int cols = 12;
    double dx = (h12.x() - a1.x()) / 11.0;
    double dy = (h12.y() - a1.y()) / 7.0;
    double innerR = radius * innerRatio;
    double innerR2 = innerR * innerR;

    // A) If global Otsu threshold per plate is selected, compute global histogram first
    int plateThreshold = fixedThresh;
    if (threshMode == "otsu-plate") {
        QVector<int> globalHist(256, 0);
        int totalPlatePixels = 0;

        for (int r = 0; r < rows; ++r) {
            for (int c = 0; c < cols; ++c) {
                QPointF p(a1.x() + dx * c, a1.y() + dy * r);
                int x0 = std::max(0, static_cast<int>(std::round(p.x() - innerR)));
                int y0 = std::max(0, static_cast<int>(std::round(p.y() - innerR)));
                int x1 = std::min(width - 1, static_cast<int>(std::round(p.x() + innerR)));
                int y1 = std::min(height - 1, static_cast<int>(std::round(p.y() + innerR)));

                for (int yy = y0; yy <= y1; ++yy) {
                    for (int xx = x0; xx <= x1; ++xx) {
                        double dist2 = (xx - p.x()) * (xx - p.x()) + (yy - p.y()) * (yy - p.y());
                        if (dist2 <= innerR2) {
                            QRgb pixel = img.pixel(xx, yy);
                            double lum = 0.299 * qRed(pixel) + 0.587 * qGreen(pixel) + 0.114 * qBlue(pixel);
                            int li = std::min(255, std::max(0, static_cast<int>(std::round(lum))));
                            globalHist[li]++;
                            totalPlatePixels++;
                        }
                    }
                }
            }
        }
        if (totalPlatePixels > 0) {
            plateThreshold = calculateOtsuThreshold(globalHist, totalPlatePixels);
            qDebug() << "FeedingAnalysisProcessor: Global plate Otsu threshold =" << plateThreshold;
        }
    }

    // B) Compute density for each well
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            QPointF p(a1.x() + dx * c, a1.y() + dy * r);
            QString wellName = QString("%1%2")
                                   .arg(QChar('A' + r))
                                   .arg(c + 1, 2, 10, QChar('0')); // e.g., "A01", "H12"

            int x0 = std::max(0, static_cast<int>(std::round(p.x() - innerR)));
            int y0 = std::max(0, static_cast<int>(std::round(p.y() - innerR)));
            int x1 = std::min(width - 1, static_cast<int>(std::round(p.x() + innerR)));
            int y1 = std::min(height - 1, static_cast<int>(std::round(p.y() + innerR)));

            QVector<int> wellHist(256, 0);
            QVector<int> wellLums;
            int totalWellPixels = 0;

            for (int yy = y0; yy <= y1; ++yy) {
                for (int xx = x0; xx <= x1; ++xx) {
                    double dist2 = (xx - p.x()) * (xx - p.x()) + (yy - p.y()) * (yy - p.y());
                    if (dist2 <= innerR2) {
                        QRgb pixel = img.pixel(xx, yy);
                        double lum = 0.299 * qRed(pixel) + 0.587 * qGreen(pixel) + 0.114 * qBlue(pixel);
                        int li = std::min(255, std::max(0, static_cast<int>(std::round(lum))));
                        wellHist[li]++;
                        wellLums.append(li);
                        totalWellPixels++;
                    }
                }
            }

            int thr = fixedThresh;
            if (threshMode == "otsu-well") {
                if (totalWellPixels > 0) {
                    thr = calculateOtsuThreshold(wellHist, totalWellPixels);
                }
            } else if (threshMode == "otsu-plate") {
                thr = plateThreshold;
            }

            int matchingPixels = 0;
            if (polarity == "dark") {
                for (int lum : wellLums) {
                    if (lum <= thr) matchingPixels++;
                }
            } else {
                for (int lum : wellLums) {
                    if (lum >= thr) matchingPixels++;
                }
            }

            double density = totalWellPixels > 0 ? (static_cast<double>(matchingPixels) / totalWellPixels) * 100.0 : 0.0;
            result[wellName] = density;
        }
    }

    return result;
}

QString FeedingAnalysisProcessor::writeTemporaryRawCsv(const QString& barcode, const QVariantMap& wellDensities)
{
    QString tempDir = QDir::tempPath();
    QString filePath = tempDir + "/raw_" + barcode + ".csv";
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << "Well,Mean\n";
        
        // Write wells in sorted order
        QStringList sortedKeys = wellDensities.keys();
        sortedKeys.sort();
        
        for (const QString& well : sortedKeys) {
            double val = wellDensities.value(well).toDouble();
            out << well << "," << QString::number(val, 'f', 4) << "\n";
        }
        file.close();
        qDebug() << "FeedingAnalysisProcessor: Wrote temporary raw CSV to:" << filePath;
        return filePath;
    } else {
        qWarning() << "FeedingAnalysisProcessor: Failed to write temporary CSV:" << filePath;
        return "";
    }
}

int FeedingAnalysisProcessor::calculateOtsuThreshold(const QVector<int>& hist, int total)
{
    double sum = 0;
    for (int i = 0; i < 256; ++i) {
        sum += i * hist[i];
    }
    
    double sumB = 0;
    int wB = 0;
    int wF = 0;
    double maxVar = -1;
    int threshold = 127;
    
    for (int t = 0; t < 256; ++t) {
        wB += hist[t];
        if (wB == 0) continue;
        wF = total - wB;
        if (wF == 0) break;
        
        sumB += t * hist[t];
        double mB = sumB / wB;
        double mF = (sum - sumB) / wF;
        double between = static_cast<double>(wB) * wF * (mB - mF) * (mB - mF);
        
        if (between > maxVar) {
            maxVar = between;
            threshold = t;
        }
    }
    return threshold;
}
