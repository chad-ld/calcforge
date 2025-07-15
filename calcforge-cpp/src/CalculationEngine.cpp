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
    m_functions["sin"] = [](double x) { return std::sin(x); };
    m_functions["cos"] = [](double x) { return std::cos(x); };
    m_functions["tan"] = [](double x) { return std::tan(x); };
    m_functions["sqrt"] = [](double x) { return std::sqrt(x); };
    m_functions["abs"] = [](double x) { return std::abs(x); };
    m_functions["log"] = [](double x) { return std::log(x); };
    m_functions["log10"] = [](double x) { return std::log10(x); };
    m_functions["exp"] = [](double x) { return std::exp(x); };
    m_functions["floor"] = [](double x) { return std::floor(x); };
    m_functions["ceil"] = [](double x) { return std::ceil(x); };
    // Note: round is handled separately as a multi-argument function
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
            // For date functions, store 0 for LN references (dates are strings)
            m_lineValues[lineNumber] = 0.0;
            return dateResult;
        }

        // Check for unit conversion FIRST (more specific pattern)
        QString unitResult = handleUnitConversion(expr);
        if (!unitResult.isEmpty()) {
            // For unit conversions, we need to extract the numeric value for LN references
            // The result format is "value unit", so we extract the numeric part
            QStringList parts = unitResult.split(' ');
            if (!parts.isEmpty()) {
                bool ok;
                double numericValue = parts[0].toDouble(&ok);
                if (ok) {
                    m_lineValues[lineNumber] = numericValue;
                }
            }
            return unitResult;
        }

        // Check for currency conversion AFTER unit conversion (broader pattern)
        QString currencyResult = handleCurrencyConversion(expr);
        if (!currencyResult.isEmpty()) {
            // For currency conversions, store 0 for LN references (currency amounts are strings)
            m_lineValues[lineNumber] = 0.0;
            return currencyResult;
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
    QRegularExpression statPattern(R"(^(sum|mean|median|mode|min|max|count|product|variance|stdev|std|range|geomean|harmmean|sumsq)\s*\(\s*(.*?)\s*\)$)",
                                   QRegularExpression::CaseInsensitiveOption);

    QRegularExpressionMatch match = statPattern.match(expr.trimmed());
    if (!match.hasMatch()) {
        return std::numeric_limits<double>::quiet_NaN(); // Not a statistical function
    }

    QString funcName = match.captured(1).toLower();
    QString rangeExpr = match.captured(2);

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

    // Handle range formats
    if (trimmedRange.contains('-') && !trimmedRange.startsWith('-')) {
        // Range format like "1-5"
        QStringList parts = trimmedRange.split('-');
        if (parts.size() == 2) {
            bool ok1, ok2;
            int start = parts[0].trimmed().toInt(&ok1);
            int end = parts[1].trimmed().toInt(&ok2);
            if (ok1 && ok2) {
                for (int i = start; i <= end; ++i) {
                    if (m_lineValues.contains(i)) { // Only include lines that have been evaluated
                        values.append(getLineValue(i));
                    }
                }
            }
        }
    }
    else if (trimmedRange.contains(',')) {
        // Comma-separated format like "1,3,5" or "1,3,5,R2"
        QStringList parts = trimmedRange.split(',');
        for (const QString &part : parts) {
            QString trimmedPart = part.trimmed();

            // Skip decimal rounding parameters (.digits) - they're handled elsewhere
            if (QRegularExpression(R"(^\.(\d+)$)").match(trimmedPart).hasMatch()) {
                continue;
            }

            // Try to parse as a line number first
            bool ok;
            int lineNum = trimmedPart.toInt(&ok);
            if (ok && m_lineValues.contains(lineNum)) { // Only include lines that have been evaluated
                values.append(getLineValue(lineNum));
            } else {
                // Try to parse as a direct numeric value (for processed cross-sheet references)
                double numericValue = trimmedPart.toDouble(&ok);
                if (ok) {
                    values.append(numericValue);
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


