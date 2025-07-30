#include "RegexUtils.h"

// Static member initialization
QRegularExpression RegexUtils::s_numericPattern;
QRegularExpression RegexUtils::s_decimalPattern;
QRegularExpression RegexUtils::s_integerPattern;
QRegularExpression RegexUtils::s_paddedZerosPattern;
QRegularExpression RegexUtils::s_unitConversionPattern;
QRegularExpression RegexUtils::s_currencyConversionPattern;
QRegularExpression RegexUtils::s_dimensionPattern;
QRegularExpression RegexUtils::s_timecodePattern;
QRegularExpression RegexUtils::s_crossSheetReferencePattern;
QRegularExpression RegexUtils::s_lineNumberReferencePattern;
QRegularExpression RegexUtils::s_letterOnlyPattern;
QRegularExpression RegexUtils::s_trailingOperatorPattern;
QRegularExpression RegexUtils::s_incompleteExpressionPattern;
bool RegexUtils::s_patternsInitialized = false;

void RegexUtils::initializePatterns()
{
    if (s_patternsInitialized) return;
    
    // Numeric patterns
    s_numericPattern = QRegularExpression(R"(^\d+(\.\d+)?$)");
    s_decimalPattern = QRegularExpression(R"(^\d*\.\d+$)");
    s_integerPattern = QRegularExpression(R"(^\d+$)");
    s_paddedZerosPattern = QRegularExpression(R"(\b0+(\d+)\b)");
    
    // Unit conversion patterns
    s_unitConversionPattern = QRegularExpression(
        R"(^([\d.]+)\s+(.+?)\s+to\s+(.+?)$)", 
        QRegularExpression::CaseInsensitiveOption
    );
    
    s_currencyConversionPattern = QRegularExpression(
        R"(^([\d.]+)\s+(.+?)\s+to\s+(.+?)$)", 
        QRegularExpression::CaseInsensitiveOption
    );
    
    // Dimension and timecode patterns
    s_dimensionPattern = QRegularExpression(
        R"(^(\?|\d+(?:\.\d+)?)x(\?|\d+(?:\.\d+)?)$)", 
        QRegularExpression::CaseInsensitiveOption
    );
    
    s_timecodePattern = QRegularExpression(
        R"(^(\d{1,2}):(\d{2}):(\d{2}):(\d{2})$)"
    );
    
    // Cross-sheet reference patterns
    s_crossSheetReferencePattern = QRegularExpression(
        R"(\bS\.([^.]+)\.LN(\d+)\b)", 
        QRegularExpression::CaseInsensitiveOption
    );
    
    s_lineNumberReferencePattern = QRegularExpression(
        R"(\bLN(\d+)\b)", 
        QRegularExpression::CaseInsensitiveOption
    );
    
    // Expression validation patterns
    s_letterOnlyPattern = QRegularExpression("^[a-zA-Z]+$");
    s_trailingOperatorPattern = QRegularExpression("[+\\-*/^]$");
    s_incompleteExpressionPattern = QRegularExpression(R"([+\-*/^]\s*$|^\s*[+*/^])");
    
    s_patternsInitialized = true;
}

// Pattern accessors
const QRegularExpression& RegexUtils::numericPattern()
{
    initializePatterns();
    return s_numericPattern;
}

const QRegularExpression& RegexUtils::decimalPattern()
{
    initializePatterns();
    return s_decimalPattern;
}

const QRegularExpression& RegexUtils::integerPattern()
{
    initializePatterns();
    return s_integerPattern;
}

const QRegularExpression& RegexUtils::paddedZerosPattern()
{
    initializePatterns();
    return s_paddedZerosPattern;
}

const QRegularExpression& RegexUtils::unitConversionPattern()
{
    initializePatterns();
    return s_unitConversionPattern;
}

const QRegularExpression& RegexUtils::currencyConversionPattern()
{
    initializePatterns();
    return s_currencyConversionPattern;
}

const QRegularExpression& RegexUtils::dimensionPattern()
{
    initializePatterns();
    return s_dimensionPattern;
}

const QRegularExpression& RegexUtils::timecodePattern()
{
    initializePatterns();
    return s_timecodePattern;
}

const QRegularExpression& RegexUtils::crossSheetReferencePattern()
{
    initializePatterns();
    return s_crossSheetReferencePattern;
}

const QRegularExpression& RegexUtils::lineNumberReferencePattern()
{
    initializePatterns();
    return s_lineNumberReferencePattern;
}

const QRegularExpression& RegexUtils::letterOnlyPattern()
{
    initializePatterns();
    return s_letterOnlyPattern;
}

const QRegularExpression& RegexUtils::trailingOperatorPattern()
{
    initializePatterns();
    return s_trailingOperatorPattern;
}

