#ifndef REGEXUTILS_H
#define REGEXUTILS_H

#include <QString>
#include <QRegularExpression>
#include <QRegularExpressionMatch>

/**
 * Utility class for common regex patterns used across CalcForge calculators
 * Eliminates duplication of regex patterns and provides consistent parsing behavior
 */
class RegexUtils
{
public:
    // Common numeric patterns
    static const QRegularExpression& numericPattern();
    static const QRegularExpression& decimalPattern();
    static const QRegularExpression& integerPattern();
    static const QRegularExpression& paddedZerosPattern();
    
    // Unit conversion patterns
    static const QRegularExpression& unitConversionPattern();
    static const QRegularExpression& currencyConversionPattern();
    
    // Dimension and timecode patterns
    static const QRegularExpression& dimensionPattern();
    static const QRegularExpression& timecodePattern();
    
    // Cross-sheet reference patterns
    static const QRegularExpression& crossSheetReferencePattern();
    static const QRegularExpression& lineNumberReferencePattern();
    
    // Expression validation patterns
    static const QRegularExpression& letterOnlyPattern();
    static const QRegularExpression& trailingOperatorPattern();
    static const QRegularExpression& incompleteExpressionPattern();
    
    // Utility functions for common parsing tasks
    static bool isNumeric(const QString& text);
    static bool isDecimal(const QString& text);
    static bool isInteger(const QString& text);
    static double extractNumericValue(const QString& text);
    
    // Unit conversion parsing helpers
    static bool parseUnitConversion(const QString& expression, double& value, QString& fromUnit, QString& toUnit);
    static bool parseCurrencyConversion(const QString& expression, double& value, QString& fromCurrency, QString& toCurrency);
    
    // Dimension parsing helpers
    static bool parseDimensions(const QString& text, double& width, double& height, bool& widthUnknown, bool& heightUnknown);
    
    // Timecode parsing helpers
    static bool parseTimecode(const QString& text, int& hours, int& minutes, int& seconds, int& frames);
    
    // Cross-sheet reference parsing
    static bool parseCrossSheetReference(const QString& text, QString& sheetName, int& lineNumber);
    
    // Expression validation helpers
    static bool isValidExpression(const QString& expression);
    static bool isIncompleteExpression(const QString& expression);
    static QString cleanupExpression(const QString& expression);
    
    // Zero-padding cleanup
    static QString removePaddedZeros(const QString& expression);

private:
    // Private constructor - utility class with static methods only
    RegexUtils() = delete;
    
    // Initialize patterns (called on first access)
    static void initializePatterns();
    
    // Pattern storage
    static QRegularExpression s_numericPattern;
    static QRegularExpression s_decimalPattern;
    static QRegularExpression s_integerPattern;
    static QRegularExpression s_paddedZerosPattern;
    static QRegularExpression s_unitConversionPattern;
    static QRegularExpression s_currencyConversionPattern;
    static QRegularExpression s_dimensionPattern;
    static QRegularExpression s_timecodePattern;
    static QRegularExpression s_crossSheetReferencePattern;
    static QRegularExpression s_lineNumberReferencePattern;
    static QRegularExpression s_letterOnlyPattern;
    static QRegularExpression s_trailingOperatorPattern;
    static QRegularExpression s_incompleteExpressionPattern;
    
    static bool s_patternsInitialized;
};

#endif // REGEXUTILS_H