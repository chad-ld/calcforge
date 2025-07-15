#ifndef PERCENTAGECALCULATOR_H
#define PERCENTAGECALCULATOR_H

#include <QString>
#include <QRegularExpression>

/**
 * Percentage calculation types for percent() function
 */
enum class PercentageType {
    BASIC,      // percent(25%, 1000) → 250
    REVERSE,    // percent(1000, %, 2000) → 50%
    INCREASE,   // percent(1000, +, 25%) → 1250
    DECREASE,   // percent(1000, -, 15%) → 850
    CHANGE      // percent(1000, to, 1200) → 20%
};

/**
 * Percentage calculation result structure
 * Contains the calculated value and formatting information
 */
struct PercentageResult {
    double value;
    QString unit;           // "%" for percentage results, empty for raw numbers
    bool isValid;
    QString errorMessage;
    PercentageType type;

    PercentageResult() : value(0.0), isValid(false), type(PercentageType::BASIC) {}
    PercentageResult(double val, const QString& unitStr, PercentageType calcType)
        : value(val), unit(unitStr), isValid(true), type(calcType) {}
    PercentageResult(const QString& error)
        : value(0.0), isValid(false), errorMessage(error), type(PercentageType::BASIC) {}
};

/**
 * Comprehensive percentage calculation system for CalcForge C++
 * Handles various percentage calculation types using percent() function syntax
 * Supports LN variable references through automatic substitution
 */
class PercentageCalculator
{
public:
    PercentageCalculator();
    
    /**
     * Calculate a percent() function call with automatic type detection
     * @param expression The percent() function call to parse and evaluate
     * @return PercentageResult with calculated value and formatting, or invalid result
     */
    PercentageResult calculateExpression(const QString &expression);
    
    /**
     * Format percentage result for display
     * @param result The percentage result to format
     * @return Formatted string for display
     */
    QString formatResult(const PercentageResult &result);

private:
    /**
     * Parse percent() function arguments and determine calculation type
     * @param args List of function arguments
     * @return PercentageResult with calculated value or error
     */
    PercentageResult parsePercentFunction(const QStringList &args);

    /**
     * Calculate basic percentage: percent(25%, 1000) = (25/100) * 1000
     */
    PercentageResult calculateBasicPercentage(double percentage, double value);

    /**
     * Calculate reverse percentage: percent(1000, %, 2000) = (1000/2000) * 100
     */
    PercentageResult calculateReversePercentage(double part, double whole, int precision = -1);

    /**
     * Calculate percentage increase: percent(1000, +, 25%) = 1000 * (1 + 25/100)
     */
    PercentageResult calculatePercentageIncrease(double value, double percentage);

    /**
     * Calculate percentage decrease: percent(1000, -, 15%) = 1000 * (1 - 15/100)
     */
    PercentageResult calculatePercentageDecrease(double value, double percentage);

    /**
     * Calculate percentage change: percent(1000, to, 1200) = (1200-1000)/1000 * 100
     */
    PercentageResult calculatePercentageChange(double oldValue, double newValue, int precision = -1);
    
    /**
     * Parse a numeric value that may include percentage symbol
     * @param str String to parse (e.g., "25%", "1000", "0.5%")
     * @param isPercentage Set to true if the value includes %
     * @return Parsed numeric value
     */
    double parseNumericValue(const QString &str, bool &isPercentage);

    /**
     * Parse precision specifier (e.g., ".2" for 2 decimal places)
     * @param str Precision string
     * @return Number of decimal places, or -1 if invalid
     */
    int parsePrecision(const QString &str);

    /**
     * Format numeric value for display (removes unnecessary decimals)
     * @param value The numeric value to format
     * @param precision Number of decimal places (-1 for auto)
     * @return Formatted string representation
     */
    QString formatNumericValue(double value, int precision = -1);

    /**
     * Validate that a numeric value is reasonable for percentage calculations
     * @param value The value to validate
     * @param context Description of the value for error messages
     * @return True if valid, false otherwise
     */
    bool validateNumericValue(double value, const QString &context);

private:
    // Regular expression for parsing percent() function calls
    QRegularExpression m_percentFunctionPattern;
};

#endif // PERCENTAGECALCULATOR_H
