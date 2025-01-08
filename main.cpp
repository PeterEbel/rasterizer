#include <algorithm> // Für std::sort
#include <vector>
#include <QFile>
#include <QTextStream>
#include <cmath>
#include <QCommandLineParser>
#include <QDebug>
#include <QApplication>
#include <QFileDialog>
#include <QImage>
#include <QVector>
#include <QMap>
#include <algorithm> // Für std::sort
#include <vector>
#include <QtConcurrent>
#include <QFuture>
#include <QElapsedTimer> // Für Performance-Messungen

QVector<QColor> scanImageToMatrixMedian(const QString& imagePath, int rasterBreite, int rasterHoehe, int& bildBreite, int& bildHoehe) {

    QElapsedTimer timer;
    timer.start();

    QImage image(imagePath);
    if (image.isNull()) {
        qDebug() << "Fehler beim Laden des Bildes: " << imagePath;
        return QVector<QColor>(); // Leeren Vektor zurückgeben
    }

    bildBreite = image.width();
    bildHoehe = image.height();

    int matrixBreite = bildBreite / rasterBreite;
    int matrixHoehe = bildHoehe / rasterHoehe;
    QVector<QColor> farbMatrix(matrixBreite * matrixHoehe);
    QList<QFuture<QColor>> futures;

    for (int y = 0; y < bildHoehe; y += rasterHoehe) {
        for (int x = 0; x < bildBreite; x += rasterBreite) {
            futures.append(QtConcurrent::run([=]() -> QColor {
                QRect rect(x, y, std::min(rasterBreite, bildBreite - x), std::min(rasterHoehe, bildHoehe - y));
                QImage subImage = image.copy(rect); // Kopie des Bildausschnitts!

                if (subImage.isNull()){
                    return QColor();
                }

                std::vector<int> redValues;
                std::vector<int> greenValues;
                std::vector<int> blueValues;

                for (int iy = 0; iy < subImage.height(); ++iy) {
                    for (int ix = 0; ix < subImage.width(); ++ix) {
                        QRgb rgb = subImage.pixel(ix, iy);
                        redValues.push_back(qRed(rgb));
                        greenValues.push_back(qGreen(rgb));
                        blueValues.push_back(qBlue(rgb));
                    }
                }
                if (!redValues.empty()) {
                    std::sort(redValues.begin(), redValues.end());
                    std::sort(greenValues.begin(), greenValues.end());
                    std::sort(blueValues.begin(), blueValues.end());

                    int medianRed = redValues[redValues.size() / 2];
                    int medianGreen = greenValues[greenValues.size() / 2];
                    int medianBlue = blueValues[blueValues.size() / 2];
                    return QColor(medianRed, medianGreen, medianBlue);
                } else {
                    return QColor();
                }

            }));
        }
    }

    int index = 0;
    for(auto & future : futures){
        farbMatrix[index] = future.result();
        index++;
    }

    qDebug() << "scanImageToMatrixMedian benötigte:" << timer.elapsed() << "Millisekunden";
    return farbMatrix;
}


QVector<QColor> scanImageToMatrixAverage(const QString& imagePath, int rasterBreite, int rasterHoehe, int& bildBreite, int& bildHoehe) {

    QElapsedTimer timer;
    timer.start();

    QImage image(imagePath);
    if (image.isNull()) {
        qDebug() << "Fehler beim Laden des Bildes: " << imagePath;
        return QVector<QColor>(); // Leeren Vektor zurückgeben
    }

    bildBreite = image.width();
    bildHoehe = image.height();

    int matrixBreite = bildBreite / rasterBreite;
    int matrixHoehe = bildHoehe / rasterHoehe;
    QVector<QColor> farbMatrix(matrixBreite * matrixHoehe);
    QList<QFuture<QColor>> futures;


    for (int y = 0; y < bildHoehe; y += rasterHoehe) {
        for (int x = 0; x < bildBreite; x += rasterBreite) {
            futures.append(QtConcurrent::run([=]() -> QColor {
                QRect rect(x, y, std::min(rasterBreite, bildBreite - x), std::min(rasterHoehe, bildHoehe - y));
                QImage subImage = image.copy(rect); // Kopie des Bildausschnitts!

                if (subImage.isNull()){
                    return QColor();
                }

                long long redSum = 0;
                long long greenSum = 0;
                long long blueSum = 0;
                int pixelCount = 0;

                for (int iy = 0; iy < subImage.height(); ++iy) {
                    for (int ix = 0; ix < subImage.width(); ++ix) {
                        QRgb rgb = subImage.pixel(ix, iy);
                        redSum += qRed(rgb);
                        greenSum += qGreen(rgb);
                        blueSum += qBlue(rgb);
                        pixelCount++;
                    }
                }

                if (pixelCount > 0) {
                    int averageRed = redSum / pixelCount;
                    int averageGreen = greenSum / pixelCount;
                    int averageBlue = blueSum / pixelCount;
                    return QColor(averageRed, averageGreen, averageBlue);
                } else {
                    return QColor();
                }
            }));
        }
    }

    int index = 0;
    for(auto & future : futures){
        farbMatrix[index] = future.result();
        index++;
    }

    qDebug() << "scanImageToMatrixAverage benötigte:" << timer.elapsed() << "Millisekunden";
    return farbMatrix;
}

