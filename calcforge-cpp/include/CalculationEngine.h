#ifndef CALCULATIONENGINE_H
#define CALCULATIONENGINE_H

#include <QString>
#include <QStringList>
#include <QHash>
#include <QRegularExpression>
#include <functional>
#include <cmath>
#include "UnitConverter.h"
#include "TimecodeCalculator.h"
#include "AspectRatioCalculator.h"
#include "DateCalculator.h"
#include "CurrencyConverter.h"

// Forward declarations
class WorksheetWidget;

/**
 * Basic calculation engine for CalcForge C++
 * Handles mathematical expressions with proper operation order
 */
class CalculationEngine
{
public:
    CalculationEngine();
    
    /**
     * Evaluate a single mathematical expression
     * @param expression The expression to evaluate (e.g., "2 + 3 * 4")
     * @param lineNumber Line number for LN references (future use)
     * @return Result as a string, or empty string if invalid
     */
    QString evaluateExpression(const QString &expression, int lineNumber = 0);
    
    /**
     * Evaluate multiple expressions (one per line)
     * @param expressions List of expressions to evaluate
     * @return List of results corresponding to each expression
     */
    QStringList evaluateExpressions(const QStringList &expressions);
    
    /**
     * Check if an expression is a comment line
     * @param expression The expression to check
     * @return True if it's a comment (starts with :::)
     */
    bool isCommentLine(const QString &expression);
    
    /**
     * Format a numeric result for display
     * @param value The numeric value to format
     * @return Formatted string representation
     */
    QString formatResult(double value);

    /**
     * Update stored line values after line insertions/deletions
     * @param insertionPoint Line number where insertion/deletion occurred (1-based)
     * @param linesDelta Number of lines inserted (positive) or deleted (negative)
     */
    void updateLineValuesAfterChange(int insertionPoint, int linesDelta);

    /**
     * Preprocess expression (convert ^ to **, handle zero-padded numbers)
     * Made public for use by auto-update system to avoid false change detection
     */
    QString preprocessExpression(const QString &expr);

    /**
     * Clear all stored line values (for new worksheets or paste operations)
     */
    void clearLineValues();

    /**
     * Set the worksheet widget reference for accessing content
     * @param widget Pointer to the worksheet widget
     */
    void setWorksheetWidget(WorksheetWidget *widget);

    /**
     * Set the sheet lookup function for cross-sheet references
     * @param lookupFunction Function that takes a sheet name and returns the WorksheetWidget pointer
     */
    void setSheetLookupFunction(std::function<WorksheetWidget*(const QString&)> lookupFunction);

    /**
     * Set the current sheet name for error reporting
     * @param sheetName Name of the current sheet
     */
    void setCurrentSheetName(const QString &sheetName);

    /**
     * Handle unit conversion expressions like "5 feet to meters"
     * @param expr The expression to check for unit conversion
     * @return Formatted result string with value and unit, or empty string if not a unit conversion
     */
    QString handleUnitConversion(const QString &expr);

    /**
     * Handle timecode function calls like "TC(24, 100)" or "TC(30, '00:01:00:00')"
     * @param expr The expression to check and evaluate
     * @return Timecode result string, or empty string if not a TC function
     */
    QString handleTimecodeFunction(const QString &expr);

    /**
     * Handle aspect ratio function calls like "AR(1920x1080, ?x2000)"
     * @param expr The expression to check and evaluate
     * @return Aspect ratio result string, or empty string if not an AR function
     */
    QString handleAspectRatioFunction(const QString &expr);

    /**
     * Handle date function calls like "D(July 4, 2023 + 30)" or "D(July 4, 2023 W+ 5)"
     * @param expr The expression to check and evaluate
     * @return Date result string, or empty string if not a D function
     */
    QString handleDateFunction(const QString &expr);

    /**
     * Handle currency conversion expressions like "100 dollars to euros"
     * @param expr The expression to check and evaluate
     * @return Currency conversion result string, or empty string if not a currency conversion
     */
    QString handleCurrencyConversion(const QString &expr);

