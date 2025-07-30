#include "DependencyTracker.h"
#include "WorksheetWidget.h"
#include "Logger.h"
#include <QDebug>
#include <algorithm>

// Static regex patterns compiled once for performance
const QRegularExpression DependencyTracker::s_lnPattern(
    R"(\bLN(\d+)\b)",
    QRegularExpression::CaseInsensitiveOption
);

const QRegularExpression DependencyTracker::s_statFunctionPattern(
    R"(^(sum|mean|median|mode|min|max|count|product|variance|stdev|std|range|geomean|harmmean|sumsq)\s*\(\s*(.*?)\s*\)$)",
    QRegularExpression::CaseInsensitiveOption
);

const QRegularExpression DependencyTracker::s_rangePattern(
    R"(^(\d+)-(\d+)$|^(\d+(?:\s*,\s*\d+)*)$|^(above|below)$)",
    QRegularExpression::CaseInsensitiveOption
);

const QRegularExpression DependencyTracker::s_crossSheetPattern(
    R"(\bS\.([^.]+)\.LN(\d+)\b)",
    QRegularExpression::CaseInsensitiveOption
);

DependencyTracker::DependencyTracker()
{
    // Reserve space for typical worksheet sizes to avoid rehashing
    m_forwardDeps.reserve(100);
    m_reverseDeps.reserve(100);
    m_crossSheetRefs.reserve(50);
    m_reverseCrossSheetRefs.reserve(50);

    LOG_DEBUG("DependencyTracker initialized with optimized containers including cross-sheet support");
}

void DependencyTracker::updateLineDependencies(int lineNumber, const QString &expression, int totalLines)
{
    // Remove existing dependencies for this line
    if (m_forwardDeps.contains(lineNumber)) {
        const QSet<int> &oldDeps = m_forwardDeps[lineNumber];
        for (int dep : oldDeps) {
            removeDependency(lineNumber, dep);
        }
    }
    
    // Parse new dependencies
    QSet<int> newDeps = parseExpressionDependencies(expression, lineNumber, totalLines);
    
    // Add new dependencies
    if (!newDeps.isEmpty()) {
        m_forwardDeps[lineNumber] = std::move(newDeps);  // Move semantics for efficiency
        
        for (int dep : m_forwardDeps[lineNumber]) {
            addDependency(lineNumber, dep);
        }
    } else {
        m_forwardDeps.remove(lineNumber);
    }
}

QSet<int> DependencyTracker::getLinesToRecalculate(const QSet<int> &changedLines, int totalLines) const
{
    QSet<int> result;
    QSet<int> visited;
    
    // Reserve space for efficiency
    result.reserve(changedLines.size() * 2);  // Estimate: each change affects ~2 lines on average
    visited.reserve(totalLines);
    
    // Start DFS from changed lines
    dfsCollectDependents(changedLines, visited, result);
    
    // Always include the originally changed lines
    result.unite(changedLines);
    
    LOG_DEBUG(QString("Dependency tracking: %1 changed lines -> %2 lines to recalculate")
              .arg(changedLines.size()).arg(result.size()));
    
    return result;
}

QList<int> DependencyTracker::getEvaluationOrder(const QSet<int> &linesToEvaluate) const
{
    QList<int> result;
    QSet<int> visited;
    QSet<int> recursionStack;

    // Reserve space for efficiency
    result.reserve(linesToEvaluate.size());

    // Perform topological sort using DFS
    for (int line : linesToEvaluate) {
        if (!visited.contains(line)) {
            topologicalSortDFS(line, linesToEvaluate, visited, recursionStack, result);
        }
    }

    // Add any remaining lines that weren't reached by DFS (independent lines)
    for (int line : linesToEvaluate) {
        if (!result.contains(line)) {
            result.append(line);
        }
    }

    return result;
}

void DependencyTracker::clear()
{
    m_forwardDeps.clear();
    m_reverseDeps.clear();
    m_crossSheetRefs.clear();
    m_reverseCrossSheetRefs.clear();

    // Re-reserve space for next use
    m_forwardDeps.reserve(100);
    m_reverseDeps.reserve(100);
    m_crossSheetRefs.reserve(50);
    m_reverseCrossSheetRefs.reserve(50);

    LOG_DEBUG("DependencyTracker cleared and re-optimized with cross-sheet support");
}

