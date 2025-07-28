#include "DateCalculator.h"
#include "Logger.h"
#include <QDebug>
#include <QRegularExpression>
#include <QLocale>

DateCalculator::DateCalculator()
{
    initializeDateFormats();
    
    // Initialize regex patterns for date arithmetic
    // Two dates with subtraction - handle spaces in dates, W- syntax, and optional E parameter
    m_dateRangePattern = QRegularExpression(
        R"(([A-Za-z]+\s+\d+,\s*\d{4}|\d+[\/\.\-]\d+[\/\.\-]\d{4}|\d{4}[\/\.\-]\d+[\/\.\-]\d+|\d{6,8})\s*W?\s*-\s*([A-Za-z]+\s+\d+,\s*\d{4}|\d+[\/\.\-]\d+[\/\.\-]\d{4}|\d{4}[\/\.\-]\d+[\/\.\-]\d+|\d{6,8})(?:\s*,\s*E)?)",
        QRegularExpression::CaseInsensitiveOption
    );

    // Date plus/minus days - handle spaces and optional W
    m_dateArithmeticPattern = QRegularExpression(
        R"(([A-Za-z]+\s+\d+,\s*\d{4}|\d+[\/\.\-]\d+[\/\.\-]\d{4}|\d{4}[\/\.\-]\d+[\/\.\-]\d+|\d{6,8})\s*W?\s*([+\-])\s*(\d+))",
        QRegularExpression::CaseInsensitiveOption
    );

    // Single date
    m_singleDatePattern = QRegularExpression(
        R"(^([A-Za-z]+\s+\d+,\s*\d{4}|\d+[\/\.\-]\d+[\/\.\-]\d{4}|\d{4}[\/\.\-]\d+[\/\.\-]\d+|\d{6,8})$)",
        QRegularExpression::CaseInsensitiveOption
    );
    
    // Business day pattern
    m_businessDayPattern = QRegularExpression(R"(\bW\b)", QRegularExpression::CaseInsensitiveOption);

    // Exclusive mode pattern
    m_exclusiveModePattern = QRegularExpression(R"(,\s*E\s*$)", QRegularExpression::CaseInsensitiveOption);
}

void DateCalculator::initializeDateFormats()
{
    // Date formats in order of preference
    m_dateFormats = {
        "MMMM d, yyyy",   // "July 12, 1985"
        "MMMM d,yyyy",    // "July 12,1985"
        "MMM d, yyyy",    // "Jul 12, 1985"
        "MMM d,yyyy",     // "Jul 12,1985"
        "M/d/yyyy",       // "7/4/2023"
        "MM/dd/yyyy",     // "07/04/2023"
        "M-d-yyyy",       // "7-4-2023"
        "MM-dd-yyyy",     // "07-04-2023"
        "M.d.yyyy",       // "7.4.2023"
        "MM.dd.yyyy",     // "07.04.2023"
        "yyyy/M/d",       // "2023/7/4"
        "yyyy/MM/dd",     // "2023/07/04"
        "yyyy-M-d",       // "2023-7-4"
        "yyyy-MM-dd",     // "2023-07-04"
        "yyyy.M.d",       // "2023.7.4"
        "yyyy.MM.dd"      // "2023.07.04"
    };
}

DateResult DateCalculator::D(const QString &expression)
{
    try {
        QString expr = expression.trimmed();
        if (expr.isEmpty()) {
            return DateResult::error("Error: Empty date expression");
        }
        
        return handleDateArithmetic(expr);
        
    } catch (const DateException &e) {
        LOG_DEBUG(QString("Date calculation error: %1").arg(e.what()));
        return DateResult::error(QString("Error: %1").arg(e.what()));
    } catch (const std::exception &e) {
        LOG_DEBUG(QString("Date calculation error: %1").arg(e.what()));
        return DateResult::error(QString("Error: %1").arg(e.what()));
    }
}

QDate DateCalculator::parseDate(const QString &dateStr)
{
    QString cleanDateStr = dateStr.trimmed();
    LOG_DEBUG(QString("parseDate called with: '%1'").arg(cleanDateStr));

    // First try continuous number format
    QDate continuousDate = parseContinuousFormat(cleanDateStr);
    if (continuousDate.isValid()) {
        LOG_DEBUG(QString("Parsed as continuous format: %1").arg(continuousDate.toString()));
        return continuousDate;
    }

    // Try standard formats
    QDate standardDate = parseStandardFormats(cleanDateStr);
    if (standardDate.isValid()) {
        LOG_DEBUG(QString("Parsed as standard format: %1").arg(standardDate.toString()));
        return standardDate;
    }
    
    // Check if it looks like a date but is invalid (e.g., July 32, February 30)
    if (dateStr.contains(QRegularExpression(R"([A-Za-z]+\s+\d+,?\s*\d{4})"))) {
        throw DateException("Date doesn't exist on calendar");
    }

    throw DateException(QString("Invalid date expression format"));
}

