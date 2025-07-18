#include "CalculationEngine.h"
#include "WorksheetWidget.h"
#include "Logger.h"
#include <QDebug>
#include <QRegularExpression>
#include <functional>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <stdexcept>

CalculationEngine::CalculationEngine() : m_worksheetWidget(nullptr)
{
    // Initialize mathematical constants
    m_constants["pi"] = M_PI;
    m_constants["e"] = M_E;
    
    // Initialize mathematical functions
    // Basic trigonometric functions
    m_functions["sin"] = [](double x) { return std::sin(x); };
    m_functions["cos"] = [](double x) { return std::cos(x); };
    m_functions["tan"] = [](double x) { return std::tan(x); };

    // Inverse trigonometric functions
    m_functions["asin"] = [](double x) { return std::asin(x); };
    m_functions["acos"] = [](double x) { return std::acos(x); };
    m_functions["atan"] = [](double x) { return std::atan(x); };

    // Hyperbolic functions
    m_functions["sinh"] = [](double x) { return std::sinh(x); };
    m_functions["cosh"] = [](double x) { return std::cosh(x); };
    m_functions["tanh"] = [](double x) { return std::tanh(x); };
    m_functions["asinh"] = [](double x) { return std::asinh(x); };
    m_functions["acosh"] = [](double x) { return std::acosh(x); };
    m_functions["atanh"] = [](double x) { return std::atanh(x); };

    // Logarithmic and exponential functions
    m_functions["log"] = [](double x) { return std::log(x); };
    m_functions["log10"] = [](double x) { return std::log10(x); };
    m_functions["log2"] = [](double x) { return std::log2(x); };
    m_functions["exp"] = [](double x) { return std::exp(x); };
    m_functions["sqrt"] = [](double x) { return std::sqrt(x); };

    // Rounding and utility functions
    m_functions["abs"] = [](double x) { return std::abs(x); };
    m_functions["floor"] = [](double x) { return std::floor(x); };
    m_functions["ceil"] = [](double x) { return std::ceil(x); };

    // Angle conversion functions
    m_functions["degrees"] = [](double x) { return x * 180.0 / M_PI; };
    m_functions["radians"] = [](double x) { return x * M_PI / 180.0; };

    // Note: round, pow, factorial, gcd, lcm are handled separately as multi-argument functions
}

QString CalculationEngine::evaluateExpression(const QString &expression, int lineNumber)
{
    QString expr = expression.trimmed();

    // Debug logging for R2 issue
    if (expr.contains("R2")) {
        LOG_DEBUG(QString("EVALUATE EXPRESSION: Line %1, Input: '%2'").arg(lineNumber).arg(expr));
    }

    // Handle empty expressions
    if (expr.isEmpty()) {
        return "";
    }

    // Handle comment lines - leave results blank
    if (isCommentLine(expr)) {
        return "";
    }

    // Try to evaluate as a mathematical expression
    try {
        // Check for timecode function first
        QString timecodeResult = handleTimecodeFunction(expr);
        if (!timecodeResult.isEmpty()) {
            // For timecode functions, try to extract numeric value for LN references
            // If result is a number (frame count), store it; otherwise store 0
            bool ok;
            double numericValue = timecodeResult.toDouble(&ok);
            if (ok) {
                m_lineValues[lineNumber] = numericValue;
            } else {
                m_lineValues[lineNumber] = 0.0; // Store 0 for timecode strings
            }
            return timecodeResult;
        }

        // Check for aspect ratio function
        QString aspectRatioResult = handleAspectRatioFunction(expr);
        if (!aspectRatioResult.isEmpty()) {
            // For aspect ratio functions, store 0 for LN references (dimensions are strings)
            m_lineValues[lineNumber] = 0.0;
            return aspectRatioResult;
        }

        // Check for date function
        QString dateResult = handleDateFunction(expr);
        if (!dateResult.isEmpty()) {
            // For date functions, try to extract numeric value for LN references
            // Date range calculations return "X Days" or "X Business Days"
            double numericValue = extractNumericValueFromResult(dateResult);
            m_lineValues[lineNumber] = numericValue;
            return dateResult;
        }

        // Check for unit conversion FIRST (more specific pattern)
        QString unitResult = handleUnitConversion(expr);
        if (!unitResult.isEmpty()) {
            // For unit conversions, extract the numeric value for LN references
            // The result format is "value unit", so we extract the numeric part
            double numericValue = extractNumericValueFromResult(unitResult);
            m_lineValues[lineNumber] = numericValue;
            return unitResult;
        }

        // Check for currency conversion AFTER unit conversion (broader pattern)
        QString currencyResult = handleCurrencyConversion(expr);
        if (!currencyResult.isEmpty()) {
            // For currency conversions, extract the numeric value for LN references
            // The result format is "value currency", so we extract the numeric part
            double numericValue = extractNumericValueFromResult(currencyResult);
            m_lineValues[lineNumber] = numericValue;
            return currencyResult;
        }

        // Check for percentage calculations AFTER currency conversion
        QString percentageResult = handlePercentageCalculation(expr);
        if (!percentageResult.isEmpty()) {
            // For percentage calculations, extract the numeric value for LN references
            // Results can be raw numbers (250) or percentages (50%)
            double numericValue = extractNumericValueFromResult(percentageResult);
            m_lineValues[lineNumber] = numericValue;
            return percentageResult;
        }

        // Check for statistical functions
        double statResult = handleStatisticalFunctions(expr, lineNumber);
        if (!std::isnan(statResult)) {
            // Store result for future LN references
            m_lineValues[lineNumber] = statResult;
            return formatResult(statResult);
        }



        // Preprocess the expression
        QString processedExpr = preprocessExpression(expr);

        // Process LN references
        processedExpr = processLNReferences(processedExpr);

        // Check if the processed expression contains an error message
        if (processedExpr.startsWith("Error:")) {
            LOG_DEBUG(QString("Expression contains error: %1").arg(processedExpr));
            return processedExpr; // Return error message directly
        }

        // Parse and evaluate
        double result = parseExpression(processedExpr);

        // Store result for future LN references
        m_lineValues[lineNumber] = result;

        // Format and return result
        return formatResult(result);

    } catch (const std::exception &e) {
        // If parsing fails, return placeholder
        LOG_DEBUG(QString("Expression evaluation failed: %1, Error: %2").arg(expr).arg(e.what()));
        return "= " + expr;
    } catch (...) {
        // If parsing fails, return placeholder
        LOG_DEBUG(QString("Expression evaluation failed with unknown error: %1").arg(expr));
        return "= " + expr;
    }
}

QStringList CalculationEngine::evaluateExpressions(const QStringList &expressions)
{
    QStringList results;
    
    for (int i = 0; i < expressions.size(); ++i) {
        QString result = evaluateExpression(expressions[i], i + 1);
        results.append(result);
    }
    
    return results;
}

bool CalculationEngine::isCommentLine(const QString &expression)
{
    return expression.trimmed().startsWith(":::");
}