/* Single-Thread Version von scanImageToMatrix
QVector<QColor> scanImageToMatrixMedian(const QString& imagePath, int rasterBreite, int rasterHoehe, int& bildBreite, int& bildHoehe) {

    QElapsedTimer timer;
    timer.start();

    QImage image(imagePath);

    if (image.isNull()) {
        qWarning() << "Bild konnte nicht geladen werden:" << imagePath;
        return QVector<QColor>(); // Leere Matrix zurückgeben
    }

    bildBreite = image.width();
    bildHoehe = image.height();

    QVector<QColor> farbMatrix;
    for (int y = 0; y < bildHoehe; y += rasterHoehe) {
        for (int x = 0; x < bildBreite; x += rasterBreite) {
            std::vector<int> redValues;
            std::vector<int> greenValues;
            std::vector<int> blueValues;

            for (int iy = 0; iy < rasterHoehe && y + iy < bildHoehe; ++iy) {
                for (int ix = 0; ix < rasterBreite && x + ix < bildBreite; ++ix) {
                    QRgb rgb = image.pixel(x + ix, y + iy);
                    redValues.push_back(qRed(rgb));
                    greenValues.push_back(qGreen(rgb));
                    blueValues.push_back(qBlue(rgb));
                }
            }

            if (!redValues.empty()) {
                std::sort(redValues.begin(), redValues.end());
                std::sort(greenValues.begin(), greenValues.end());
                std::sort(blueValues.begin(), blueValues.end());

                int size;
                int medianRed;
                size = redValues.size();
                if (size % 2 == 0) { // Gerade Anzahl
                    medianRed = (redValues[size / 2 - 1] + redValues[size / 2]) / 2;
                } else { // Ungerade Anzahl
                    medianRed = redValues[size / 2];
                }

                int medianGreen;
                size = greenValues.size();
                if (size % 2 == 0) { // Gerade Anzahl
                    medianGreen = (greenValues[size / 2 - 1] + greenValues[size / 2]) / 2;
                } else { // Ungerade Anzahl
                    medianGreen = greenValues[size / 2];
                }

                int medianBlue;
                size = blueValues.size();
                if (size % 2 == 0) { // Gerade Anzahl
                    medianBlue = (blueValues[size / 2 - 1] + blueValues[size / 2]) / 2;
                } else { // Ungerade Anzahl
                    medianBlue = blueValues[size / 2];
                }

                QColor medianColor(medianRed, medianGreen, medianBlue);
                farbMatrix.append(medianColor);
            }
        }
    }

    qDebug() << "scanImageToMatrixMedian benötigte:" << timer.elapsed() << "Millisekunden";
    return farbMatrix;
}

QVector<QColor> scanImageToMatrixAverage(const QString& imagePath, int rasterBreite, int rasterHoehe, int& bildBreite, int& bildHoehe) {

    QElapsedTimer timer;
    timer.start();

    QImage image(imagePath);

    if (image.isNull()) {
        qDebug() << "Bild konnte nicht geladen werden: " << imagePath;
        return QVector<QColor>();
    }

    bildBreite = image.width();
    bildHoehe = image.height();

    QVector<QColor> farbMatrix;

    for (int y = 0; y < bildHoehe; y += rasterHoehe) {
        for (int x = 0; x < bildBreite; x += rasterBreite) {
            int rSum = 0;
            int gSum = 0;
            int bSum = 0;
            int pixelCount = 0;

            for (int iy = 0; iy < rasterHoehe && y + iy < bildHoehe; ++iy) {
                for (int ix = 0; ix < rasterBreite && x + ix < bildBreite; ++ix) {
                    QRgb rgb = image.pixel(x + ix, y + iy);
                    rSum += qRed(rgb);
                    gSum += qGreen(rgb);
                    bSum += qBlue(rgb);
                    pixelCount++;
                }
            }

            if (pixelCount > 0) {
                QColor avgColor(rSum / pixelCount, gSum / pixelCount, bSum / pixelCount);
                farbMatrix.append(avgColor);
            } else {
                farbMatrix.append(QColor(Qt::white));
            }
        }
    }

    qDebug() << "scanImageToMatrixAverage benötigte:" << timer.elapsed() << "Millisekunden";
    return farbMatrix;
}
*/