void DependencyTracker::removeLine(int lineNumber)
{
    // Remove all dependencies involving this line
    if (m_forwardDeps.contains(lineNumber)) {
        const QSet<int> &deps = m_forwardDeps[lineNumber];
        for (int dep : deps) {
            removeDependency(lineNumber, dep);
        }
        m_forwardDeps.remove(lineNumber);
    }

    if (m_reverseDeps.contains(lineNumber)) {
        const QSet<int> &reverseDeps = m_reverseDeps[lineNumber];
        for (int dep : reverseDeps) {
            if (m_forwardDeps.contains(dep)) {
                m_forwardDeps[dep].remove(lineNumber);
                if (m_forwardDeps[dep].isEmpty()) {
                    m_forwardDeps.remove(dep);
                }
            }
        }
        m_reverseDeps.remove(lineNumber);
    }

    // Remove cross-sheet references from this line
    if (m_crossSheetRefs.contains(lineNumber)) {
        const QSet<CrossSheetReference> &refs = m_crossSheetRefs[lineNumber];
        for (const CrossSheetReference &ref : refs) {
            QPair<QString, int> key(ref.sheetName, ref.lineNumber);
            if (m_reverseCrossSheetRefs.contains(key)) {
                m_reverseCrossSheetRefs[key].remove(lineNumber);
                if (m_reverseCrossSheetRefs[key].isEmpty()) {
                    m_reverseCrossSheetRefs.remove(key);
                }
            }
        }
        m_crossSheetRefs.remove(lineNumber);
    }
}

QSet<int> DependencyTracker::getDirectDependencies(int lineNumber) const
{
    return m_forwardDeps.value(lineNumber, QSet<int>());
}

QSet<int> DependencyTracker::getReverseDependencies(int lineNumber) const
{
    return m_reverseDeps.value(lineNumber, QSet<int>());
}

bool DependencyTracker::hasCircularDependencies() const
{
    // Use DFS to detect cycles
    QSet<int> visited;
    QSet<int> recursionStack;
    
    for (auto it = m_forwardDeps.constBegin(); it != m_forwardDeps.constEnd(); ++it) {
        int line = it.key();
        if (!visited.contains(line)) {
            if (hasCycleDFS(line, visited, recursionStack)) {
                return true;
            }
        }
    }
    
    return false;
}

DependencyTracker::Stats DependencyTracker::getStats() const
{
    Stats stats;
    stats.totalDependencies = 0;
    stats.maxDependenciesPerLine = 0;
    stats.linesWithDependencies = m_forwardDeps.size();
    
    for (auto it = m_forwardDeps.constBegin(); it != m_forwardDeps.constEnd(); ++it) {
        int depCount = it.value().size();
        stats.totalDependencies += depCount;
        stats.maxDependenciesPerLine = std::max(stats.maxDependenciesPerLine, depCount);
    }
    
    return stats;
}

QSet<int> DependencyTracker::parseExpressionDependencies(const QString &expression, int lineNumber, int totalLines) const
{
    QSet<int> dependencies;
    dependencies.reserve(10);  // Reserve space for typical dependency count
    
    QString trimmed = expression.trimmed();
    if (trimmed.isEmpty()) {
        return dependencies;
    }
    
    // Parse LN references
    QRegularExpressionMatchIterator lnIterator = s_lnPattern.globalMatch(trimmed);
    while (lnIterator.hasNext()) {
        QRegularExpressionMatch match = lnIterator.next();
        bool ok;
        int referencedLine = match.captured(1).toInt(&ok);
        if (ok && referencedLine != lineNumber) {  // Avoid self-reference
            dependencies.insert(referencedLine);
        }
    }
    
    // Parse statistical functions
    QRegularExpressionMatch statMatch = s_statFunctionPattern.match(trimmed);
    if (statMatch.hasMatch()) {
        QString rangeExpr = statMatch.captured(2);
        QSet<int> rangeDeps = parseRange(rangeExpr, lineNumber, totalLines);
        dependencies.unite(rangeDeps);
    }
    
    return dependencies;
}

QSet<int> DependencyTracker::parseRange(const QString &rangeExpr, int currentLine, int totalLines) const
{
    QSet<int> result;
    QString trimmed = rangeExpr.trimmed();

    if (trimmed.isEmpty()) {
        // Empty range means "above"
        for (int i = 1; i < currentLine; ++i) {
            result.insert(i);
        }
        return result;
    }

    // Handle special keywords
    if (trimmed.toLower() == "above") {
        for (int i = 1; i < currentLine; ++i) {
            result.insert(i);
        }
        return result;
    }

    if (trimmed.toLower() == "below") {
        for (int i = currentLine + 1; i <= totalLines; ++i) {
            result.insert(i);
        }
        return result;
    }

    // Handle dash ranges (e.g., "1-5")
    if (trimmed.contains('-') && !trimmed.startsWith('-')) {
        QStringList parts = trimmed.split('-');
        if (parts.size() == 2) {
            bool ok1, ok2;
            int start = parts[0].trimmed().toInt(&ok1);
            int end = parts[1].trimmed().toInt(&ok2);
            if (ok1 && ok2 && start <= end) {
                for (int i = start; i <= end; ++i) {
                    if (i != currentLine) {  // Avoid self-reference
                        result.insert(i);
                    }
                }
            }
        }
        return result;
    }

    // Handle comma-separated ranges (e.g., "1,3,5")
    if (trimmed.contains(',')) {
        QStringList parts = trimmed.split(',');
        for (const QString &part : parts) {
            bool ok;
            int lineNum = part.trimmed().toInt(&ok);
            if (ok && lineNum != currentLine) {  // Avoid self-reference
                result.insert(lineNum);
            }
        }
        return result;
    }

    // Handle single line number
    bool ok;
    int lineNum = trimmed.toInt(&ok);
    if (ok && lineNum != currentLine) {  // Avoid self-reference
        result.insert(lineNum);
    }

    return result;
}