    /**
     * Get the stored value for a specific line number
     * @param lineNumber The line number to look up
     * @return The stored value, or 0.0 if not found
     */
    double getLineValue(int lineNumber) const;

    /**
     * Check if a line has a calculated value
     * @param lineNumber The line number to check
     * @return True if the line has been evaluated and has a value
     */
    bool hasLineValue(int lineNumber) const;

    /**
     * Get the stored value for a cross-sheet reference
     * @param sheetName Name of the sheet (case-insensitive)
     * @param lineNumber Line number in the referenced sheet
     * @return The stored value, or error string if not found
     */
    QString getCrossSheetValue(const QString &sheetName, int lineNumber) const;

private:
    /**
     * Parse and evaluate a mathematical expression
     * Uses recursive descent parser for proper operation order
     * @param expr The expression to parse
     * @return The calculated result
     */
    double parseExpression(const QString &expr);
    
    /**
     * Parse addition and subtraction (lowest precedence)
     */
    double parseAddSub(const QString &expr, int &pos);
    
    /**
     * Parse multiplication and division
     */
    double parseMulDiv(const QString &expr, int &pos);
    
    /**
     * Parse exponentiation (highest precedence)
     */
    double parsePower(const QString &expr, int &pos);
    
    /**
     * Parse unary operators and factors (numbers, functions, parentheses)
     */
    double parseFactor(const QString &expr, int &pos);
    
    /**
     * Parse a number from the expression
     */
    double parseNumber(const QString &expr, int &pos);
    
    /**
     * Parse a function call (sin, cos, sqrt, etc.)
     */
    double parseFunction(const QString &expr, int &pos);
    
    /**
     * Skip whitespace in the expression
     */
    void skipWhitespace(const QString &expr, int &pos);

    /**
     * Check if expression is incomplete or invalid
     */
    bool isIncompleteExpression(const QString &expr);

    /**
     * Process LN references in an expression
     * Replaces LN1, LN2, etc. with their stored values
     * Also handles cross-sheet references like S.SheetName.LN5
     * @param expr The expression containing LN references
     * @return Expression with LN references replaced by values
     */
    QString processLNReferences(const QString &expr);

private:
    /**
     * Handle statistical functions (sum, mean, min, max, etc.)
     * @param expr The expression containing statistical function
     * @param lineNumber Current line number for context
     * @return Result value, or NaN if not a statistical function
     */
    double handleStatisticalFunctions(const QString &expr, int lineNumber);

    /**
     * Parse range expressions for statistical functions
     * @param rangeExpr Range expression like "1-5", "1,3,5", "above", "below"
     * @param currentLine Current line number for relative ranges
     * @return List of values from the specified range
     */
    QList<double> getValuesFromRange(const QString &rangeExpr, int currentLine);

    /**
     * Extract numeric value from formatted result strings
     * Used for LN references to strip text endings from conversion results
     * @param result Formatted result string like "129 miles" or "85.63 Euros"
     * @return Numeric value extracted from the beginning of the string
     */
    double extractNumericValueFromResult(const QString &result);

    // Mathematical constants and functions
    QHash<QString, double> m_constants;
    QHash<QString, std::function<double(double)>> m_functions;

    // Line number to value mapping for LN references
    QHash<int, double> m_lineValues;

    // Unit converter for handling unit conversions
    UnitConverter m_unitConverter;

    // Timecode calculator for handling TC functions
    TimecodeCalculator m_timecodeCalculator;

    // Aspect ratio calculator for handling AR functions
    AspectRatioCalculator m_aspectRatioCalculator;

    // Date calculator for handling D functions
    DateCalculator m_dateCalculator;

    // Currency converter for handling currency conversions
    CurrencyConverter m_currencyConverter;

    // Cross-sheet reference support
    std::function<WorksheetWidget*(const QString&)> m_sheetLookupFunction;
    QString m_currentSheetName;

    // Reference to the worksheet widget for accessing content
    WorksheetWidget *m_worksheetWidget;
};

#endif // CALCULATIONENGINE_H
