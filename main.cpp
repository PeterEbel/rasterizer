#include <QCoreApplication>
#include <QImage>
#include <QColor>
#include <QFile>
#include <QTextStream>
#include <vector>
#include <iostream>

std::vector<double> analyzeBlocksLuminance(const QImage &image, int blockWidth, int blockHeight) {

    std::vector<double> luminanceValues;

    for (int row = 0; row < image.height(); row += blockHeight) {
        for (int col = 0; col < image.width(); col += blockWidth) {
            double totalLuminance = 0.0;
            int pixelCount = 0;

            // Analyze the current block
            for (int i = 0; i < blockHeight; ++i) {
                for (int j = 0; j < blockWidth; ++j) {
                    int pixelX = col + j;
                    int pixelY = row + i;

                    // Check if the pixel is within the image bounds
                    if (pixelX < image.width() && pixelY < image.height()) {
                        QColor pixelColor = image.pixelColor(pixelX, pixelY);
                        double luminance = 0.299 * pixelColor.red() + 0.587 * pixelColor.green() + 0.114 * pixelColor.blue();
                        totalLuminance += luminance;
                        pixelCount++;
                    }
                }
            }

            // Calculate average luminance for the block
            if (pixelCount > 0) {
                luminanceValues.push_back(totalLuminance / pixelCount);
            } else {
                luminanceValues.push_back(0.0); // No pixels in this block
            }
        }
    }

    return luminanceValues; // Return a vector of average luminance values for each block
}

std::vector<QColor> analyzeBlocksColor(const QImage &image, int blockWidth, int blockHeight, bool meanColor) {

    std::vector<QColor> colorValues;

    for (int row = 0; row < image.height(); row += blockHeight) {
        for (int col = 0; col < image.width(); col += blockWidth) {
            double totalRed = 0.0, totalGreen = 0.0, totalBlue = 0.0;
            int pixelCount = 0;

            // Analyze the current block
            for (int i = 0; i < blockHeight; ++i) {
                for (int j = 0; j < blockWidth; ++j) {
                    int pixelX = col + j;
                    int pixelY = row + i;

                    // Check if the pixel is within the image bounds
                    if (pixelX < image.width() && pixelY < image.height()) {
                        QColor pixelColor = image.pixelColor(pixelX, pixelY);
                        totalRed += pixelColor.red();
                        totalGreen += pixelColor.green();
                        totalBlue += pixelColor.blue();
                        pixelCount++;
                    }
                }
            }

            // Calculate average color for the block
            if (pixelCount > 0) {
                if (meanColor) {
                    // Mean color calculation (average of averages)
                    totalRed /= pixelCount;
                    totalGreen /= pixelCount;
                    totalBlue /= pixelCount;
                }
                colorValues.push_back(QColor(static_cast<int>(totalRed), static_cast<int>(totalGreen), static_cast<int>(totalBlue)));
            } else {
                colorValues.push_back(QColor(0, 0, 0)); // No pixels in this block
            }
        }
    }

    return colorValues; // Return a vector of average colors for each block
}

void generateSVG(const QString &filename, const QImage &inputImage, int outputWidth, int outputHeight, double circleDiameter, int blockWidth, int blockHeight, int mode) {

    QFile file(filename);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        std::cerr << "Error: Could not open file for writing." << std::endl;
        return;
    }

    std::cout << "Width: " << inputImage.width() << std::endl << "Height: " << inputImage.height() << std::endl;

    QTextStream out(&file);
    out << "<svg width=\"" << outputWidth << "mm\" height=\"" << outputHeight << "mm\" xmlns=\"http://www.w3.org/2000/svg\">\n";

    std::vector<double> luminanceValues;
    std::vector<QColor> colorValues;

    if (mode == 1) {
        luminanceValues = analyzeBlocksLuminance(inputImage, blockWidth, blockHeight);
        std::cout << "Elements in vector: " << luminanceValues.size() << std::endl;
    } else if (mode == 2 || mode == 3) {
        colorValues = analyzeBlocksColor(inputImage, blockWidth, blockHeight, mode == 2);
    } else {
        std::cerr << "Error: Invalid mode specified. Use 1 for luminance, 2 for average color, or 3 for mean color." << std::endl;
        return;
    }

    int circlesPerRow = outputWidth / circleDiameter;
    int circlesPerColumn = outputHeight / circleDiameter;

    std::cout << "Circles per row: " << outputWidth / circleDiameter << std::endl << "Circles per column: " << outputHeight / circleDiameter << std::endl;

    // Loop through each block of circles
    for (int row = 0; row < circlesPerColumn; ++row) {
        for (int col = 0; col < circlesPerRow; ++col) {
            // Calculate the corresponding block index
//            int blockIndex = (row * blockHeight / blockHeight) * (inputImage.width() / blockWidth) + (col * blockWidth / blockWidth);
             int blockIndex = row * (inputImage.width() / blockWidth) + col;

            QColor fillColor;

            if (mode == 1) {
                double luminance = luminanceValues[blockIndex]; // Get average luminance for the block
                fillColor = QColor(static_cast<int>(luminance), static_cast<int>(luminance), static_cast<int>(luminance)); // Grayscale
            } else if (mode == 2) {
                fillColor = colorValues[blockIndex]; // Get average color for the block
            } else if (mode == 3) {
                fillColor = colorValues[blockIndex]; // Get mean color for the block
            }

            // Draw the circles for the current block
            for (int i = 0; i < blockHeight / 10; ++i) {
                for (int j = 0; j < blockWidth / 10; ++j) {
                    out << QString("<circle cx=\"%1\" cy=\"%2\" r=\"%3\" fill=\"%4\" />\n")
                    .arg((col * (blockWidth / 10) + j) * circleDiameter + circleDiameter / 2) // Center X
                        .arg((row * (blockHeight / 10) + i) * circleDiameter + circleDiameter / 2) // Center Y
                        .arg(circleDiameter / 2) // Radius
                        .arg(fillColor.name()); // Fill color in hex format
                }
            }
        }
    }

    out << "</svg>\n";
    file.close();
}
int main(int argc, char *argv[]) {

    QCoreApplication app(argc, argv);

    if (argc != 8) {
        std::cerr << "Usage: " << argv[0] << " <image_file> <output_width_mm> <output_height_mm> <circle_diameter_mm> <block_width_px> <block_height_px> <mode>" << std::endl;
        return 1;
    }

    QString imageFile = argv[1];
    int outputWidth = QString(argv[2]).toInt();
    int outputHeight = QString(argv[3]).toInt();
    double circleDiameter = QString(argv[4]).toDouble();
    int blockWidth = QString(argv[5]).toInt();
    int blockHeight = QString(argv[6]).toInt();
    int mode = QString(argv[7]).toInt();

    // Load the input image
    QImage inputImage(imageFile);
    if (inputImage.isNull()) {
        std::cerr << "Error: Could not load image file." << std::endl;
        return 1;
    }

    // Generate the SVG output file
    QString outputFile = "output.svg"; // You can modify this to take an output filename as a parameter if needed
    generateSVG(outputFile, inputImage, outputWidth, outputHeight, circleDiameter, blockWidth, blockHeight, mode);

    std::cout << "SVG file generated: " << outputFile.toStdString() << std::endl;
    return 0;
}