const QRegularExpression& RegexUtils::incompleteExpressionPattern()
{
    initializePatterns();
    return s_incompleteExpressionPattern;
}

// Utility functions
bool RegexUtils::isNumeric(const QString& text)
{
    return numericPattern().match(text.trimmed()).hasMatch();
}

bool RegexUtils::isDecimal(const QString& text)
{
    return decimalPattern().match(text.trimmed()).hasMatch();
}

bool RegexUtils::isInteger(const QString& text)
{
    return integerPattern().match(text.trimmed()).hasMatch();
}

double RegexUtils::extractNumericValue(const QString& text)
{
    QRegularExpressionMatch match = numericPattern().match(text.trimmed());
    if (match.hasMatch()) {
        bool ok;
        double value = match.captured().toDouble(&ok);
        return ok ? value : 0.0;
    }
    return 0.0;
}

bool RegexUtils::parseUnitConversion(const QString& expression, double& value, QString& fromUnit, QString& toUnit)
{
    QRegularExpressionMatch match = unitConversionPattern().match(expression.trimmed());
    if (match.hasMatch()) {
        bool ok;
        value = match.captured(1).toDouble(&ok);
        if (ok) {
            fromUnit = match.captured(2).trimmed();
            toUnit = match.captured(3).trimmed();
            return true;
        }
    }
    return false;
}

bool RegexUtils::parseCurrencyConversion(const QString& expression, double& value, QString& fromCurrency, QString& toCurrency)
{
    QRegularExpressionMatch match = currencyConversionPattern().match(expression.trimmed());
    if (match.hasMatch()) {
        bool ok;
        value = match.captured(1).toDouble(&ok);
        if (ok) {
            fromCurrency = match.captured(2).trimmed();
            toCurrency = match.captured(3).trimmed();
            return true;
        }
    }
    return false;
}

bool RegexUtils::parseDimensions(const QString& text, double& width, double& height, bool& widthUnknown, bool& heightUnknown)
{
    QRegularExpressionMatch match = dimensionPattern().match(text.trimmed());
    if (match.hasMatch()) {
        QString widthStr = match.captured(1);
        QString heightStr = match.captured(2);
        
        widthUnknown = (widthStr == "?");
        heightUnknown = (heightStr == "?");
        
        if (!widthUnknown) {
            bool ok;
            width = widthStr.toDouble(&ok);
            if (!ok) return false;
        }
        
        if (!heightUnknown) {
            bool ok;
            height = heightStr.toDouble(&ok);
            if (!ok) return false;
        }
        
        return true;
    }
    return false;
}

bool RegexUtils::parseTimecode(const QString& text, int& hours, int& minutes, int& seconds, int& frames)
{
    QRegularExpressionMatch match = timecodePattern().match(text.trimmed());
    if (match.hasMatch()) {
        bool ok;
        hours = match.captured(1).toInt(&ok);
        if (!ok) return false;
        
        minutes = match.captured(2).toInt(&ok);
        if (!ok) return false;
        
        seconds = match.captured(3).toInt(&ok);
        if (!ok) return false;
        
        frames = match.captured(4).toInt(&ok);
        if (!ok) return false;
        
        return true;
    }
    return false;
}

bool RegexUtils::parseCrossSheetReference(const QString& text, QString& sheetName, int& lineNumber)
{
    QRegularExpressionMatch match = crossSheetReferencePattern().match(text);
    if (match.hasMatch()) {
        sheetName = match.captured(1);
        bool ok;
        lineNumber = match.captured(2).toInt(&ok);
        return ok;
    }
    return false;
}

bool RegexUtils::isValidExpression(const QString& expression)
{
    QString trimmed = expression.trimmed();
    return !trimmed.isEmpty() && !isIncompleteExpression(trimmed);
}

bool RegexUtils::isIncompleteExpression(const QString& expression)
{
    QString trimmed = expression.trimmed();
    
    // Check for expressions that are too short and contain only letters
    if (trimmed.length() <= 2 && letterOnlyPattern().match(trimmed).hasMatch()) {
        return true;
    }
    
    // Check for trailing operators
    if (trailingOperatorPattern().match(trimmed).hasMatch()) {
        return true;
    }
    
    // Check for incomplete patterns
    return incompleteExpressionPattern().match(trimmed).hasMatch();
}

QString RegexUtils::cleanupExpression(const QString& expression)
{
    return removePaddedZeros(expression.trimmed());
}

QString RegexUtils::removePaddedZeros(const QString& expression)
{
    QString result = expression;
    
    // Replace padded zeros (like 007 -> 7) but preserve standalone zeros
    result.replace(paddedZerosPattern(), "\\1");
    
    // Handle standalone zeros pattern
    QRegularExpression justZeros(R"(\b0+\b)");
    result.replace(justZeros, "0");
    
    return result;
}