QDate DateCalculator::parseContinuousFormat(const QString &dateStr)
{
    // Only try continuous format if the string is mostly numeric (no month names)
    if (dateStr.contains(QRegularExpression(R"([A-Za-z])"))) {
        return QDate(); // Contains letters, not a continuous format
    }

    QString numOnly = extractNumericOnly(dateStr);

    if (numOnly.length() < 6 || numOnly.length() > 8) {
        return QDate(); // Invalid
    }
    
    try {
        int month, day, year;
        
        if (numOnly.length() == 8) {  // MMDDYYYY
            month = numOnly.mid(0, 2).toInt();
            day = numOnly.mid(2, 2).toInt();
            year = numOnly.mid(4, 4).toInt();
        }
        else if (numOnly.length() == 7) {  // MDDYYYY
            month = numOnly.mid(0, 1).toInt();
            day = numOnly.mid(1, 2).toInt();
            year = numOnly.mid(3, 4).toInt();
        }
        else {  // MDYYYY (length 6)
            month = numOnly.mid(0, 1).toInt();
            day = numOnly.mid(1, 1).toInt();
            year = numOnly.mid(2, 4).toInt();
        }
        
        if (isValidMonthDay(month, day)) {
            QDate date(year, month, day);
            if (date.isValid()) {
                return date;
            }
        }
    } catch (...) {
        // Fall through to return invalid date
    }
    
    return QDate(); // Invalid
}

QDate DateCalculator::parseStandardFormats(const QString &dateStr)
{
    QLocale locale(QLocale::English);

    LOG_DEBUG(QString("Trying to parse date: '%1'").arg(dateStr));

    // First check for obviously impossible dates before trying to parse
    QRegularExpression impossibleDatePattern(R"((?:July|Jul)\s+(?:3[2-9]|[4-9]\d)|\b(?:February|Feb)\s+(?:3[0-9]|[4-9]\d)|\b(?:April|Apr|June|Jun|September|Sep|November|Nov)\s+(?:3[1-9]|[4-9]\d))");
    if (impossibleDatePattern.match(dateStr).hasMatch()) {
        LOG_DEBUG(QString("Detected impossible date: '%1'").arg(dateStr));
        return QDate(); // Invalid - will trigger "doesn't exist on calendar" error
    }

    for (const QString &format : m_dateFormats) {
        QDate date = locale.toDate(dateStr, format);
        if (date.isValid()) {
            LOG_DEBUG(QString("Successfully parsed '%1' with format '%2' -> %3").arg(dateStr, format, date.toString()));
            return date;
        }
    }

    LOG_DEBUG(QString("Failed to parse date: '%1' with any standard format").arg(dateStr));
    return QDate(); // Invalid
}

QString DateCalculator::extractNumericOnly(const QString &dateStr)
{
    QString result;
    for (const QChar &c : dateStr) {
        if (c.isDigit()) {
            result.append(c);
        }
    }
    return result;
}

bool DateCalculator::isValidMonthDay(int month, int day)
{
    return (month >= 1 && month <= 12 && day >= 1 && day <= 31);
}