QString CalculationEngine::formatResult(double value)
{
    // Handle special values
    if (std::isnan(value)) {
        return "NaN";
    }
    if (std::isinf(value)) {
        return value > 0 ? "∞" : "-∞";
    }
    
    // Round to 6 decimal places to avoid floating point noise
    double rounded = std::round(value * 1000000.0) / 1000000.0;
    
    // Convert to integer if it's a whole number
    if (std::abs(rounded - std::round(rounded)) < 1e-9) {
        return QString::number(static_cast<long long>(std::round(rounded)));
    }
    
    // Format as decimal, removing trailing zeros
    QString result = QString::number(rounded, 'f', 6);
    
    // Remove trailing zeros and decimal point if not needed
    while (result.endsWith('0') && result.contains('.')) {
        result.chop(1);
    }
    if (result.endsWith('.')) {
        result.chop(1);
    }
    
    return result;
}

QString CalculationEngine::preprocessExpression(const QString &expr)
{
    QString processed = expr;

    // Debug logging for R2 issue
    if (expr.contains("R2")) {
        LOG_DEBUG(QString("PREPROCESSING: Input: '%1'").arg(expr));
    }

    // Convert ^ to ** for exponentiation
    processed.replace("^", "**");

    // Handle padded numbers (leading zeros) - convert 010 to 10, etc.
    QRegularExpression paddedNumbers(R"(\b0+(\d+)\b)");
    processed.replace(paddedNumbers, R"(\1)");

    // Handle cases where we might have just zeros
    QRegularExpression justZeros(R"(\b0+\b)");
    processed.replace(justZeros, "0");

    // Debug logging for R2 issue
    if (expr.contains("R2") || processed.contains("R2")) {
        LOG_DEBUG(QString("PREPROCESSING: Output: '%1'").arg(processed));
    }

    return processed;
}

bool CalculationEngine::isIncompleteExpression(const QString &expr)
{
    QString trimmed = expr.trimmed();
    
    // Skip evaluation of single letters or very short expressions
    if (trimmed.length() <= 2 && trimmed.contains(QRegularExpression("[a-zA-Z]"))) {
        return true;
    }
    
    // Check for incomplete function names
    QRegularExpression letterOnlyRegex("^[a-zA-Z]+$");
    if (letterOnlyRegex.match(trimmed).hasMatch() && trimmed.length() < 3) {
        return true;
    }
    
    // Check for unmatched parentheses
    int openParens = trimmed.count('(');
    int closeParens = trimmed.count(')');
    if (openParens != closeParens) {
        return true;
    }
    
    // Check for trailing operators
    QRegularExpression trailingOpRegex("[+\\-*/^]$");
    if (trailingOpRegex.match(trimmed).hasMatch()) {
        return true;
    }
    
    return false;
}

double CalculationEngine::parseExpression(const QString &expr)
{
    int pos = 0;
    double result = parseAddSub(expr, pos);
    
    // Make sure we consumed the entire expression
    skipWhitespace(expr, pos);
    if (pos < expr.length()) {
        throw std::runtime_error("Unexpected characters at end of expression");
    }
    
    return result;
}

double CalculationEngine::parseAddSub(const QString &expr, int &pos)
{
    double left = parseMulDiv(expr, pos);
    
    while (pos < expr.length()) {
        skipWhitespace(expr, pos);
        if (pos >= expr.length()) break;
        
        QChar op = expr[pos];
        if (op == '+' || op == '-') {
            pos++;
            double right = parseMulDiv(expr, pos);
            if (op == '+') {
                left += right;
            } else {
                left -= right;
            }
        } else {
            break;
        }
    }
    
    return left;
}

double CalculationEngine::parseMulDiv(const QString &expr, int &pos)
{
    double left = parsePower(expr, pos);
    
    while (pos < expr.length()) {
        skipWhitespace(expr, pos);
        if (pos >= expr.length()) break;
        
        QChar op = expr[pos];
        if (op == '*' || op == '/') {
            pos++;
            double right = parsePower(expr, pos);
            if (op == '*') {
                left *= right;
            } else {
                if (std::abs(right) < 1e-15) {
                    throw std::runtime_error("Division by zero");
                }
                left /= right;
            }
        } else {
            break;
        }
    }
    
    return left;
}

double CalculationEngine::parsePower(const QString &expr, int &pos)
{
    double left = parseFactor(expr, pos);
    
    skipWhitespace(expr, pos);
    if (pos < expr.length() - 1 && expr.mid(pos, 2) == "**") {
        pos += 2;
        double right = parsePower(expr, pos); // Right associative
        left = std::pow(left, right);
    }
    
    return left;
}

double CalculationEngine::parseFactor(const QString &expr, int &pos)
{
    skipWhitespace(expr, pos);
    
    if (pos >= expr.length()) {
        throw std::runtime_error("Unexpected end of expression");
    }
    
    // Handle unary minus
    if (expr[pos] == '-') {
        pos++;
        return -parseFactor(expr, pos);
    }
    
    // Handle unary plus
    if (expr[pos] == '+') {
        pos++;
        return parseFactor(expr, pos);
    }
    
    // Handle parentheses
    if (expr[pos] == '(') {
        pos++;
        double result = parseAddSub(expr, pos);
        skipWhitespace(expr, pos);
        if (pos >= expr.length() || expr[pos] != ')') {
            throw std::runtime_error("Missing closing parenthesis");
        }
        pos++;
        return result;
    }
    
    // Handle numbers
    if (expr[pos].isDigit() || expr[pos] == '.') {
        return parseNumber(expr, pos);
    }
    
    // Handle functions and constants
    if (expr[pos].isLetter()) {
        return parseFunction(expr, pos);
    }
    
    throw std::runtime_error("Unexpected character: " + QString(expr[pos]).toStdString());
}

double CalculationEngine::parseNumber(const QString &expr, int &pos)
{
    int start = pos;
    
    // Parse integer part
    while (pos < expr.length() && expr[pos].isDigit()) {
        pos++;
    }
    
    // Parse decimal part
    if (pos < expr.length() && expr[pos] == '.') {
        pos++;
        while (pos < expr.length() && expr[pos].isDigit()) {
            pos++;
        }
    }
    
    // Parse scientific notation
    if (pos < expr.length() && (expr[pos] == 'e' || expr[pos] == 'E')) {
        pos++;
        if (pos < expr.length() && (expr[pos] == '+' || expr[pos] == '-')) {
            pos++;
        }
        while (pos < expr.length() && expr[pos].isDigit()) {
            pos++;
        }
    }
    
    QString numberStr = expr.mid(start, pos - start);
    bool ok;
    double value = numberStr.toDouble(&ok);
    
    if (!ok) {
        throw std::runtime_error("Invalid number: " + numberStr.toStdString());
    }
    
    return value;
}

