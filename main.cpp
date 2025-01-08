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

QVector<QColor> scanImageToMatrixMedian(const QString& imagePath, int rasterBreite, int rasterHoehe, int& bildBreite, int& bildHoehe) {

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
    return farbMatrix;
}

QVector<QColor> scanImageToMatrixAverage(const QString& imagePath, int rasterBreite, int rasterHoehe, int& bildBreite, int& bildHoehe) {

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
    return farbMatrix;
}

int getPaletteIndex(int helligkeit, const QVector<QColor>& palette) {

    return helligkeit * (palette.size() - 1) / 255;

}

void createSvgFromColorMatrix(const QVector<QColor>& farbMatrix, int bildBreite, int bildHoehe, int rasterBreite, int rasterHoehe, const QString& svgFileName, const QString& scalingMode, bool grayscale, bool blackCircles, const QVector<QColor>& activePalette, bool usePalette) {

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

    double cornerCoverageFactor = 1.3;
    double maxRadiusFactor = blackCircles ? cornerCoverageFactor : 1.2;

    int index = 0;
    for (int y = 0; y < bildHoehe; y += rasterHoehe) {
        for (int x = 0; x < bildBreite; x += rasterBreite) {

            if (index < farbMatrix.size()) {

                QColor color = farbMatrix.at(index);
                int helligkeit = qGray(color.rgb());
                helligkeit = 255 - helligkeit;

                double maxRadius = std::min(rasterBreite, rasterHoehe) / 2.0 * maxRadiusFactor;
                double radius = 0;

                if (scalingMode == "linear") {
                    radius = (double)helligkeit / 255.0 * maxRadius;
                } else if (scalingMode == "sqrt") {
                    radius = std::sqrt((double)helligkeit / 255.0) * maxRadius;
                } else if (scalingMode == "logarithmic") {
                    if (helligkeit > 0) {
                        radius = std::log((double)helligkeit + 1) / std::log(256.0) * maxRadius;
                    } else {
                        radius = 0;
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
    parser.addPositionalArgument("bild", "Das Eingabebild.");

    QCommandLineOption rasterBreiteOption(QStringList() << "b" << "raster-breite", "Die Breite des Rasters.", "breite", "10");
    parser.addOption(rasterBreiteOption);
    QCommandLineOption rasterHoeheOption(QStringList() << "t" << "raster-hoehe", "Die Höhe des Rasters.", "hoehe", "10");
    parser.addOption(rasterHoeheOption);
    QCommandLineOption outputOption(QStringList() << "o" << "ausgabe", "Die Ausgabedatei.", "datei", "output.svg");
    parser.addOption(outputOption);
    QCommandLineOption scalingModeOption(QStringList() << "s" << "skalierung", "Der Skalierungsmodus (linear, sqrt, logarithmic, konstant).", "modus", "linear");
    parser.addOption(scalingModeOption);
    QCommandLineOption grayscaleOption(QStringList() << "g" << "graustufen", "Verwendet Graustufen.");
    parser.addOption(grayscaleOption);
    QCommandLineOption blackCirclesOption(QStringList() << "B" << "schwarze-kreise", "Verwendet nur schwarze Kreise.");
    parser.addOption(blackCirclesOption);
    QCommandLineOption paletteOption(QStringList() << "p" << "palette", "Die zu verwendende Farbpalette (palette1, palette2, palette3, grayscale).", "palette", "");
    parser.addOption(paletteOption);
    QCommandLineOption medianOption(QStringList() << "m" << "median", "Verwendet den Median zur Farbbestimmung anstelle des Durchschnitts.");
    parser.addOption(medianOption);

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
        qDebug() << Q_FUNC_INFO << "Verwende Medianberechnung";
    } else {
        farbMatrix = scanImageToMatrixAverage(imagePath, rasterBreite, rasterHoehe, bildBreite, bildHoehe);
        qDebug()  << Q_FUNC_INFO << "Verwende Durchschnittsberechnung";
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

    createSvgFromColorMatrix(farbMatrix, bildBreite, bildHoehe, rasterBreite, rasterHoehe, svgFileName, scalingMode, grayscale, blackCircles, activePalette, usePalette);
    qDebug() << "SVG-Datei erfolgreich erzeugt.";

    return(0);
}