inline void DependencyTracker::addDependency(int fromLine, int toLine)
{
    // Add to reverse dependencies
    m_reverseDeps[toLine].insert(fromLine);
}

inline void DependencyTracker::removeDependency(int fromLine, int toLine)
{
    // Remove from reverse dependencies
    if (m_reverseDeps.contains(toLine)) {
        m_reverseDeps[toLine].remove(fromLine);
        if (m_reverseDeps[toLine].isEmpty()) {
            m_reverseDeps.remove(toLine);
        }
    }
}

void DependencyTracker::dfsCollectDependents(const QSet<int> &startLines,
                                           QSet<int> &visited,
                                           QSet<int> &result) const
{
    for (int line : startLines) {
        if (visited.contains(line)) {
            continue;
        }

        visited.insert(line);
        result.insert(line);

        // Get lines that depend on this line
        if (m_reverseDeps.contains(line)) {
            const QSet<int> &dependents = m_reverseDeps[line];
            dfsCollectDependents(dependents, visited, result);
        }
    }
}

bool DependencyTracker::hasCycleDFS(int line, QSet<int> &visited, QSet<int> &recursionStack) const
{
    visited.insert(line);
    recursionStack.insert(line);

    if (m_forwardDeps.contains(line)) {
        const QSet<int> &deps = m_forwardDeps[line];
        for (int dep : deps) {
            if (!visited.contains(dep)) {
                if (hasCycleDFS(dep, visited, recursionStack)) {
                    return true;
                }
            } else if (recursionStack.contains(dep)) {
                return true;  // Cycle detected
            }
        }
    }

    recursionStack.remove(line);
    return false;
}

void DependencyTracker::topologicalSortDFS(int line,
                                          const QSet<int> &linesToEvaluate,
                                          QSet<int> &visited,
                                          QSet<int> &recursionStack,
                                          QList<int> &result) const
{
    // Skip if not in our evaluation set
    if (!linesToEvaluate.contains(line)) {
        return;
    }

    // Skip if already visited
    if (visited.contains(line)) {
        return;
    }

    // Mark as visited and add to recursion stack
    visited.insert(line);
    recursionStack.insert(line);

    // Visit all dependencies first (lines this line depends on)
    if (m_forwardDeps.contains(line)) {
        const QSet<int> &dependencies = m_forwardDeps[line];
        for (int dependency : dependencies) {
            if (linesToEvaluate.contains(dependency)) {
                // Only process if it's in our evaluation set
                if (!visited.contains(dependency)) {
                    topologicalSortDFS(dependency, linesToEvaluate, visited, recursionStack, result);
                }
                // Note: We ignore cycles for now - they'll be handled gracefully by the evaluation
            }
        }
    }

    // Remove from recursion stack and add to result
    recursionStack.remove(line);
    result.append(line);
}

void DependencyTracker::updateCrossSheetDependencies(int lineNumber, const QString &expression, const QString &currentSheetName)
{
    // Remove existing cross-sheet dependencies for this line
    if (m_crossSheetRefs.contains(lineNumber)) {
        const QSet<CrossSheetReference> &oldRefs = m_crossSheetRefs[lineNumber];
        for (const CrossSheetReference &ref : oldRefs) {
            QPair<QString, int> key(ref.sheetName, ref.lineNumber);
            if (m_reverseCrossSheetRefs.contains(key)) {
                m_reverseCrossSheetRefs[key].remove(lineNumber);
                if (m_reverseCrossSheetRefs[key].isEmpty()) {
                    m_reverseCrossSheetRefs.remove(key);
                }
            }
        }
        m_crossSheetRefs.remove(lineNumber);
    }

    // Parse new cross-sheet references
    QSet<CrossSheetReference> newRefs = parseCrossSheetReferences(expression);

    // Add new cross-sheet dependencies
    if (!newRefs.isEmpty()) {
        m_crossSheetRefs[lineNumber] = std::move(newRefs);

        for (const CrossSheetReference &ref : m_crossSheetRefs[lineNumber]) {
            QPair<QString, int> key(ref.sheetName, ref.lineNumber);
            m_reverseCrossSheetRefs[key].insert(lineNumber);
        }
    }

    LOG_DEBUG(QString("Updated cross-sheet dependencies for line %1 in sheet '%2': %3 references")
              .arg(lineNumber).arg(currentSheetName).arg(newRefs.size()));
}