double CalculationEngine::parseFunction(const QString &expr, int &pos)
{
    int start = pos;
    
    // Parse function name
    while (pos < expr.length() && (expr[pos].isLetterOrNumber() || expr[pos] == '_')) {
        pos++;
    }
    
    QString name = expr.mid(start, pos - start);
    
    // Check if it's a constant
    if (m_constants.contains(name)) {
        return m_constants[name];
    }
    
    // Check if it's a function
    if (m_functions.contains(name)) {
        skipWhitespace(expr, pos);
        if (pos >= expr.length() || expr[pos] != '(') {
            throw std::runtime_error("Function " + name.toStdString() + " requires parentheses");
        }
        pos++; // Skip '('

        double arg = parseAddSub(expr, pos);

        skipWhitespace(expr, pos);
        if (pos >= expr.length() || expr[pos] != ')') {
            throw std::runtime_error("Missing closing parenthesis for function " + name.toStdString());
        }
        pos++; // Skip ')'

        return m_functions[name](arg);
    }

    // Check for multi-argument functions (round, truncate, TR are all aliases)
    if (name == "round" || name == "truncate" || name == "TR") {
        skipWhitespace(expr, pos);
        if (pos >= expr.length() || expr[pos] != '(') {
            throw std::runtime_error("Function " + name.toStdString() + " requires parentheses");
        }
        pos++; // Skip '('

        double value = parseAddSub(expr, pos);

        skipWhitespace(expr, pos);
        if (pos < expr.length() && expr[pos] == ',') {
            pos++; // Skip ','
            double decimals = parseAddSub(expr, pos);

            skipWhitespace(expr, pos);
            if (pos >= expr.length() || expr[pos] != ')') {
                throw std::runtime_error("Missing closing parenthesis for function " + name.toStdString());
            }
            pos++; // Skip ')'

            // Implement round with decimal places
            double factor = std::pow(10.0, decimals);
            return std::round(value * factor) / factor;
        } else {
            // Single argument round
            skipWhitespace(expr, pos);
            if (pos >= expr.length() || expr[pos] != ')') {
                throw std::runtime_error("Missing closing parenthesis for function " + name.toStdString());
            }
            pos++; // Skip ')'

            return std::round(value);
        }
    }

    // Check for pow function (two arguments)
    if (name == "pow") {
        skipWhitespace(expr, pos);
        if (pos >= expr.length() || expr[pos] != '(') {
            throw std::runtime_error("Function pow requires parentheses");
        }
        pos++; // Skip '('

        double base = parseAddSub(expr, pos);

        skipWhitespace(expr, pos);
        if (pos >= expr.length() || expr[pos] != ',') {
            throw std::runtime_error("Function pow requires two arguments: pow(base, exponent)");
        }
        pos++; // Skip ','

        double exponent = parseAddSub(expr, pos);

        skipWhitespace(expr, pos);
        if (pos >= expr.length() || expr[pos] != ')') {
            throw std::runtime_error("Missing closing parenthesis for function pow");
        }
        pos++; // Skip ')'

        return std::pow(base, exponent);
    }

    // Check for factorial function (single argument, integer)
    if (name == "factorial") {
        skipWhitespace(expr, pos);
        if (pos >= expr.length() || expr[pos] != '(') {
            throw std::runtime_error("Function factorial requires parentheses");
        }
        pos++; // Skip '('

        double value = parseAddSub(expr, pos);

        skipWhitespace(expr, pos);
        if (pos >= expr.length() || expr[pos] != ')') {
            throw std::runtime_error("Missing closing parenthesis for function factorial");
        }
        pos++; // Skip ')'

        // Check if value is a non-negative integer
        if (value < 0 || value != std::floor(value)) {
            throw std::runtime_error("Factorial requires a non-negative integer");
        }

        // Calculate factorial
        int n = static_cast<int>(value);
        if (n > 170) { // Prevent overflow (170! is close to double limit)
            throw std::runtime_error("Factorial argument too large (max 170)");
        }

        double result = 1.0;
        for (int i = 2; i <= n; ++i) {
            result *= i;
        }
        return result;
    }

    // Check for gcd function (two arguments)
    if (name == "gcd") {
        skipWhitespace(expr, pos);
        if (pos >= expr.length() || expr[pos] != '(') {
            throw std::runtime_error("Function gcd requires parentheses");
        }
        pos++; // Skip '('

        double a = parseAddSub(expr, pos);

        skipWhitespace(expr, pos);
        if (pos >= expr.length() || expr[pos] != ',') {
            throw std::runtime_error("Function gcd requires two arguments: gcd(a, b)");
        }
        pos++; // Skip ','

        double b = parseAddSub(expr, pos);

        skipWhitespace(expr, pos);
        if (pos >= expr.length() || expr[pos] != ')') {
            throw std::runtime_error("Missing closing parenthesis for function gcd");
        }
        pos++; // Skip ')'

        // Convert to integers and calculate GCD
        long long ia = static_cast<long long>(std::abs(a));
        long long ib = static_cast<long long>(std::abs(b));

        // Euclidean algorithm
        while (ib != 0) {
            long long temp = ib;
            ib = ia % ib;
            ia = temp;
        }

        return static_cast<double>(ia);
    }

    // Check for lcm function (two arguments)
    if (name == "lcm") {
        skipWhitespace(expr, pos);
        if (pos >= expr.length() || expr[pos] != '(') {
            throw std::runtime_error("Function lcm requires parentheses");
        }
        pos++; // Skip '('

        double a = parseAddSub(expr, pos);

        skipWhitespace(expr, pos);
        if (pos >= expr.length() || expr[pos] != ',') {
            throw std::runtime_error("Function lcm requires two arguments: lcm(a, b)");
        }
        pos++; // Skip ','

        double b = parseAddSub(expr, pos);

        skipWhitespace(expr, pos);
        if (pos >= expr.length() || expr[pos] != ')') {
            throw std::runtime_error("Missing closing parenthesis for function lcm");
        }
        pos++; // Skip ')'

        // Convert to integers and calculate LCM
        long long ia = static_cast<long long>(std::abs(a));
        long long ib = static_cast<long long>(std::abs(b));

        if (ia == 0 || ib == 0) {
            return 0.0;
        }

        // Calculate GCD first
        long long gcd_val = ia;
        long long temp_b = ib;
        while (temp_b != 0) {
            long long temp = temp_b;
            temp_b = gcd_val % temp_b;
            gcd_val = temp;
        }

        // LCM = (a * b) / GCD(a, b)
        return static_cast<double>((ia / gcd_val) * ib);
    }

    throw std::runtime_error("Unknown function or constant: " + name.toStdString());
}

void CalculationEngine::skipWhitespace(const QString &expr, int &pos)
{
    while (pos < expr.length() && expr[pos].isSpace()) {
        pos++;
    }
}

