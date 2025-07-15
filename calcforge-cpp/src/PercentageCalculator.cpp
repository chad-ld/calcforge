#include "PercentageCalculator.h"
#include "Logger.h"
#include <QRegularExpressionMatch>
#include <cmath>

PercentageCalculator::PercentageCalculator()
{
    // Initialize regex pattern for percent() function calls
    // Pattern: percent(arg1, arg2, arg3, ...)
    m_percentFunctionPattern = QRegularExpression(R"(^percent\s*\(\s*(.+)\s*\)$)",
                                                 QRegularExpression::CaseInsensitiveOption);

    LOG_DEBUG("PercentageCalculator initialized with percent() function parsing");
}

PercentageResult PercentageCalculator::calculateExpression(const QString &expression)
{
    QString expr = expression.trimmed();
    LOG_DEBUG(QString("PercentageCalculator: Evaluating expression '%1'").arg(expr));

    // Check if this is a percent() function call
    QRegularExpressionMatch match = m_percentFunctionPattern.match(expr);
    if (!match.hasMatch()) {
        LOG_DEBUG(QString("Not a percent() function call: %1").arg(expr));
        return PercentageResult(); // Invalid result
    }

    // Extract and parse function arguments
    QString argsString = match.captured(1).trimmed();
    LOG_DEBUG(QString("Parsing percent() function arguments: '%1'").arg(argsString));

    // Split arguments by comma, but be careful with nested expressions
    QStringList args;
    QString currentArg;
    int parenLevel = 0;

    for (int i = 0; i < argsString.length(); ++i) {
        QChar c = argsString[i];
        if (c == '(') {
            parenLevel++;
            currentArg += c;
        } else if (c == ')') {
            parenLevel--;
            currentArg += c;
        } else if (c == ',' && parenLevel == 0) {
            args.append(currentArg.trimmed());
            currentArg.clear();
        } else {
            currentArg += c;
        }
    }

    // Add the last argument
    if (!currentArg.isEmpty()) {
        args.append(currentArg.trimmed());
    }

    LOG_DEBUG(QString("Parsed %1 arguments from percent() function").arg(args.size()));
    for (int i = 0; i < args.size(); ++i) {
        LOG_DEBUG(QString("  Arg %1: '%2'").arg(i + 1).arg(args[i]));
    }

    // Parse and calculate based on arguments
    return parsePercentFunction(args);
}

PercentageResult PercentageCalculator::parsePercentFunction(const QStringList &args)
{
    if (args.size() < 2) {
        return PercentageResult("percent() function requires at least 2 arguments");
    }

    // Determine calculation type based on argument count and second argument
    QString secondArg = args[1].trimmed().toLower();

    if (args.size() == 2) {
        // Basic percentage: percent(25%, 1000)
        bool isPercentage1, isPercentage2;
        double arg1 = parseNumericValue(args[0], isPercentage1);
        double arg2 = parseNumericValue(args[1], isPercentage2);

        if (isPercentage1 && !isPercentage2) {
            // percent(25%, 1000) → 25% of 1000
            return calculateBasicPercentage(arg1, arg2);
        } else {
            return PercentageResult("Invalid arguments for basic percentage: first argument must be percentage, second must be value");
        }
    }

    else if (args.size() >= 3) {
        if (secondArg == "%") {
            // Reverse percentage: percent(1000, %, 2000) or percent(1000, %, 2000, .2)
            bool isPercentage1, isPercentage3;
            double part = parseNumericValue(args[0], isPercentage1);
            double whole = parseNumericValue(args[2], isPercentage3);

            if (isPercentage1 || isPercentage3) {
                return PercentageResult("Invalid arguments for reverse percentage: values cannot include % symbol");
            }

            int precision = -1;
            if (args.size() >= 4) {
                precision = parsePrecision(args[3]);
                if (precision == -1) {
                    return PercentageResult("Invalid precision specifier: " + args[3]);
                }
            }

            return calculateReversePercentage(part, whole, precision);
        }

        else if (secondArg == "+" || secondArg == "increase") {
            // Percentage increase: percent(1000, +, 25%)
            bool isPercentage1, isPercentage3;
            double value = parseNumericValue(args[0], isPercentage1);
            double percentage = parseNumericValue(args[2], isPercentage3);

            if (isPercentage1 || !isPercentage3) {
                return PercentageResult("Invalid arguments for percentage increase: first argument must be value, third must be percentage");
            }

            return calculatePercentageIncrease(value, percentage);
        }

        else if (secondArg == "-" || secondArg == "decrease") {
            // Percentage decrease: percent(1000, -, 15%)
            bool isPercentage1, isPercentage3;
            double value = parseNumericValue(args[0], isPercentage1);
            double percentage = parseNumericValue(args[2], isPercentage3);

            if (isPercentage1 || !isPercentage3) {
                return PercentageResult("Invalid arguments for percentage decrease: first argument must be value, third must be percentage");
            }

            return calculatePercentageDecrease(value, percentage);
        }

        else if (secondArg == "to" || secondArg == "change") {
            // Percentage change: percent(1000, to, 1200) or percent(1000, to, 1200, .1)
            bool isPercentage1, isPercentage3;
            double oldValue = parseNumericValue(args[0], isPercentage1);
            double newValue = parseNumericValue(args[2], isPercentage3);

            if (isPercentage1 || isPercentage3) {
                return PercentageResult("Invalid arguments for percentage change: values cannot include % symbol");
            }

            int precision = -1;
            if (args.size() >= 4) {
                precision = parsePrecision(args[3]);
                if (precision == -1) {
                    return PercentageResult("Invalid precision specifier: " + args[3]);
                }
            }

            return calculatePercentageChange(oldValue, newValue, precision);
        }

        else {
            return PercentageResult("Unknown operation: " + secondArg + ". Valid operations: %, +, -, increase, decrease, to, change");
        }
    }

    return PercentageResult("Invalid number of arguments for percent() function");
}

