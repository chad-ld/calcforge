#include "AspectRatioCalculator.h"
#include "Logger.h"
#include <QDebug>
#include <QRegularExpression>
#include <cmath>

AspectRatioCalculator::AspectRatioCalculator()
{
    // Initialize regex patterns for dimension matching
    // Matches patterns like "1920x1080", "?x2000", "1280x?", "1920.5x1080.0"
    m_dimensionPattern = QRegularExpression(R"(^(\?|\d+(?:\.\d+)?)x(\?|\d+(?:\.\d+)?)$)", QRegularExpression::CaseInsensitiveOption);
    m_unknownPattern = QRegularExpression(R"(^\?$)");
}

AspectRatioResult AspectRatioCalculator::AR(const QString &originalDimensions, const QString &targetDimensions)
{
    try {
        // Parse original dimensions
        Dimensions original = parseDimensions(originalDimensions.trimmed());
        
        // Validate original dimensions (both must be known)
        if (original.widthUnknown || original.heightUnknown) {
            return AspectRatioResult("Error: Original dimensions must be fully specified (no '?' allowed)", false);
        }
        
        // Calculate aspect ratio from original dimensions
        double aspectRatio = calculateAspectRatio(original.width, original.height);
        
        // Parse target dimensions
        Dimensions target = parseDimensions(targetDimensions.trimmed());
        
        // Validate target dimensions (exactly one must be unknown)
        validateTargetDimensions(target);
        
        double resultWidth, resultHeight;
        
        if (target.widthUnknown && !target.heightUnknown) {
            // Solve for width: width = height * aspect_ratio
            resultWidth = solveForWidth(target.height, aspectRatio);
            resultHeight = target.height;
        }
        else if (target.heightUnknown && !target.widthUnknown) {
            // Solve for height: height = width / aspect_ratio
            resultWidth = target.width;
            resultHeight = solveForHeight(target.width, aspectRatio);
        }
        else {
            // This should be caught by validateTargetDimensions, but just in case
            return AspectRatioResult("Error: Exactly one dimension must be '?' to solve for", false);
        }
        
        // Format and return result
        QString result = formatDimensions(resultWidth, resultHeight);
        return AspectRatioResult(result);
        
    } catch (const AspectRatioError &e) {
        LOG_DEBUG(QString("Aspect ratio calculation error: %1").arg(e.what()));
        return AspectRatioResult(QString("Error: %1").arg(e.what()), false);
    } catch (const std::exception &e) {
        LOG_DEBUG(QString("Aspect ratio calculation error: %1").arg(e.what()));
        return AspectRatioResult(QString("Error: %1").arg(e.what()), false);
    }
}

Dimensions AspectRatioCalculator::parseDimensions(const QString &dimensionStr)
{
    QRegularExpressionMatch match = m_dimensionPattern.match(dimensionStr);
    
    if (!match.hasMatch()) {
        throw AspectRatioError(QString("Invalid dimension format: %1. Expected format like '1920x1080', '?x2000', or '1280x?'").arg(dimensionStr));
    }
    
    QString widthStr = match.captured(1);
    QString heightStr = match.captured(2);
    
    Dimensions dims;
    
    // Parse width
    if (widthStr == "?") {
        dims.widthUnknown = true;
        dims.width = 0;
    } else {
        bool ok;
        dims.width = widthStr.toDouble(&ok);
        if (!ok || dims.width <= 0) {
            throw AspectRatioError(QString("Invalid width value: %1. Must be a positive number").arg(widthStr));
        }
        dims.widthUnknown = false;
    }
    
    // Parse height
    if (heightStr == "?") {
        dims.heightUnknown = true;
        dims.height = 0;
    } else {
        bool ok;
        dims.height = heightStr.toDouble(&ok);
        if (!ok || dims.height <= 0) {
            throw AspectRatioError(QString("Invalid height value: %1. Must be a positive number").arg(heightStr));
        }
        dims.heightUnknown = false;
    }
    
    return dims;
}

double AspectRatioCalculator::calculateAspectRatio(double width, double height)
{
    if (height == 0) {
        throw AspectRatioError("Height cannot be zero when calculating aspect ratio");
    }
    
    return width / height;
}

double AspectRatioCalculator::solveForWidth(double height, double aspectRatio)
{
    if (height <= 0) {
        throw AspectRatioError("Height must be positive when solving for width");
    }
    
    return height * aspectRatio;
}

double AspectRatioCalculator::solveForHeight(double width, double aspectRatio)
{
    if (width <= 0) {
        throw AspectRatioError("Width must be positive when solving for height");
    }
    
    if (aspectRatio == 0) {
        throw AspectRatioError("Aspect ratio cannot be zero when solving for height");
    }
    
    return width / aspectRatio;
}

QString AspectRatioCalculator::formatDimensions(double width, double height, int precision)
{
    if (precision == 0) {
        // Format as integers (most common case for video resolutions)
        return QString("%1x%2")
            .arg(static_cast<int>(std::round(width)))
            .arg(static_cast<int>(std::round(height)));
    } else {
        // Format with specified decimal precision
        return QString("%1x%2")
            .arg(width, 0, 'f', precision)
            .arg(height, 0, 'f', precision);
    }
}

bool AspectRatioCalculator::isDimensionFormat(const QString &str) const
{
    return m_dimensionPattern.match(str).hasMatch();
}

void AspectRatioCalculator::validateTargetDimensions(const Dimensions &target)
{
    bool hasUnknown = target.widthUnknown || target.heightUnknown;
    bool hasBothUnknown = target.widthUnknown && target.heightUnknown;
    bool hasNeitherUnknown = !target.widthUnknown && !target.heightUnknown;
    
    if (!hasUnknown || hasBothUnknown) {
        throw AspectRatioError("Exactly one dimension must be '?' to solve for. Found: " +
                              QString(target.widthUnknown ? "width=?" : "width=known") + ", " +
                              QString(target.heightUnknown ? "height=?" : "height=known"));
    }
}
