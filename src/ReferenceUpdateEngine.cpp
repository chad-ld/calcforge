#include "ReferenceUpdateEngine.h"
#include "Logger.h"
#include <QRegularExpressionMatchIterator>
#include <algorithm>

ReferenceUpdateEngine::ReferenceUpdateEngine()
    : m_lnRegex(R"(\bLN(\d+)\b)", QRegularExpression::CaseInsensitiveOption)
    , m_statisticalRegex(R"(\b(sum|mean|median|mode|min|max|count|product|variance|stdev|std|range|geomean|harmmean|sumsq|perc5|perc95|meanfps)\s*\(\s*([^)]+)\s*\))", QRegularExpression::CaseInsensitiveOption)
    , m_rangeRegex(R"(\b(\d+)\s*-\s*(\d+)\b)")
    , m_commaListRegex(R"(\b\d+\b)")
{
    // Initialize statistical function names - complete list matching CalculationEngine
    m_statisticalFunctions << "sum" << "mean" << "median" << "mode" << "min" << "max"
                          << "count" << "product" << "variance" << "stdev" << "std"
                          << "range" << "geomean" << "harmmean" << "sumsq"
                          << "perc5" << "perc95" << "meanfps";
}

bool ReferenceUpdateEngine::updateReferences(QStringList &content, const QList<LineChange> &changes)
{
    if (changes.isEmpty()) {
        return false;
    }
    
    bool anyUpdated = false;
    
    // Update references in each line
    for (int i = 0; i < content.size(); ++i) {
        QString &line = content[i];
        if (updateExpressionReferences(line, changes)) {
            anyUpdated = true;
            LOG_DEBUG(QString("Updated references in line %1: %2").arg(i + 1).arg(line));
        }
    }
    
    return anyUpdated;
}

bool ReferenceUpdateEngine::updateExpressionReferences(QString &expression, const QList<LineChange> &changes)
{
    if (expression.trimmed().isEmpty() || changes.isEmpty()) {
        return false;
    }
    
    // Find all references in the expression
    QList<ExpressionReference> references = findReferences(expression);
    
    if (references.isEmpty()) {
        return false;
    }
    
    // Sort references by position (descending) to avoid position shifts during replacement
    std::sort(references.begin(), references.end(), 
              [](const ExpressionReference &a, const ExpressionReference &b) {
                  return a.startPos > b.startPos;
              });
    
    bool modified = false;
    
    // Update each reference
    for (const ExpressionReference &ref : references) {
        QString updatedText;
        
        switch (ref.type) {
        case ExpressionReference::LNReference:
            updatedText = updateLNReference(ref, changes);
            break;
        case ExpressionReference::StatisticalRange:
            updatedText = updateStatisticalRange(ref, changes);
            break;
        case ExpressionReference::RelativeRange:
            // Relative ranges (above, below) don't change
            continue;
        }
        
        if (updatedText != ref.originalText) {
            // Replace the reference in the expression
            expression.replace(ref.startPos, ref.length, updatedText);
            modified = true;
        }
    }
    
    return modified;
}

QList<ExpressionReference> ReferenceUpdateEngine::findReferences(const QString &expression)
{
    QList<ExpressionReference> references;
    
    // Find LN references
    references.append(parseLNReferences(expression));
    
    // Find statistical function ranges
    references.append(parseStatisticalRanges(expression));
    
    return references;
}

int ReferenceUpdateEngine::calculateNewLineNumber(int originalLine, const QList<LineChange> &changes)
{
    int newLineNumber = originalLine;
    
    // Apply changes in order
    for (const LineChange &change : changes) {
        if (change.type == LineChange::Insertion) {
            // If insertion happens at or before this line, shift line number up
            if (change.startLine <= originalLine) {
                newLineNumber += change.count;
            }
        } else if (change.type == LineChange::Deletion) {
            // If deletion affects this line
            if (change.startLine <= originalLine && originalLine < change.startLine + change.count) {
                // This line was deleted
                return -1;
            } else if (change.startLine <= originalLine) {
                // This line comes after the deletion, shift down
                newLineNumber -= change.count;
            }
        }
        // Modifications don't change line numbers
    }
    
    return newLineNumber;
}

bool ReferenceUpdateEngine::updateRange(int startLine, int endLine, const QList<LineChange> &changes, 
                                       int &newStart, int &newEnd)
{
    newStart = calculateNewLineNumber(startLine, changes);
    newEnd = calculateNewLineNumber(endLine, changes);
    
    // Check if range is still valid
    if (newStart == -1 || newEnd == -1 || newStart > newEnd) {
        return false;
    }
    
    return true;
}

QList<ExpressionReference> ReferenceUpdateEngine::parseLNReferences(const QString &expression)
{
    QList<ExpressionReference> references;
    
    QRegularExpressionMatchIterator iterator = m_lnRegex.globalMatch(expression);
    while (iterator.hasNext()) {
        QRegularExpressionMatch match = iterator.next();
        
        ExpressionReference ref;
        ref.type = ExpressionReference::LNReference;
        ref.startPos = match.capturedStart();
        ref.length = match.capturedLength();
        ref.originalText = match.captured(0);
        
        bool ok;
        int lineNumber = match.captured(1).toInt(&ok);
        if (ok) {
            ref.lineNumbers.append(lineNumber);
        }
        
        references.append(ref);
    }
    
    return references;
}

