#pragma once

#include <QObject>
#include <QString>
#include <QVariant>
#include <QMap>
#include <QPointF>

class FeedingAnalysisProcessor : public QObject
{
    Q_OBJECT
public:
    explicit FeedingAnalysisProcessor(QObject *parent = nullptr);

    /**
     * Extracts a single frame from an MP4 video at the specified timestamp (in seconds)
     * and saves it to a temporary PNG file.
     * @param videoPath Absolute local path to the MP4 file.
     * @param timeInSeconds Timestamp in seconds (e.g. 0.5).
     * @return Path to the saved temporary PNG file.
     */
    Q_INVOKABLE QString extractFrameFromVideo(const QString& videoPath, double timeInSeconds);

    /**
     * Calibrates the grid and calculates the percentage of dark pixels (feeding density) for each well.
     * @param imagePath Path to the plate PNG/JPG frame.
     * @param a1 Clicked center of well A1.
     * @param h12 Clicked center of well H12.
     * @param radius Radius of a well in pixels.
     * @param innerRatio Inner sampling ratio (e.g. 0.8 to sample the inner 80%).
     * @param threshMode Thresholding mode ("otsu-well", "otsu-plate", "fixed").
     * @param fixedThresh Fixed threshold value (0-255).
     * @param polarity Pixel polarity to count ("dark" or "light").
     * @return QVariantMap mapping well coordinates (e.g. "A01", "A02"...) to computed feeding density percentage (0-100).
     */
    Q_INVOKABLE QVariantMap processPlateImage(const QString& imagePath,
                                              const QPointF& a1,
                                              const QPointF& h12,
                                              double radius,
                                              double innerRatio,
                                              const QString& threshMode,
                                              int fixedThresh,
                                              const QString& polarity);

    /**
     * Helper to write computed raw values for a plate to a temporary CSV file.
     * @param barcode The plate barcode.
     * @param wellDensities Map of well coordinates to density values.
     * @return The absolute path to the generated temporary CSV file.
     */
    Q_INVOKABLE QString writeTemporaryRawCsv(const QString& barcode, const QVariantMap& wellDensities);

private:
    int calculateOtsuThreshold(const QVector<int>& hist, int total);
};