PercentageResult PercentageCalculator::calculateBasicPercentage(double percentage, double value)
{
    if (!validateNumericValue(percentage, "percentage") || !validateNumericValue(value, "value")) {
        return PercentageResult("Invalid numeric values for basic percentage calculation");
    }

    double result = (percentage / 100.0) * value;
    LOG_DEBUG(QString("Basic percentage: %1% of %2 = %3").arg(percentage).arg(value).arg(result));
    return PercentageResult(result, "", PercentageType::BASIC);
}

PercentageResult PercentageCalculator::calculateReversePercentage(double part, double whole, int precision)
{
    if (!validateNumericValue(part, "part value") || !validateNumericValue(whole, "whole value")) {
        return PercentageResult("Invalid numeric values for reverse percentage calculation");
    }

    if (whole == 0.0) {
        return PercentageResult("Cannot calculate percentage: division by zero");
    }

    double result = (part / whole) * 100.0;
    LOG_DEBUG(QString("Reverse percentage: %1 is %2% of %3").arg(part).arg(result).arg(whole));
    return PercentageResult(result, "%", PercentageType::REVERSE);
}

PercentageResult PercentageCalculator::calculatePercentageIncrease(double value, double percentage)
{
    if (!validateNumericValue(value, "base value") || !validateNumericValue(percentage, "percentage")) {
        return PercentageResult("Invalid numeric values for percentage increase calculation");
    }

    double result = value * (1.0 + percentage / 100.0);
    LOG_DEBUG(QString("Percentage increase: %1 + %2% = %3").arg(value).arg(percentage).arg(result));
    return PercentageResult(result, "", PercentageType::INCREASE);
}

PercentageResult PercentageCalculator::calculatePercentageDecrease(double value, double percentage)
{
    if (!validateNumericValue(value, "base value") || !validateNumericValue(percentage, "percentage")) {
        return PercentageResult("Invalid numeric values for percentage decrease calculation");
    }

    double result = value * (1.0 - percentage / 100.0);
    LOG_DEBUG(QString("Percentage decrease: %1 - %2% = %3").arg(value).arg(percentage).arg(result));
    return PercentageResult(result, "", PercentageType::DECREASE);
}

PercentageResult PercentageCalculator::calculatePercentageChange(double oldValue, double newValue, int precision)
{
    if (!validateNumericValue(oldValue, "old value") || !validateNumericValue(newValue, "new value")) {
        return PercentageResult("Invalid numeric values for percentage change calculation");
    }

    if (oldValue == 0.0) {
        return PercentageResult("Cannot calculate percentage change: division by zero");
    }

    double result = ((newValue - oldValue) / oldValue) * 100.0;
    LOG_DEBUG(QString("Percentage change: %1 to %2 = %3% change").arg(oldValue).arg(newValue).arg(result));
    return PercentageResult(result, "%", PercentageType::CHANGE);
}

QString PercentageCalculator::formatResult(const PercentageResult &result)
{
    if (!result.isValid) {
        return QString("Error: %1").arg(result.errorMessage);
    }

    // Determine precision based on result type
    int precision = -1;
    if (result.type == PercentageType::REVERSE || result.type == PercentageType::CHANGE) {
        // For percentage results, we might want to apply custom precision
        // This will be handled by the individual calculation methods
    }

    QString formattedValue = formatNumericValue(result.value, precision);
    return QString("%1%2").arg(formattedValue).arg(result.unit);
}

double PercentageCalculator::parseNumericValue(const QString &str, bool &isPercentage)
{
    QString trimmed = str.trimmed();
    isPercentage = trimmed.endsWith('%');

    if (isPercentage) {
        // Remove the % symbol and parse
        QString numberPart = trimmed.left(trimmed.length() - 1).trimmed();
        bool ok;
        double value = numberPart.toDouble(&ok);
        if (!ok) {
            LOG_DEBUG(QString("Failed to parse percentage value: %1").arg(str));
            return 0.0;
        }
        return value;
    } else {
        // Parse as regular number
        bool ok;
        double value = trimmed.toDouble(&ok);
        if (!ok) {
            LOG_DEBUG(QString("Failed to parse numeric value: %1").arg(str));
            return 0.0;
        }
        return value;
    }
}

int PercentageCalculator::parsePrecision(const QString &str)
{
    QString trimmed = str.trimmed();
    if (!trimmed.startsWith('.')) {
        return -1;
    }

    QString numberPart = trimmed.mid(1); // Remove the '.'
    bool ok;
    int precision = numberPart.toInt(&ok);
    if (!ok || precision < 0 || precision > 10) {
        return -1;
    }

    return precision;
}

QString PercentageCalculator::formatNumericValue(double value, int precision)
{
    if (precision >= 0) {
        // Use specified precision
        return QString::number(value, 'f', precision);
    } else {
        // Auto-format: remove unnecessary decimal places
        if (std::abs(value - std::round(value)) < 1e-10) {
            return QString::number(static_cast<long long>(std::round(value)));
        } else {
            return QString::number(value, 'g', 15);
        }
    }
}

bool PercentageCalculator::validateNumericValue(double value, const QString &context)
{
    // Check for NaN or infinite values
    if (std::isnan(value) || std::isinf(value)) {
        LOG_DEBUG(QString("Invalid %1: NaN or infinite value").arg(context));
        return false;
    }
    
    // Check for extremely large values that might cause overflow
    if (std::abs(value) > 1e15) {
        LOG_DEBUG(QString("Invalid %1: value too large (%2)").arg(context).arg(value));
        return false;
    }
    
    return true;
}



