#ifndef ASPECTRATIOCALCULATOR_H
#define ASPECTRATIOCALCULATOR_H

#include <QString>
#include <QRegularExpression>
#include <stdexcept>
#include "CalcForgeResult.h"
#include "CalcForgeException.h"

/**
 * Dimension structure for parsing and calculations
 * Contains width and height values, with support for unknown dimensions
 */
struct Dimensions {
    double width;
    double height;
    bool widthUnknown;
    bool heightUnknown;
    
    Dimensions() : width(0), height(0), widthUnknown(false), heightUnknown(false) {}
    Dimensions(double w, double h) : width(w), height(h), widthUnknown(false), heightUnknown(false) {}
};

/**
 * Custom exception for aspect ratio errors
 */
class AspectRatioError : public std::runtime_error {
public:
    explicit AspectRatioError(const QString& message) 
        : std::runtime_error(message.toStdString()) {}
};

/**
 * Aspect ratio calculation system for CalcForge C++
 * Handles dimension scaling while preserving aspect ratios
 * Supports video/graphics resolution calculations
 */
class AspectRatioCalculator
{
public:
    AspectRatioCalculator();
    
    /**
     * Main AR function that calculates missing dimensions
     * @param originalDimensions Original dimensions (e.g., "1920x1080")
     * @param targetDimensions Target dimensions with one unknown (e.g., "?x2000" or "1280x?")
     * @return AspectRatioResult with calculated dimensions or error
     */
    AspectRatioResult AR(const QString &originalDimensions, const QString &targetDimensions);
    
    /**
     * Parse dimension string into Dimensions structure
     * @param dimensionStr Dimension string (e.g., "1920x1080", "?x2000", "1280x?")
     * @return Dimensions structure with parsed values
     * @throws AspectRatioError if parsing fails
     */
    Dimensions parseDimensions(const QString &dimensionStr);
    
    /**
     * Calculate aspect ratio from width and height
     * @param width Width value
     * @param height Height value
     * @return Aspect ratio (width/height)
     * @throws AspectRatioError if height is zero
     */
    double calculateAspectRatio(double width, double height);
    
    /**
     * Solve for missing width given height and aspect ratio
     * @param height Known height
     * @param aspectRatio Aspect ratio to maintain
     * @return Calculated width
     */
    double solveForWidth(double height, double aspectRatio);
    
    /**
     * Solve for missing height given width and aspect ratio
     * @param width Known width
     * @param aspectRatio Aspect ratio to maintain
     * @return Calculated height
     */
    double solveForHeight(double width, double aspectRatio);
    
    /**
     * Format dimensions as string
     * @param width Width value
     * @param height Height value
     * @param precision Decimal precision (default: 0 for integers)
     * @return Formatted dimension string (e.g., "1920x1080")
     */
    QString formatDimensions(double width, double height, int precision = 0);
    
    /**
     * Check if a string represents a valid dimension format
     * @param str String to check
     * @return True if string matches dimension pattern
     */
    bool isDimensionFormat(const QString &str) const;
    
    /**
     * Validate that exactly one dimension is unknown
     * @param target Target dimensions to validate
     * @throws AspectRatioError if validation fails
     */
    void validateTargetDimensions(const Dimensions &target);

private:
    // Regular expressions for dimension pattern matching
    QRegularExpression m_dimensionPattern;
    QRegularExpression m_unknownPattern;
};

#endif // ASPECTRATIOCALCULATOR_H