QString CalculationEngine::processLNReferences(const QString &expr)
{
    QString processed = expr;

    // First, handle cross-sheet references (S.SheetName.LN#)
    QRegularExpression crossSheetRegex(R"(\bS\.([^.]+)\.LN(\d+)\b)", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatchIterator crossSheetIterator = crossSheetRegex.globalMatch(processed);

    // Process cross-sheet matches from right to left to avoid position shifts
    QList<QRegularExpressionMatch> crossSheetMatches;
    while (crossSheetIterator.hasNext()) {
        crossSheetMatches.prepend(crossSheetIterator.next());
    }

    LOG_DEBUG(QString("Processing expression: '%1', found %2 cross-sheet references").arg(expr).arg(crossSheetMatches.size()));

    for (const QRegularExpressionMatch &match : crossSheetMatches) {
        QString sheetName = match.captured(1).trimmed();
        int lineNumber = match.captured(2).toInt();

        LOG_DEBUG(QString("Found cross-sheet reference: S.%1.LN%2").arg(sheetName).arg(lineNumber));

        QString replacement = getCrossSheetValue(sheetName, lineNumber);

        LOG_DEBUG(QString("Cross-sheet value for S.%1.LN%2: '%3'").arg(sheetName).arg(lineNumber).arg(replacement));

        // If it's a numeric value, use it directly; if it's an error, keep it as text
        bool isNumeric;
        double numericValue = replacement.toDouble(&isNumeric);
        if (isNumeric) {
            replacement = QString::number(numericValue, 'g', 15);
        }
        // If it's an error message, we'll leave it as text which will cause evaluation to fail gracefully

        processed.replace(match.capturedStart(), match.capturedLength(), replacement);
    }

    // Then handle regular LN references (LN#) - but skip if we have error messages
    if (!processed.contains("Error:")) {
        QRegularExpression lnRegex(R"(\bLN(\d+)\b)", QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatchIterator iterator = lnRegex.globalMatch(processed);

        // Process matches from right to left to avoid position shifts
        QList<QRegularExpressionMatch> matches;
        while (iterator.hasNext()) {
            matches.prepend(iterator.next());
        }

        for (const QRegularExpressionMatch &match : matches) {
            int lineNumber = match.captured(1).toInt();
            double value = getLineValue(lineNumber);

            // Replace the LN reference with the numeric value
            QString replacement = QString::number(value, 'g', 15); // Use 'g' format for clean output
            processed.replace(match.capturedStart(), match.capturedLength(), replacement);
        }
    }

    LOG_DEBUG(QString("Final processed expression: '%1' -> '%2'").arg(expr).arg(processed));
    return processed;
}

double CalculationEngine::getLineValue(int lineNumber) const
{
    return m_lineValues.value(lineNumber, 0.0); // Return 0.0 if line not found
}

bool CalculationEngine::hasLineValue(int lineNumber) const
{
    return m_lineValues.contains(lineNumber);
}

void CalculationEngine::clearLineValues()
{
    m_lineValues.clear();
}

void CalculationEngine::setWorksheetWidget(WorksheetWidget *widget)
{
    m_worksheetWidget = widget;
}

void CalculationEngine::updateLineValuesAfterChange(int insertionPoint, int linesDelta)
{
    if (linesDelta == 0) {
        return; // No change
    }

    // Create a new map with updated line numbers
    QHash<int, double> updatedValues;

    for (auto it = m_lineValues.begin(); it != m_lineValues.end(); ++it) {
        int oldLineNumber = it.key();
        double value = it.value();

        int newLineNumber = oldLineNumber;

        if (linesDelta > 0) {
            // Lines were inserted
            if (oldLineNumber >= insertionPoint) {
                newLineNumber = oldLineNumber + linesDelta;
            }
        } else {
            // Lines were deleted
            int deletionEnd = insertionPoint - linesDelta - 1; // End of deleted range

            if (oldLineNumber >= insertionPoint && oldLineNumber <= deletionEnd) {
                // This line was deleted, don't include it
                continue;
            } else if (oldLineNumber > deletionEnd) {
                // This line comes after the deletion, shift it down
                newLineNumber = oldLineNumber + linesDelta; // linesDelta is negative
            }
        }



        updatedValues[newLineNumber] = value;
    }

    // Replace the old values with updated ones
    m_lineValues = updatedValues;
}

double CalculationEngine::handleStatisticalFunctions(const QString &expr, int lineNumber)
{
    // Pattern to match statistical functions
    QRegularExpression statPattern(R"(^(sum|mean|median|mode|min|max|count|product|variance|stdev|std|range|geomean|harmmean|sumsq|perc5|perc95|meanfps)\s*\(\s*(.*?)\s*\)$)",
                                   QRegularExpression::CaseInsensitiveOption);

    QRegularExpressionMatch match = statPattern.match(expr.trimmed());
    if (!match.hasMatch()) {
        return std::numeric_limits<double>::quiet_NaN(); // Not a statistical function
    }

    QString funcName = match.captured(1).toLower();
    QString rangeExpr = match.captured(2);
    QString originalRangeExpr = rangeExpr; // Store original for percentile functions

    // Debug logging for R2 issue
    LOG_DEBUG(QString("STATISTICAL FUNCTION: %1, rangeExpr: '%2'").arg(funcName).arg(rangeExpr));

    // Process cross-sheet references in the range expression first
    rangeExpr = processLNReferences(rangeExpr);
    LOG_DEBUG(QString("STATISTICAL FUNCTION: After processing cross-sheet refs: '%1'").arg(rangeExpr));

    // If cross-sheet references couldn't be resolved (contain "Error:"), defer processing
    if (rangeExpr.contains("Error:")) {
        LOG_DEBUG(QString("STATISTICAL FUNCTION: Deferring due to unresolved cross-sheet references"));
        return std::numeric_limits<double>::quiet_NaN(); // Not ready for processing yet
    }

    // Check for decimal rounding parameter (.X format)
    int roundDecimals = -1; // -1 means no rounding
    QStringList parts = rangeExpr.split(',');

    // Only check for rounding if there are 2+ parameters
    if (parts.size() >= 2) {
        QString lastPart = parts.last().trimmed();
        QRegularExpression roundPattern(R"(^\.(\d+)$)");
        QRegularExpressionMatch roundMatch = roundPattern.match(lastPart);

        if (roundMatch.hasMatch()) {
            LOG_DEBUG(QString("ROUND PARAMETER: Found .%1 in '%2'").arg(roundMatch.captured(1)).arg(rangeExpr));
            roundDecimals = roundMatch.captured(1).toInt();

            // Remove the round parameter from the range expression
            parts.removeLast();
            rangeExpr = parts.join(",").trimmed();
            LOG_DEBUG(QString("ROUND PARAMETER: Cleaned to '%1'").arg(rangeExpr));
        }
    }

    // Get values from the range expression
    QList<double> values = getValuesFromRange(rangeExpr, lineNumber);

    if (values.isEmpty()) {
        return 0.0; // Return 0 for empty ranges
    }

    // Apply the statistical function
    double result = std::numeric_limits<double>::quiet_NaN();

    try {
        if (funcName == "sum") {
            result = std::accumulate(values.begin(), values.end(), 0.0);
        }
        else if (funcName == "mean") {
            result = std::accumulate(values.begin(), values.end(), 0.0) / values.size();
        }
        else if (funcName == "min") {
            result = *std::min_element(values.begin(), values.end());
        }
        else if (funcName == "max") {
            result = *std::max_element(values.begin(), values.end());
        }
        else if (funcName == "count") {
            result = static_cast<double>(values.size());
        }
        else if (funcName == "product") {
            result = std::accumulate(values.begin(), values.end(), 1.0, std::multiplies<double>());
        }
        else if (funcName == "range") {
            auto minmax = std::minmax_element(values.begin(), values.end());
            result = *minmax.second - *minmax.first;
        }
        else if (funcName == "median") {
            QList<double> sortedValues = values;
            std::sort(sortedValues.begin(), sortedValues.end());
            int size = sortedValues.size();
            if (size % 2 == 0) {
                result = (sortedValues[size/2 - 1] + sortedValues[size/2]) / 2.0;
            } else {
                result = sortedValues[size/2];
            }
        }
        else if (funcName == "variance") {
            if (values.size() <= 1) {
                result = 0.0;
            } else {
                double mean = std::accumulate(values.begin(), values.end(), 0.0) / values.size();
                double variance = 0.0;
                for (double value : values) {
                    variance += (value - mean) * (value - mean);
                }
                result = variance / (values.size() - 1); // Sample variance
            }
        }
        else if (funcName == "stdev" || funcName == "std") {
            if (values.size() <= 1) {
                result = 0.0;
            } else {
                double mean = std::accumulate(values.begin(), values.end(), 0.0) / values.size();
                double variance = 0.0;
                for (double value : values) {
                    variance += (value - mean) * (value - mean);
                }
                result = std::sqrt(variance / (values.size() - 1)); // Sample standard deviation
            }
        }
        else if (funcName == "geomean") {
            double product = 1.0;
            for (double value : values) {
                if (value <= 0) {
                    return std::numeric_limits<double>::quiet_NaN(); // Geometric mean requires positive values
                }
                product *= value;
            }
            result = std::pow(product, 1.0 / values.size());
        }
        else if (funcName == "harmmean") {
            double sum = 0.0;
            for (double value : values) {
                if (value == 0) {
                    return std::numeric_limits<double>::quiet_NaN(); // Harmonic mean undefined for zero
                }
                sum += 1.0 / value;
            }
            result = values.size() / sum;
        }
        else if (funcName == "sumsq") {
            double sumSquares = 0.0;
            for (double value : values) {
                sumSquares += value * value;
            }
            result = sumSquares;
        }
        else if (funcName == "mode") {
            // Find the most frequently occurring value
            QMap<double, int> frequency;
            for (double value : values) {
                frequency[value]++;
            }

            int maxCount = 0;
            double modeValue = 0.0;
            for (auto it = frequency.begin(); it != frequency.end(); ++it) {
                if (it.value() > maxCount) {
                    maxCount = it.value();
                    modeValue = it.key();
                }
            }

            // If all values appear only once, return the first value
            if (maxCount == 1) {
                result = values.isEmpty() ? 0.0 : values.first();
            } else {
                result = modeValue;
            }
        }
        else if (funcName == "perc5") {
            // 5th percentile with optional method parameter
            result = calculatePercentile(values, 0.05, originalRangeExpr);
        }
        else if (funcName == "perc95") {
            // 95th percentile with optional method parameter
            result = calculatePercentile(values, 0.95, originalRangeExpr);
        }
        else if (funcName == "meanfps") {
            // Mean frames per second (harmonic mean approach: 1/mean_frame_time)
            // This gives the most accurate representation of actual performance for graphics/game programming
            if (values.isEmpty()) {
                result = 0.0;
            } else {
                double sum = 0.0;
                LOG_DEBUG(QString("meanfps: Processing %1 values").arg(values.size()));
                for (double value : values) {
                    LOG_DEBUG(QString("meanfps: Frame time value = %1").arg(value));
                    if (value <= 0) {
                        return std::numeric_limits<double>::quiet_NaN(); // Invalid frame time
                    }
                    sum += value; // Sum of frame times
                }
                double meanFrameTime = sum / values.size(); // Mean frame time
                LOG_DEBUG(QString("meanfps: Sum = %1, Count = %2, Mean frame time = %3").arg(sum).arg(values.size()).arg(meanFrameTime));
                result = 1.0 / meanFrameTime; // Mean FPS = 1 / mean frame time
                LOG_DEBUG(QString("meanfps: Final result = %1 FPS").arg(result));
            }
        }

        // Apply rounding if specified
        if (roundDecimals >= 0 && !std::isnan(result)) {
            double factor = std::pow(10.0, roundDecimals);
            result = std::round(result * factor) / factor;
        }

        return result;

    } catch (const std::exception &e) {
        return 0.0; // Return 0 on error
    }

    return std::numeric_limits<double>::quiet_NaN(); // Unknown function
}

QList<double> CalculationEngine::getValuesFromRange(const QString &rangeExpr, int currentLine)
{
    QList<double> values;
    QString trimmedRange = rangeExpr.trimmed();

    // Handle empty range - use all lines above current
    if (trimmedRange.isEmpty()) {
        // Use the same content-based approach as "above"
        if (m_worksheetWidget) {
            QString content = m_worksheetWidget->getContent();
            QStringList lines = content.split('\n');

            // Scan lines above the current line
            for (int i = 0; i < currentLine - 1; ++i) {  // i is 0-based, currentLine is 1-based
                QString line = lines[i].trimmed();

                // Skip empty lines and comment lines
                if (line.isEmpty() || line.startsWith(":::")) {
                    continue;
                }

                int lineNumber = i + 1;  // Convert to 1-based line number

                // Only count lines that contain actual numeric values, not expressions
                // Check if the line content is a simple number (not an expression)
                bool ok;
                double numericValue = line.toDouble(&ok);
                if (ok && !line.isEmpty() && line.contains(QRegularExpression("[0-9]"))) {
                    // This is a simple numeric value, count it
                    if (m_lineValues.contains(lineNumber)) {
                        // Use the calculated value if available (handles cases like 0292 -> 292)
                        values.append(getLineValue(lineNumber));
                    } else {
                        // Use the parsed value
                        values.append(numericValue);
                    }
                }
                // Skip lines with expressions (like count(below), sum(), etc.) even if they have calculated values
            }
        } else {
            // Fallback to old method if worksheet widget not available
            for (int i = 1; i < currentLine; ++i) {
                if (m_lineValues.contains(i)) {
                    values.append(getLineValue(i));
                }
            }
        }
        return values;
    }

    // Handle special keywords
    if (trimmedRange.toLower() == "above") {
        // Get all lines above current line by examining the actual worksheet content
        // This approach is more reliable than checking the line values hash

        if (m_worksheetWidget) {
            QString content = m_worksheetWidget->getContent();
            QStringList lines = content.split('\n');

            // Scan lines above the current line
            for (int i = 0; i < currentLine - 1; ++i) {  // i is 0-based, currentLine is 1-based
                QString line = lines[i].trimmed();

                // Skip empty lines and comment lines
                if (line.isEmpty() || line.startsWith(":::")) {
                    continue;
                }

                int lineNumber = i + 1;  // Convert to 1-based line number

                // Only count lines that contain actual numeric values, not expressions
                // Check if the line content is a simple number (not an expression)
                bool ok;
                double numericValue = line.toDouble(&ok);
                if (ok && !line.isEmpty() && line.contains(QRegularExpression("[0-9]"))) {
                    // This is a simple numeric value, count it
                    if (m_lineValues.contains(lineNumber)) {
                        // Use the calculated value if available (handles cases like 0292 -> 292)
                        values.append(getLineValue(lineNumber));
                    } else {
                        // Use the parsed value
                        values.append(numericValue);
                    }
                }
                // Skip lines with expressions (like count(below), sum(), etc.) even if they have calculated values
            }
        } else {
            // Fallback to old method if worksheet widget not available
            for (int i = 1; i < currentLine; ++i) {
                if (m_lineValues.contains(i)) {
                    values.append(getLineValue(i));
                }
            }
        }
        return values;
    }

    if (trimmedRange.toLower() == "below") {
        // Get all lines below current line by examining the actual worksheet content
        // This approach is more reliable than checking the line values hash

        if (m_worksheetWidget) {
            QString content = m_worksheetWidget->getContent();
            QStringList lines = content.split('\n');

            // Scan lines below the current line and evaluate them if they contain expressions
            for (int i = currentLine; i < lines.size(); ++i) {  // i starts at currentLine (0-based)
                QString line = lines[i].trimmed();

                // Skip empty lines and comment lines
                if (line.isEmpty() || line.startsWith(":::")) {
                    continue;
                }

                int lineNumber = i + 1;  // Convert to 1-based line number

                // Only count lines that contain actual numeric values, not expressions
                // Check if the line content is a simple number (not an expression)
                bool ok;
                double numericValue = line.toDouble(&ok);
                if (ok && !line.isEmpty() && line.contains(QRegularExpression("[0-9]"))) {
                    // This is a simple numeric value, count it
                    if (m_lineValues.contains(lineNumber)) {
                        // Use the calculated value if available (handles cases like 0292 -> 292)
                        values.append(getLineValue(lineNumber));
                    } else {
                        // Use the parsed value
                        values.append(numericValue);
                    }
                }
                // Skip lines with expressions (like count(below), sum(), etc.) even if they have calculated values
            }
        }
        return values;
    }

    // Handle comma-separated expressions first (may contain ranges)
    if (trimmedRange.contains(',')) {
        // Comma-separated format like "1,3,5" or "1,3,5,.2" (always line references for range functions)
        QStringList parts = trimmedRange.split(',');
        for (const QString &part : parts) {
            QString trimmedPart = part.trimmed();

            // Skip decimal rounding parameters (.digits) - they're handled elsewhere
            if (QRegularExpression(R"(^\.(\d+)$)").match(trimmedPart).hasMatch()) {
                continue;
            }

            // Skip method parameters for percentile functions
            if (trimmedPart.toLower() == "linear" || trimmedPart.toLower() == "nearest") {
                continue;
            }

            // Check if this part is a range (like "4-10")
            if (trimmedPart.contains('-') && !trimmedPart.startsWith('-')) {
                QStringList rangeParts = trimmedPart.split('-');
                if (rangeParts.size() == 2) {
                    bool ok1, ok2;
                    int start = rangeParts[0].trimmed().toInt(&ok1);
                    int end = rangeParts[1].trimmed().toInt(&ok2);
                    if (ok1 && ok2) {
                        LOG_DEBUG(QString("Comma-separated range parsing: %1-%2").arg(start).arg(end));

                        // Use worksheet content approach for better reliability
                        if (m_worksheetWidget) {
                            QString content = m_worksheetWidget->getContent();
                            QStringList lines = content.split('\n');

                            for (int i = start; i <= end; ++i) {
                                if (i >= 1 && i <= lines.size()) {
                                    QString line = lines[i - 1].trimmed(); // Convert to 0-based index
                                    LOG_DEBUG(QString("Comma range line %1: '%2'").arg(i).arg(line));

                                    // Only count lines that contain actual numeric values, not expressions
                                    bool ok;
                                    double numericValue = line.toDouble(&ok);
                                    if (ok && !line.isEmpty() && line.contains(QRegularExpression("[0-9]"))) {
                                        LOG_DEBUG(QString("Comma range adding value %1 from line %2").arg(numericValue).arg(i));
                                        values.append(numericValue);
                                    }
                                }
                            }
                        } else {
                            // Fallback to old method if worksheet widget not available
                            for (int i = start; i <= end; ++i) {
                                if (m_lineValues.contains(i)) { // Only include lines that have been evaluated
                                    values.append(getLineValue(i));
                                }
                            }
                        }
                        continue; // Skip the rest of the processing for this part
                    }
                }
            }

            // Parse as line number (range functions always use line references)
            bool ok;
            int lineNum = trimmedPart.toInt(&ok);
            if (ok && m_lineValues.contains(lineNum)) { // Only include lines that have been evaluated
                values.append(getLineValue(lineNum));
            } else if (ok) {
                // Line number is valid but not yet evaluated - this can happen during dependency resolution
                // For now, skip it (the function will be re-evaluated later)
                LOG_DEBUG(QString("Line %1 not yet evaluated, skipping for now").arg(lineNum));
            } else {
                // Try to parse as a direct numeric value (for processed cross-sheet references like S.Sheet.LN1)
                double numericValue = trimmedPart.toDouble(&ok);
                if (ok) {
                    values.append(numericValue);
                }
            }
        }
    }
    else if (trimmedRange.contains('-') && !trimmedRange.startsWith('-')) {
        // Range format like "1-5" (without commas)
        QStringList parts = trimmedRange.split('-');
        if (parts.size() == 2) {
            bool ok1, ok2;
            int start = parts[0].trimmed().toInt(&ok1);
            int end = parts[1].trimmed().toInt(&ok2);
            if (ok1 && ok2) {
                LOG_DEBUG(QString("Range parsing: %1-%2").arg(start).arg(end));

                // Use worksheet content approach for better reliability
                if (m_worksheetWidget) {
                    QString content = m_worksheetWidget->getContent();
                    QStringList lines = content.split('\n');

                    for (int i = start; i <= end; ++i) {
                        if (i >= 1 && i <= lines.size()) {
                            QString line = lines[i - 1].trimmed(); // Convert to 0-based index
                            LOG_DEBUG(QString("Range line %1: '%2'").arg(i).arg(line));

                            // Only count lines that contain actual numeric values, not expressions
                            bool ok;
                            double numericValue = line.toDouble(&ok);
                            if (ok && !line.isEmpty() && line.contains(QRegularExpression("[0-9]"))) {
                                LOG_DEBUG(QString("Range adding value %1 from line %2").arg(numericValue).arg(i));
                                values.append(numericValue);
                            }
                        }
                    }
                } else {
                    // Fallback to old method if worksheet widget not available
                    for (int i = start; i <= end; ++i) {
                        if (m_lineValues.contains(i)) { // Only include lines that have been evaluated
                            values.append(getLineValue(i));
                        }
                    }
                }
            }
        }
    }
    else {
        // Single value - try line number first, then direct numeric value
        bool ok;
        int lineNum = trimmedRange.toInt(&ok);
        if (ok && m_lineValues.contains(lineNum)) { // Only include lines that have been evaluated
            values.append(getLineValue(lineNum));
        } else {
            // Try to parse as a direct numeric value (for processed cross-sheet references)
            double numericValue = trimmedRange.toDouble(&ok);
            if (ok) {
                values.append(numericValue);
            }
        }
    }

    return values;
}

double CalculationEngine::calculatePercentile(const QList<double> &values, double percentile, const QString &rangeExpr)
{
    if (values.isEmpty()) {
        return 0.0;
    }

    // Parse method parameter from range expression
    QString method = "linear"; // default
    QStringList parts = rangeExpr.split(',');

    // Look for method parameter (second non-rounding parameter)
    for (const QString &part : parts) {
        QString trimmedPart = part.trimmed();

        // Skip decimal rounding parameters (.digits)
        if (QRegularExpression(R"(^\.(\d+)$)").match(trimmedPart).hasMatch()) {
            continue;
        }
        // Skip numeric line references
        bool isNumeric;
        trimmedPart.toInt(&isNumeric);
        if (isNumeric) {
            continue;
        }
        // Skip range expressions like "4-10"
        if (trimmedPart.contains('-')) {
            continue;
        }
        // This should be the method parameter
        if (trimmedPart.toLower() == "nearest" || trimmedPart.toLower() == "linear") {
            method = trimmedPart.toLower();
            break;
        }
    }

    QList<double> sortedValues = values;
    std::sort(sortedValues.begin(), sortedValues.end());

    if (method == "nearest") {
        // Nearest-rank method: return actual data value
        int index = static_cast<int>(std::ceil(percentile * sortedValues.size())) - 1;
        int maxIndex = static_cast<int>(sortedValues.size() - 1);
        index = std::max(0, std::min(index, maxIndex));
        return sortedValues[index];
    } else {
        // Linear interpolation method (default)
        double index = percentile * (sortedValues.size() - 1);
        int lowerIndex = static_cast<int>(std::floor(index));
        int upperIndex = static_cast<int>(std::ceil(index));

        if (lowerIndex == upperIndex) {
            return sortedValues[lowerIndex];
        } else {
            double weight = index - lowerIndex;
            return sortedValues[lowerIndex] * (1.0 - weight) + sortedValues[upperIndex] * weight;
        }
    }
}

QString CalculationEngine::handleUnitConversion(const QString &expr)
{
    UnitConversionResult result = m_unitConverter.convertExpression(expr);

    if (result.isValid) {
        // Format the result similar to the Python/Electron versions
        QString formattedValue = formatResult(result.value);
        return QString("%1 %2").arg(formattedValue).arg(result.unit);
    }

    // If unit conversion failed, return empty string to allow currency conversion to try
    // Only return error messages for actual unit conversion attempts with valid units
    return QString(); // Empty string indicates no unit conversion
}

QString CalculationEngine::handleTimecodeFunction(const QString &expr)
{
    // TIMECODE FUNCTION SYNTAX NOTE:
    // CORRECT:   TC(24, 00:00:10:00 + 00:00:05:00)
    // INCORRECT: TC(24, "00:00:10:00" + "00:00:05:00")
    // Do NOT use quotes around timecode values!

    // Pattern to match TC function calls like "TC(24, 100)" or "TC(30, 00:01:00:00)"
    QRegularExpression tcPattern(R"(^TC\s*\(\s*([^,]+)\s*,\s*(.+)\s*\)$)", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match = tcPattern.match(expr.trimmed());

    if (match.hasMatch()) {
        QString fpsStr = match.captured(1).trimmed();
        QString timecodeExpr = match.captured(2).trimmed();

        // Remove quotes from timecode expression if present
        if ((timecodeExpr.startsWith('"') && timecodeExpr.endsWith('"')) ||
            (timecodeExpr.startsWith('\'') && timecodeExpr.endsWith('\''))) {
            timecodeExpr = timecodeExpr.mid(1, timecodeExpr.length() - 2);
        }

        bool ok;
        double fps = fpsStr.toDouble(&ok);
        if (!ok) {
            LOG_DEBUG(QString("Invalid framerate in TC function: %1").arg(fpsStr));
            return QString("Error: Invalid framerate '%1'").arg(fpsStr);
        }

        TimecodeResult result = m_timecodeCalculator.TC(fps, timecodeExpr);
        if (result.isValid) {
            return result.value;
        } else {
            return result.errorMessage;
        }
    }

    return QString(); // Empty string indicates no TC function
}

QString CalculationEngine::handleAspectRatioFunction(const QString &expr)
{
    // Pattern to match AR function calls like "AR(1920x1080, ?x2000)" or "AR('1920x1080', '?x2000')"
    QRegularExpression arPattern(R"(^AR\s*\(\s*([^,]+)\s*,\s*([^)]+)\s*\)$)", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match = arPattern.match(expr.trimmed());

    if (match.hasMatch()) {
        QString originalDims = match.captured(1).trimmed();
        QString targetDims = match.captured(2).trimmed();

        // Remove quotes from dimension strings if present
        auto removeQuotes = [](QString &str) {
            if ((str.startsWith('"') && str.endsWith('"')) ||
                (str.startsWith('\'') && str.endsWith('\''))) {
                str = str.mid(1, str.length() - 2);
            }
        };

        removeQuotes(originalDims);
        removeQuotes(targetDims);

        AspectRatioResult result = m_aspectRatioCalculator.AR(originalDims, targetDims);
        if (result.isValid) {
            return result.dimensions;
        } else {
            return result.errorMessage;
        }
    }

    return QString(); // Empty string indicates no AR function
}

QString CalculationEngine::handleDateFunction(const QString &expr)
{
    // Pattern to match D function calls like "D(July 4, 2023 + 30)" or "D(July 4, 2023 W+ 5)"
    QRegularExpression datePattern(R"(^D\s*\(\s*(.+)\s*\)$)", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match = datePattern.match(expr.trimmed());

    LOG_DEBUG(QString("Checking if '%1' is a D function").arg(expr));

    if (match.hasMatch()) {
        QString dateExpr = match.captured(1).trimmed();
        LOG_DEBUG(QString("D function detected with expression: '%1'").arg(dateExpr));

        // Remove quotes from date expression if present
        if ((dateExpr.startsWith('"') && dateExpr.endsWith('"')) ||
            (dateExpr.startsWith('\'') && dateExpr.endsWith('\''))) {
            dateExpr = dateExpr.mid(1, dateExpr.length() - 2);
            LOG_DEBUG(QString("Removed quotes, expression is now: '%1'").arg(dateExpr));
        }

        DateResult result = m_dateCalculator.D(dateExpr);
        if (result.isValid) {
            LOG_DEBUG(QString("D function successful: '%1' -> '%2'").arg(dateExpr, result.value));
            return result.value;
        } else {
            LOG_DEBUG(QString("D function failed: '%1' -> '%2'").arg(dateExpr, result.errorMessage));
            return result.errorMessage;
        }
    }

    return QString(); // Empty string indicates no D function
}

QString CalculationEngine::handleCurrencyConversion(const QString &expr)
{
    // Check if expression matches currency conversion pattern
    // Pattern: "amount currency to currency" (e.g., "100 dollars to euros")
    // Use a broader pattern to catch invalid amounts and provide better error messages
    QRegularExpression currencyPattern(R"(\s+to\s+)", QRegularExpression::CaseInsensitiveOption);

    LOG_DEBUG(QString("Checking if '%1' is a currency conversion").arg(expr));

    // Quick check if it looks like a currency conversion (contains " to ")
    if (currencyPattern.match(expr.trimmed()).hasMatch()) {
        LOG_DEBUG(QString("Expression contains 'to' - attempting currency conversion"));

        CurrencyResult result = m_currencyConverter.convertExpression(expr);
        if (result.isValid) {
            LOG_DEBUG(QString("Currency conversion successful: %1").arg(result.value));
            return result.value;
        } else {
            LOG_DEBUG(QString("Currency conversion failed: %1").arg(result.errorMessage));
            return QString("Error: %1").arg(result.errorMessage);
        }
    }

    return QString(); // Empty string indicates no currency conversion
}

QString CalculationEngine::handlePercentageCalculation(const QString &expr)
{
    LOG_DEBUG(QString("Checking if '%1' is a percent() function call").arg(expr));

    // Check if this looks like a percent() function call
    if (!expr.trimmed().toLower().startsWith("percent(")) {
        LOG_DEBUG(QString("Not a percent() function call: %1").arg(expr));
        return QString(); // Empty string indicates no percentage calculation
    }

    // First, substitute any LN references in the expression
    QString processedExpr = processLNReferences(expr);

    // Check if the processed expression contains an error message
    if (processedExpr.startsWith("Error:")) {
        LOG_DEBUG(QString("Expression contains error after LN processing: %1").arg(processedExpr));
        return processedExpr; // Return error message directly
    }

    // Pass to percentage calculator
    PercentageResult result = m_percentageCalculator.calculateExpression(processedExpr);

    if (result.isValid) {
        QString formattedResult = m_percentageCalculator.formatResult(result);
        LOG_DEBUG(QString("Percentage calculation successful: %1").arg(formattedResult));
        return formattedResult;
    } else {
        LOG_DEBUG(QString("Percentage calculation failed: %1").arg(result.errorMessage));
        // Return error for failed percent() function calls since they were explicitly requested
        return QString("Error: %1").arg(result.errorMessage);
    }
}

void CalculationEngine::setSheetLookupFunction(std::function<WorksheetWidget*(const QString&)> lookupFunction)
{
    m_sheetLookupFunction = lookupFunction;
}

void CalculationEngine::setCurrentSheetName(const QString &sheetName)
{
    m_currentSheetName = sheetName;
}

QString CalculationEngine::getCrossSheetValue(const QString &sheetName, int lineNumber) const
{
    LOG_DEBUG(QString("getCrossSheetValue called: sheet='%1', line=%2").arg(sheetName).arg(lineNumber));

    if (!m_sheetLookupFunction) {
        LOG_ERROR("Cross-sheet lookup function not available");
        return QString("Error: Cross-sheet lookup not available");
    }

    // Get the worksheet widget for the specified sheet (case-insensitive)
    WorksheetWidget *targetSheet = m_sheetLookupFunction(sheetName);
    if (!targetSheet) {
        LOG_WARNING(QString("Sheet '%1' not found").arg(sheetName));
        return QString("Error: Sheet '%1' not found").arg(sheetName);
    }

    LOG_DEBUG(QString("Found target sheet '%1'").arg(sheetName));

    // Check if the line exists and has a calculated value
    if (!targetSheet->hasLineValue(lineNumber)) {
        // Check if the target line exists but just hasn't been evaluated yet
        QString content = targetSheet->getContent();
        QStringList lines = content.split('\n');

        if (lineNumber >= 1 && lineNumber <= lines.size()) {
            QString targetLine = lines[lineNumber - 1].trimmed();
            if (!targetLine.isEmpty() && !targetLine.startsWith(":::")) {
                // Line exists but hasn't been evaluated - likely due to circular reference
                LOG_WARNING(QString("LN%1 on sheet '%2' exists but not evaluated - possible circular reference").arg(lineNumber).arg(sheetName));
                return QString("Error: LN%1 on \"%2\" not evaluated (circular reference?)").arg(lineNumber).arg(sheetName);
            }
        }

        LOG_WARNING(QString("LN%1 not found on sheet '%2'").arg(lineNumber).arg(sheetName));
        return QString("Error: LN%1 not found on \"%2\"").arg(lineNumber).arg(sheetName);
    }

    // Get the line value from the target sheet
    double value = targetSheet->getLineValue(lineNumber);

    LOG_DEBUG(QString("Retrieved value %1 for line %2 from sheet '%3'").arg(value).arg(lineNumber).arg(sheetName));

    return QString::number(value, 'g', 15);
}

double CalculationEngine::extractNumericValueFromResult(const QString &result)
{
    if (result.isEmpty()) {
        return 0.0;
    }

    // Split the result by spaces and try to parse the first part as a number
    QStringList parts = result.split(' ', Qt::SkipEmptyParts);
    if (!parts.isEmpty()) {
        bool ok;
        double numericValue = parts[0].toDouble(&ok);
        if (ok) {
            LOG_DEBUG(QString("Extracted numeric value %1 from result '%2'").arg(numericValue).arg(result));
            return numericValue;
        }
    }

    // If we can't extract a number, try to find the first number in the string using regex
    QRegularExpression numberRegex(R"([-+]?\d*\.?\d+)");
    QRegularExpressionMatch match = numberRegex.match(result);
    if (match.hasMatch()) {
        bool ok;
        double numericValue = match.captured(0).toDouble(&ok);
        if (ok) {
            LOG_DEBUG(QString("Extracted numeric value %1 from result '%2' using regex").arg(numericValue).arg(result));
            return numericValue;
        }
    }

    LOG_DEBUG(QString("Could not extract numeric value from result '%1', returning 0.0").arg(result));
    return 0.0;
}