DateResult DateCalculator::handleDateArithmetic(const QString &expression)
{
    QString expr = expression.trimmed();
    
    // Check for date range (two dates with subtraction)
    QRegularExpressionMatch rangeMatch = m_dateRangePattern.match(expr);
    if (rangeMatch.hasMatch()) {
        try {
            QString date1Str = rangeMatch.captured(1).trimmed();
            QString date2Str = rangeMatch.captured(2).trimmed();
            
            QDate date1 = parseDate(date1Str);
            QDate date2 = parseDate(date2Str);

            bool isBusinessDays = m_businessDayPattern.match(expr).hasMatch();
            bool isExclusive = m_exclusiveModePattern.match(expr).hasMatch();
            
            if (isBusinessDays) {
                int days = countBusinessDays(date1, date2, isExclusive);
                // Always show positive result for date differences
                days = std::abs(days);
                return DateResult::success(QString("%1 Business Days").arg(days));
            } else {
                int days = date1.daysTo(date2);
                // Always show positive result for date differences
                days = std::abs(days);
                // Add 1 for inclusive range unless exclusive mode is specified
                if (!isExclusive) {
                    days += 1;
                }
                return DateResult::success(QString("%1 Days").arg(days));
            }
        } catch (const DateException &e) {
            return DateResult::error(QString("Error: %1").arg(e.what()));
        }
    }
    
    // Check for date arithmetic (date +/- days)
    QRegularExpressionMatch arithMatch = m_dateArithmeticPattern.match(expr);
    if (arithMatch.hasMatch()) {
        try {
            QString dateStr = arithMatch.captured(1).trimmed();
            QString operation = arithMatch.captured(2);
            int days = arithMatch.captured(3).toInt();

            LOG_DEBUG(QString("Date arithmetic: '%1' %2 %3 days").arg(dateStr, operation).arg(days));

            QDate date = parseDate(dateStr);
            if (date.isValid()) {
                LOG_DEBUG(QString("Parsed date: %1").arg(date.toString()));
            } else {
                LOG_DEBUG(QString("Failed to parse date: '%1'").arg(dateStr));
            }
            bool isBusinessDays = m_businessDayPattern.match(expr).hasMatch();
            
            QDate result;
            if (isBusinessDays) {
                if (operation == "+") {
                    result = addBusinessDays(date, days);
                } else {
                    result = addBusinessDays(date, -days);
                }
            } else {
                if (operation == "+") {
                    result = date.addDays(days);
                } else {
                    result = date.addDays(-days);
                }
            }

            LOG_DEBUG(QString("Result after %1%2 days: %3").arg(operation).arg(days).arg(result.toString()));

            return DateResult::success(formatDate(result));
        } catch (const DateException &e) {
            return DateResult::error(QString("Error: %1").arg(e.what()));
        }
    }
    
    // Check for single date
    QRegularExpressionMatch singleMatch = m_singleDatePattern.match(expr);
    if (singleMatch.hasMatch()) {
        try {
            QString dateStr = singleMatch.captured(1).trimmed();
            QDate date = parseDate(dateStr);
            return DateResult::success(formatDate(date));
        } catch (const DateException &e) {
            return DateResult::error(QString("Error: %1").arg(e.what()));
        }
    }
    
    return DateResult::error("Error: Invalid date expression format");
}

QDate DateCalculator::addBusinessDays(const QDate &startDate, int days)
{
    QDate currentDate = startDate;
    int remainingDays = qAbs(days);
    int direction = (days > 0) ? 1 : -1;

    while (remainingDays > 0) {
        currentDate = currentDate.addDays(direction);
        // Skip weekends (Qt: Monday = 1, Sunday = 7)
        if (currentDate.dayOfWeek() <= 5) {  // Monday to Friday
            remainingDays--;
        }
    }

    return currentDate;
}

int DateCalculator::countBusinessDays(const QDate &startDate, const QDate &endDate, bool exclusive)
{
    QDate start = startDate;
    QDate end = endDate;

    // Ensure start is before end
    if (start > end) {
        qSwap(start, end);
    }

    int businessDays = 0;
    QDate currentDate;

    if (exclusive) {
        // Exclusive: start from day after start date, end before end date
        currentDate = start.addDays(1);
        end = end.addDays(-1);
    } else {
        // Inclusive: start from start date, include end date
        currentDate = start;
    }

    while (currentDate <= end) {
        // Qt: Monday = 1, Sunday = 7
        if (currentDate.dayOfWeek() <= 5) {  // Monday to Friday
            businessDays++;
        }
        currentDate = currentDate.addDays(1);
    }

    QString mode = exclusive ? "exclusive" : "inclusive";
    LOG_DEBUG(QString("Business days between %1 and %2 (%3): %4").arg(start.toString(), endDate.toString(), mode).arg(businessDays));
    return businessDays;
}

QString DateCalculator::formatDate(const QDate &date)
{
    if (!date.isValid()) {
        return "Invalid Date";
    }

    QLocale locale(QLocale::English);
    return locale.toString(date, "MMMM d, yyyy");
}

bool DateCalculator::isDateFormat(const QString &str) const
{
    // Try to parse the string as a date
    try {
        DateCalculator* nonConstThis = const_cast<DateCalculator*>(this);
        QDate date = nonConstThis->parseDate(str);
        return date.isValid();
    } catch (...) {
        return false;
    }
}