QSet<DependencyTracker::CrossSheetReference> DependencyTracker::getCrossSheetReferences(int lineNumber) const
{
    return m_crossSheetRefs.value(lineNumber, QSet<CrossSheetReference>());
}

QSet<int> DependencyTracker::getLinesThatReference(const QString &sheetName, int lineNumber) const
{
    QPair<QString, int> key(sheetName.toLower(), lineNumber);
    return m_reverseCrossSheetRefs.value(key, QSet<int>());
}

bool DependencyTracker::hasCrossSheetReferences() const
{
    return !m_crossSheetRefs.isEmpty();
}

QSet<DependencyTracker::CrossSheetReference> DependencyTracker::parseCrossSheetReferences(const QString &expression) const
{
    QSet<CrossSheetReference> references;
    references.reserve(5);  // Reserve space for typical cross-sheet reference count

    QString trimmed = expression.trimmed();
    if (trimmed.isEmpty()) {
        return references;
    }

    // Parse cross-sheet references using the compiled regex
    QRegularExpressionMatchIterator iterator = s_crossSheetPattern.globalMatch(trimmed);
    while (iterator.hasNext()) {
        QRegularExpressionMatch match = iterator.next();
        QString sheetName = match.captured(1).trimmed();
        bool ok;
        int lineNumber = match.captured(2).toInt(&ok);

        if (ok && !sheetName.isEmpty()) {
            references.insert(CrossSheetReference(sheetName, lineNumber));
            LOG_DEBUG(QString("Found cross-sheet reference: S.%1.LN%2").arg(sheetName).arg(lineNumber));
        }
    }

    return references;
}

bool DependencyTracker::hasCrossSheetCircularDependencies(const QString &currentSheetName,
                                                         std::function<class WorksheetWidget*(const QString&)> sheetLookupFunction) const
{
    if (!sheetLookupFunction) {
        return false;
    }

    // Use a set to track visited sheets to detect cycles
    QSet<QString> visitedSheets;
    QSet<QString> currentPath;

    return detectCrossSheetCycle(currentSheetName, sheetLookupFunction, visitedSheets, currentPath);
}

bool DependencyTracker::detectCrossSheetCycle(const QString &sheetName,
                                             std::function<class WorksheetWidget*(const QString&)> sheetLookupFunction,
                                             QSet<QString> &visitedSheets,
                                             QSet<QString> &currentPath) const
{
    // If we've already visited this sheet in the current path, we have a cycle
    if (currentPath.contains(sheetName)) {
        LOG_WARNING(QString("Cross-sheet circular dependency detected: %1 -> %2")
                   .arg(QStringList(currentPath.values()).join(" -> ")).arg(sheetName));
        return true;
    }

    // If we've already fully processed this sheet, no cycle from here
    if (visitedSheets.contains(sheetName)) {
        return false;
    }

    // Add to current path
    currentPath.insert(sheetName);

    // Get the worksheet for this sheet
    WorksheetWidget *worksheet = sheetLookupFunction(sheetName);
    if (!worksheet) {
        currentPath.remove(sheetName);
        return false;
    }

    // Get all cross-sheet references from this worksheet
    QSet<QString> referencedSheets = getCrossSheetReferencesFromWorksheet(worksheet);

    // Check each referenced sheet for cycles
    for (const QString &referencedSheet : referencedSheets) {
        if (detectCrossSheetCycle(referencedSheet, sheetLookupFunction, visitedSheets, currentPath)) {
            return true; // Cycle detected
        }
    }

    // Remove from current path and mark as visited
    currentPath.remove(sheetName);
    visitedSheets.insert(sheetName);

    return false;
}

QSet<QString> DependencyTracker::getCrossSheetReferencesFromWorksheet(class WorksheetWidget *worksheet) const
{
    QSet<QString> referencedSheets;

    if (!worksheet) {
        return referencedSheets;
    }

    // Get the content of the worksheet
    QString content = worksheet->getContent();
    QStringList lines = content.split('\n');

    // Parse each line for cross-sheet references
    for (const QString &line : lines) {
        QSet<CrossSheetReference> refs = parseCrossSheetReferences(line);
        for (const CrossSheetReference &ref : refs) {
            referencedSheets.insert(ref.sheetName.toLower());
        }
    }

    return referencedSheets;
}