int getPaletteIndex(int helligkeit, const QVector<QColor>& palette) {
    return helligkeit * (palette.size() - 1) / 255;
}

void createSvgFromColorMatrix(const QVector<QColor>& farbMatrix, int bildBreite, int bildHoehe, int rasterBreite, int rasterHoehe, const QString& svgFileName, const QString& scalingMode, bool grayscale, bool blackCircles, const QVector<QColor>& activePalette, bool usePalette, double cornerCoverageFactor) {

    QFile svgFile(svgFileName);
    if (!svgFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "Konnte SVG-Datei nicht öffnen.";
        return;
    }

    QTextStream svgStream(&svgFile);
    svgStream.setEncoding(QStringConverter::Utf8);

    svgStream << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n";
    svgStream << "<svg width=\"" << bildBreite << "\" height=\"" << bildHoehe << "\" version=\"1.1\" xmlns=\"http://www.w3.org/2000/svg\">\n";
    svgStream << "<rect id=\"background\" width=\"" << bildBreite << "\" height=\"" << bildHoehe << "\" fill=\"white\" />\n";
    svgStream << "<g id=\"circles\">\n";

    int index = 0;
    for (int y = 0; y < bildHoehe; y += rasterHoehe) {
        for (int x = 0; x < bildBreite; x += rasterBreite) {

            if (index < farbMatrix.size()) {

                QColor color = farbMatrix.at(index);
                int helligkeit = qGray(color.rgb());
                helligkeit = 255 - helligkeit;

                double maxRadius = std::min(rasterBreite, rasterHoehe) / 2.0 * cornerCoverageFactor;
                double radius = 0;

                if (scalingMode == "linear") {
                    radius = (double) helligkeit / 255.0 * maxRadius;
                } else if (scalingMode == "sqrt") {
                    radius = std::sqrt((double) helligkeit / 255.0) * maxRadius;
                } else if (scalingMode == "log") {
                    if (helligkeit > 0) {
                        radius = std::log((double) helligkeit + 1) / std::log(256.0) * maxRadius;
                    } else {
                        radius = 0.0;
                    }
                } else if (scalingMode == "konstant") {
                    radius = maxRadius;
                } else {
                    qDebug() << "Ungültiger Skalierungsmodus: " << scalingMode;
                    radius = maxRadius;
                }

                int centerX = x + rasterBreite / 2;
                int centerY = y + rasterHoehe / 2;

                if (blackCircles) {
                    svgStream << "<circle cx=\"" << centerX << "\" cy=\"" << centerY << "\" r=\"" << radius << "\" fill=\"black\" />\n";
                } else if (grayscale) {
                    svgStream << "<circle cx=\"" << centerX << "\" cy=\"" << centerY << "\" r=\"" << radius << "\" fill=\"rgb(" << 255 - helligkeit << "," << 255 - helligkeit << "," << 255 - helligkeit << ")\" />\n"; // Invertierung auch hier für Graustufen
                } else if (usePalette) { // Überprüfung, ob eine Palette verwendet werden soll
                    int paletteIndex = getPaletteIndex(helligkeit, activePalette);
                    QColor paletteColor = activePalette.at(paletteIndex);
                    svgStream << "<circle cx=\"" << centerX << "\" cy=\"" << centerY << "\" r=\"" << radius << "\" fill=\"rgb(" << paletteColor.red() << "," << paletteColor.green() << "," << paletteColor.blue() << ")\" />\n";
                } else { // Keine Palette verwenden: Direkte Verwendung der Farbe aus der Farbmatrix
                    svgStream << "<circle cx=\"" << centerX << "\" cy=\"" << centerY << "\" r=\"" << radius << "\" fill=\"rgb(" << color.red() << "," << color.green() << "," << color.blue() << ")\" />\n";
                }

                index++;
            }
        }
    }

    svgStream << "</g>\n";
    svgStream << "</svg>\n";
    svgFile.close();
}


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription("Erzeugt SVG-Grafiken aus Bildern.");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("bild", "Das zu rasternde Bild");

    QCommandLineOption rasterBreiteOption(QStringList() << "x" << "raster-breite", "Breite des Rasters", "breite", "10");
    parser.addOption(rasterBreiteOption);
    QCommandLineOption rasterHoeheOption(QStringList() << "y" << "raster-hoehe", "Höhe des Rasters", "hoehe", "10");
    parser.addOption(rasterHoeheOption);
    QCommandLineOption outputOption(QStringList() << "o" << "ausgabe", "Ausgabedatei", "datei", "output.svg");
    parser.addOption(outputOption);
    QCommandLineOption scalingModeOption(QStringList() << "s" << "skalierung", "Skalierungsmodus (linear, sqrt, log, konstant).", "modus", "linear");
    parser.addOption(scalingModeOption);
    QCommandLineOption grayscaleOption(QStringList() << "g" << "graustufen", "Verwendet Graustufen statt Farben");
    parser.addOption(grayscaleOption);
    QCommandLineOption blackCirclesOption(QStringList() << "B" << "schwarze-kreise", "Erzeugt schwarze Kreise");
    parser.addOption(blackCirclesOption);
    QCommandLineOption paletteOption(QStringList() << "p" << "palette", "Zu verwendende Farbpalette (palette1, palette2, palette3, grayscale).", "palette", "");
    parser.addOption(paletteOption);
    QCommandLineOption medianOption(QStringList() << "m" << "median", "Verwendet den Mittelwert zur Farbbestimmung anstelle des Durchschnitts");
    parser.addOption(medianOption);
    QCommandLineOption cornerCoverageOption(QStringList() << "e" << "eckabdeckung", "Faktor für die Eckabdeckung (z.B. 1.0, 1.3).", "faktor", "1.0");
    parser.addOption(cornerCoverageOption);

    parser.process(a.arguments());

    const QStringList args = parser.positionalArguments();
    if (args.isEmpty()) {
        parser.showHelp();
        return 1;
    }

    QString imagePath = args.first();
    int rasterBreite = parser.value(rasterBreiteOption).toInt();
    int rasterHoehe = parser.value(rasterHoeheOption).toInt();
    QString svgFileName = parser.value(outputOption);
    QString scalingMode = parser.value(scalingModeOption);
    bool grayscale = parser.isSet(grayscaleOption);
    bool blackCircles = parser.isSet(blackCirclesOption);

    int bildBreite = 0;
    int bildHoehe = 0;

    bool useMedian = parser.isSet(medianOption);

    QVector<QColor> farbMatrix;
    if (useMedian) {
        farbMatrix = scanImageToMatrixMedian(imagePath, rasterBreite, rasterHoehe, bildBreite, bildHoehe);
        qDebug() << "Verwende Medianberechnung";
    } else {
        farbMatrix = scanImageToMatrixAverage(imagePath, rasterBreite, rasterHoehe, bildBreite, bildHoehe);
        qDebug()  << "Verwende Durchschnittsberechnung";
    }

    if (farbMatrix.isEmpty()) {
        return 1;
    }

    bool usePalette = parser.isSet(paletteOption); // Überprüfen, ob die Option gesetzt ist
    QString paletteName;
    if (usePalette) {
        paletteName = parser.value(paletteOption);
    }

    QMap<QString, QVector<QColor>> palettes;
    palettes["palette1"] = {Qt::red, Qt::green, Qt::blue};
    palettes["palette2"] = {QColor("#FF69B4"), QColor("#00FFFF"), QColor("#90EE90"), QColor("#FFA500")};
    palettes["palette3"] = {QColor("#264653"), QColor("#2A9D8F"), QColor("#E9C46A"), QColor("#F4A261"), QColor("#E76F51")};
    palettes["grayscale"] = {Qt::black, Qt::darkGray, Qt::gray, Qt::lightGray, Qt::white};

    QVector<QColor> activePalette;
    if (usePalette) {
       if (palettes.contains(paletteName)) {
           activePalette = palettes[paletteName];
       } else {
           qDebug() << "Ungültige Palette: " << paletteName << ". Verwende Standardpalette (palette3).";
           activePalette = palettes["palette3"];
       }
    }

    double cornerCoverageFactor = 1.0; // Standardwert
    bool ok;

    if (!parser.value(cornerCoverageOption).isEmpty() && parser.isSet(cornerCoverageOption)) {
        cornerCoverageFactor = QLocale::c().toDouble(parser.value(cornerCoverageOption), &ok);
        if (!ok) {
            qDebug() << "Corner-Coverage-Faktor: Ungültiger Wert. Verwende Standardwert 1.0";
            cornerCoverageFactor = 1.0;
        }
    } else {
        qDebug() << "Corner-Coverage-Faktor: Standardwert verwendet: 1.0";
    }

    if (cornerCoverageFactor <= 0) {
        qDebug() << "Corner-Coverage-Faktor: Muss größer als 0 sein. Verwende Standardwert 1.0.";
        cornerCoverageFactor = 1.0;
    } else if (cornerCoverageFactor > 2.0) {
        qDebug() << "Corner-Coverage-Faktor: Wert ist zu gross. Verwende Maximalwert 2.0.";
        cornerCoverageFactor = 2.0;
    } else {
        qDebug() << "Corner-Coverage-Faktor: Verwende Wert:" << cornerCoverageFactor;
    }

    createSvgFromColorMatrix(farbMatrix, bildBreite, bildHoehe, rasterBreite, rasterHoehe, svgFileName, scalingMode, grayscale, blackCircles, activePalette, usePalette, cornerCoverageFactor);
    qDebug() << "SVG-Datei erfolgreich erzeugt.";

    return(0);
}