QList<ExpressionReference> ReferenceUpdateEngine::parseStatisticalRanges(const QString &expression)
{
    QList<ExpressionReference> references;
    
    QRegularExpressionMatchIterator iterator = m_statisticalRegex.globalMatch(expression);
    while (iterator.hasNext()) {
        QRegularExpressionMatch match = iterator.next();
        
        QString functionName = match.captured(1).toLower();
        QString arguments = match.captured(2);
        
        // Skip relative ranges (above, below)
        if (arguments.contains("above", Qt::CaseInsensitive) ||
            arguments.contains("below", Qt::CaseInsensitive)) {
            ExpressionReference ref;
            ref.type = ExpressionReference::RelativeRange;
            ref.startPos = match.capturedStart();
            ref.length = match.capturedLength();
            ref.originalText = match.captured(0);
            references.append(ref);
            continue;
        }
        
        ExpressionReference ref;
        ref.type = ExpressionReference::StatisticalRange;
        ref.startPos = match.capturedStart();
        ref.length = match.capturedLength();
        ref.originalText = match.captured(0);
        
        // Parse the arguments to extract line numbers
        if (arguments.contains('-')) {
            // Range format: "2-5"
            QPair<int, int> range = parseRange(arguments);
            if (range.first > 0 && range.second > 0) {
                for (int i = range.first; i <= range.second; ++i) {
                    ref.lineNumbers.append(i);
                }
            }
        } else {
            // Comma-separated format: "1,3,5"
            ref.lineNumbers = parseCommaSeparatedNumbers(arguments);
        }
        
        if (!ref.lineNumbers.isEmpty()) {
            references.append(ref);
        }
    }
    
    return references;
}

QString ReferenceUpdateEngine::updateLNReference(const ExpressionReference &ref, const QList<LineChange> &changes)
{
    if (ref.lineNumbers.isEmpty()) {
        return ref.originalText;
    }
    
    int originalLine = ref.lineNumbers.first();
    int newLine = calculateNewLineNumber(originalLine, changes);
    
    if (newLine == -1) {
        // Line was deleted - could return error or 0
        LOG_WARNING(QString("LN reference to deleted line %1").arg(originalLine));
        return "0";  // Return 0 for deleted line references
    }
    
    // Preserve case of original reference
    QString prefix = ref.originalText.left(2);  // "LN" or "ln"
    return prefix + QString::number(newLine);
}

QString ReferenceUpdateEngine::updateStatisticalRange(const ExpressionReference &ref, const QList<LineChange> &changes)
{
    if (ref.lineNumbers.isEmpty()) {
        return ref.originalText;
    }
    
    // Extract function name and update the arguments
    QRegularExpressionMatch match = m_statisticalRegex.match(ref.originalText);
    if (!match.hasMatch()) {
        return ref.originalText;
    }
    
    QString functionName = match.captured(1);
    QString arguments = match.captured(2);
    
    // Determine if it's a range or comma-separated list
    if (arguments.contains('-')) {
        // Range format: update "2-5" to new range
        QPair<int, int> originalRange = parseRange(arguments);
        if (originalRange.first > 0 && originalRange.second > 0) {
            int newStart, newEnd;
            if (updateRange(originalRange.first, originalRange.second, changes, newStart, newEnd)) {
                return QString("%1(%2-%3)").arg(functionName).arg(newStart).arg(newEnd);
            } else {
                LOG_WARNING(QString("Statistical range %1-%2 became invalid after line changes")
                           .arg(originalRange.first).arg(originalRange.second));
                return ref.originalText;  // Keep original if range becomes invalid
            }
        }
    } else {
        // Comma-separated format: update each number but preserve ROUND= parameters
        QStringList updatedParts;
        QStringList parts = arguments.split(',');

        for (const QString &part : parts) {
            QString trimmedPart = part.trimmed();

            // Check if this is a decimal rounding parameter (.X)
            if (QRegularExpression(R"(^\.(\d+)$)").match(trimmedPart).hasMatch()) {
                // Preserve decimal rounding parameters as-is
                updatedParts.append(trimmedPart);
            } else {
                // Try to parse as line number and update it
                bool ok;
                int originalLine = trimmedPart.toInt(&ok);
                if (ok) {
                    int newLine = calculateNewLineNumber(originalLine, changes);
                    if (newLine > 0) {
                        updatedParts.append(QString::number(newLine));
                    }
                } else {
                    // Keep non-numeric parts as-is (might be other special parameters)
                    updatedParts.append(trimmedPart);
                }
            }
        }

        if (!updatedParts.isEmpty()) {
            return QString("%1(%2)").arg(functionName).arg(updatedParts.join(","));
        } else {
            LOG_WARNING(QString("All lines in statistical function became invalid"));
            return ref.originalText;  // Keep original if all lines become invalid
        }
    }
    
    return ref.originalText;
}

QList<int> ReferenceUpdateEngine::parseCommaSeparatedNumbers(const QString &text)
{
    QList<int> numbers;
    
    QRegularExpressionMatchIterator iterator = m_commaListRegex.globalMatch(text);
    while (iterator.hasNext()) {
        QRegularExpressionMatch match = iterator.next();
        bool ok;
        int number = match.captured(0).toInt(&ok);
        if (ok && number > 0) {
            numbers.append(number);
        }
    }
    
    return numbers;
}

QPair<int, int> ReferenceUpdateEngine::parseRange(const QString &text)
{
    QRegularExpressionMatch match = m_rangeRegex.match(text);
    if (match.hasMatch()) {
        bool ok1, ok2;
        int start = match.captured(1).toInt(&ok1);
        int end = match.captured(2).toInt(&ok2);
        if (ok1 && ok2 && start > 0 && end > 0) {
            return QPair<int, int>(start, end);
        }
    }
    
    return QPair<int, int>(0, 0);
}
