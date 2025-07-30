#ifndef DATECALCULATOR_H
#define DATECALCULATOR_H

#include <QString>
#include <QStringList>
#include <QDate>
#include <QRegularExpression>
#include <stdexcept>

/**
 * Date calculation result structure
 * Contains the result value and any error information
 */
struct DateResult {
    QString value;
    bool isValid;
    QString errorMessage;

    DateResult() : isValid(false) {}
    DateResult(const QString& val) : value(val), isValid(true) {}
    DateResult(const QString& error, bool) : isValid(false), errorMessage(error) {}
};

/**
 * Date range result structure for date differences
 * Contains the number of days and the unit type
 */
struct DateRangeResult {
    int days;
    QString unit;
    bool isValid;
    QString errorMessage;

    DateRangeResult() : days(0), isValid(false) {}
    DateRangeResult(int d, const QString& u) : days(d), unit(u), isValid(true) {}
    DateRangeResult(const QString& error, bool) : days(0), isValid(false), errorMessage(error) {}
};

/**
 * Custom exception for date errors
 */
class DateError : public std::runtime_error {
public:
    explicit DateError(const QString& message) 
        : std::runtime_error(message.toStdString()) {}
};

/**
 * Comprehensive date calculation system for CalcForge C++
 * Handles date parsing, arithmetic, and business day calculations
 * Supports multiple date formats and business day operations
 */
class DateCalculator
{
public:
    DateCalculator();
    
    /**
     * Main D function that handles date arithmetic and formatting
     * @param expression Date expression (e.g., "July 4, 2023 + 30", "July 4, 2023 W+ 5")
     * @return DateResult with formatted date or error
     */
    DateResult D(const QString &expression);
    
    /**
     * Parse a date string in various formats
     * @param dateStr Date string to parse
     * @return QDate object
     * @throws DateError if parsing fails
     */
    QDate parseDate(const QString &dateStr);
    
    /**
     * Handle date arithmetic expressions
     * @param expression Date arithmetic expression
     * @return DateResult with calculated result
     */
    DateResult handleDateArithmetic(const QString &expression);
    
    /**
     * Add business days to a date, skipping weekends
     * @param startDate Starting date
     * @param days Number of business days to add (can be negative)
     * @return Resulting date after adding business days
     */
    QDate addBusinessDays(const QDate &startDate, int days);
    
    /**
     * Count business days between two dates, excluding weekends
     * @param startDate Start date
     * @param endDate End date
     * @param exclusive If true, exclude start and end dates; if false, include both
     * @return Number of business days between dates
     */
    int countBusinessDays(const QDate &startDate, const QDate &endDate, bool exclusive = false);
    
    /**
     * Format a date as a readable string
     * @param date Date to format
     * @return Formatted date string (e.g., "July 4, 2023")
     */
    QString formatDate(const QDate &date);
    
    /**
     * Check if a string represents a valid date format
     * @param str String to check
     * @return True if string matches a date pattern
     */
    bool isDateFormat(const QString &str) const;
    
    /**
     * Parse continuous number format dates (MMDDYYYY, MDDYYYY, MDYYYY)
     * @param dateStr Date string with only numbers
     * @return QDate object if successful, invalid QDate if failed
     */
    QDate parseContinuousFormat(const QString &dateStr);

private:
    /**
     * Initialize date format patterns
     */
    void initializeDateFormats();
    
    /**
     * Try to parse date using standard formats
     * @param dateStr Date string to parse
     * @return QDate object if successful, invalid QDate if failed
     */
    QDate parseStandardFormats(const QString &dateStr);
    
    /**
     * Extract numeric-only string from date
     * @param dateStr Original date string
     * @return String with only digits
     */
    QString extractNumericOnly(const QString &dateStr);
    
    /**
     * Validate month and day values
     * @param month Month value (1-12)
     * @param day Day value (1-31)
     * @return True if valid
     */
    bool isValidMonthDay(int month, int day);

    // Date format patterns for parsing
    QStringList m_dateFormats;
    
    // Regular expressions for date pattern matching
    QRegularExpression m_dateRangePattern;
    QRegularExpression m_dateArithmeticPattern;
    QRegularExpression m_singleDatePattern;
    QRegularExpression m_businessDayPattern;
    QRegularExpression m_exclusiveModePattern;
};

#endif // DATECALCULATOR_H
