#pragma once

#include <QHash>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QRegularExpression>
#include <QPair>
#include <memory>

/**
 * High-performance dependency tracking system optimized for C++
 * 
 * Leverages C++ advantages:
 * - Fast QHash/QSet containers with O(1) average lookup
 * - Efficient memory layout with move semantics
 * - Compile-time optimizations with inline functions
 * - STL algorithms for optimal performance
 * - Smart pointers for memory safety
 */
class DependencyTracker
{
public:
    /**
     * Represents a dependency relationship between lines
     */
    struct Dependency {
        int fromLine;     // Line that depends on another
        int toLine;       // Line being depended upon
        QString type;     // Type: "LN", "range", "above", "below", "cross_sheet"

        Dependency() = default;
        Dependency(int from, int to, const QString &t)
            : fromLine(from), toLine(to), type(t) {}

        // Efficient comparison for QSet
        bool operator==(const Dependency &other) const {
            return fromLine == other.fromLine &&
                   toLine == other.toLine &&
                   type == other.type;
        }
    };

    /**
     * Represents a cross-sheet reference (e.g., S.SheetName.LN5)
     */
    struct CrossSheetReference {
        QString sheetName;    // Name of referenced sheet (case-insensitive)
        int lineNumber;       // Line number in referenced sheet

        CrossSheetReference() = default;
        CrossSheetReference(const QString &sheet, int line)
            : sheetName(sheet.toLower()), lineNumber(line) {}

        // Efficient comparison for QSet and QHash
        bool operator==(const CrossSheetReference &other) const {
            return sheetName.compare(other.sheetName, Qt::CaseInsensitive) == 0 &&
                   lineNumber == other.lineNumber;
        }
    };
    
    DependencyTracker();
    ~DependencyTracker() = default;
    
    // Move semantics for performance
    DependencyTracker(DependencyTracker &&other) noexcept = default;
    DependencyTracker &operator=(DependencyTracker &&other) noexcept = default;
    
    // Disable copy to prevent accidental expensive copies
    DependencyTracker(const DependencyTracker &) = delete;
    DependencyTracker &operator=(const DependencyTracker &) = delete;
    
    /**
     * Update dependencies for a specific line
     * @param lineNumber Line number being updated
     * @param expression New expression content
     * @param totalLines Total number of lines in worksheet (for range calculations)
     */
    void updateLineDependencies(int lineNumber, const QString &expression, int totalLines = 1000);

    /**
     * Update cross-sheet dependencies for a specific line
     * @param lineNumber Line number being updated
     * @param expression New expression content
     * @param currentSheetName Name of the current sheet
     */
    void updateCrossSheetDependencies(int lineNumber, const QString &expression, const QString &currentSheetName);
    
    /**
     * Get all lines that need recalculation when given lines change
     * @param changedLines Set of line numbers that changed
     * @param totalLines Total number of lines in worksheet
     * @return Set of all lines that need recalculation (including changed lines)
     */
    QSet<int> getLinesToRecalculate(const QSet<int> &changedLines, int totalLines) const;

    /**
     * Get lines in dependency-safe evaluation order
     * @param linesToEvaluate Set of line numbers to evaluate
     * @return List of line numbers in dependency order (prerequisites first)
     */
    QList<int> getEvaluationOrder(const QSet<int> &linesToEvaluate) const;
    
    /**
     * Clear all dependencies (for new worksheets)
     */
    void clear();
    
    /**
     * Remove dependencies for a specific line (when line is deleted)
     * @param lineNumber Line number to remove
     */
    void removeLine(int lineNumber);
    
    /**
     * Get direct dependencies for a line (what this line depends on)
     * @param lineNumber Line number to query
     * @return Set of line numbers this line depends on
     */
    QSet<int> getDirectDependencies(int lineNumber) const;
    
    /**
     * Get reverse dependencies for a line (what depends on this line)
     * @param lineNumber Line number to query  
     * @return Set of line numbers that depend on this line
     */
    QSet<int> getReverseDependencies(int lineNumber) const;
    
    /**
     * Check if there are any circular dependencies
     * @return True if circular dependencies detected
     */
    bool hasCircularDependencies() const;

    /**
     * Check for circular dependencies across sheets
     * @param currentSheetName Name of the current sheet
     * @param sheetLookupFunction Function to get worksheet by name
     * @return True if cross-sheet circular dependencies detected
     */
    bool hasCrossSheetCircularDependencies(const QString &currentSheetName,
                                          std::function<class WorksheetWidget*(const QString&)> sheetLookupFunction) const;

    /**
     * Get cross-sheet references for a specific line
     * @param lineNumber Line number to query
     * @return Set of cross-sheet references this line uses
     */
    QSet<CrossSheetReference> getCrossSheetReferences(int lineNumber) const;

    /**
     * Get all lines that reference a specific sheet and line
     * @param sheetName Name of the referenced sheet (case-insensitive)
     * @param lineNumber Line number in the referenced sheet
     * @return Set of line numbers that reference the specified sheet line
     */
    QSet<int> getLinesThatReference(const QString &sheetName, int lineNumber) const;

    /**
     * Check if any lines have cross-sheet references
     * @return True if cross-sheet references exist
     */
    bool hasCrossSheetReferences() const;

private:
    /**
     * Recursively detect cycles in cross-sheet dependencies
     */
    bool detectCrossSheetCycle(const QString &sheetName,
                              std::function<class WorksheetWidget*(const QString&)> sheetLookupFunction,
                              QSet<QString> &visitedSheets,
                              QSet<QString> &currentPath) const;

    /**
     * Get all sheets referenced by a worksheet
     */
    QSet<QString> getCrossSheetReferencesFromWorksheet(class WorksheetWidget *worksheet) const;
    
    /**
     * Get dependency statistics for debugging
     */
    struct Stats {
        int totalDependencies;
        int maxDependenciesPerLine;
        int linesWithDependencies;
    };
    Stats getStats() const;

private:
    // Forward dependencies: line -> set of lines it depends on
    QHash<int, QSet<int>> m_forwardDeps;

    // Reverse dependencies: line -> set of lines that depend on it
    QHash<int, QSet<int>> m_reverseDeps;

    // Cross-sheet dependencies: line -> set of cross-sheet references it uses
    QHash<int, QSet<CrossSheetReference>> m_crossSheetRefs;

    // Reverse cross-sheet dependencies: (sheet, line) -> set of lines that reference it
    QHash<QPair<QString, int>, QSet<int>> m_reverseCrossSheetRefs;

    // Compiled regex patterns for performance (created once, used many times)
    static const QRegularExpression s_lnPattern;
    static const QRegularExpression s_statFunctionPattern;
    static const QRegularExpression s_rangePattern;
    static const QRegularExpression s_crossSheetPattern;
    
    /**
     * Parse expression to extract dependencies
     * @param expression Expression to parse
     * @param lineNumber Current line number for context
     * @param totalLines Total lines in worksheet
     * @return Set of line numbers this expression depends on
     */
    QSet<int> parseExpressionDependencies(const QString &expression, int lineNumber, int totalLines) const;

    /**
     * Parse expression to extract cross-sheet references
     * @param expression Expression to parse
     * @return Set of cross-sheet references found in the expression
     */
    QSet<CrossSheetReference> parseCrossSheetReferences(const QString &expression) const;
    
    /**
     * Parse range expression (e.g., "1-5", "above", "below")
     * @param rangeExpr Range expression
     * @param currentLine Current line number for relative ranges
     * @param totalLines Total lines for "below" calculation
     * @return Set of line numbers in the range
     */
    QSet<int> parseRange(const QString &rangeExpr, int currentLine, int totalLines) const;
    
    /**
     * Add a dependency relationship
     * @param fromLine Line that depends
     * @param toLine Line being depended upon
     */
    inline void addDependency(int fromLine, int toLine);
    
    /**
     * Remove a dependency relationship
     * @param fromLine Line that depends
     * @param toLine Line being depended upon
     */
    inline void removeDependency(int fromLine, int toLine);
    
    /**
     * Perform depth-first search to find all dependent lines
     * @param startLines Starting set of changed lines
     * @param visited Set to track visited lines (prevents infinite loops)
     * @param result Set to accumulate all dependent lines
     */
    void dfsCollectDependents(const QSet<int> &startLines,
                             QSet<int> &visited,
                             QSet<int> &result) const;

    /**
     * DFS helper for topological sorting
     * @param line Current line being processed
     * @param linesToEvaluate Set of lines to consider for evaluation
     * @param visited Set to track visited lines
     * @param recursionStack Set to detect cycles
     * @param result List to accumulate sorted lines
     */
    void topologicalSortDFS(int line,
                           const QSet<int> &linesToEvaluate,
                           QSet<int> &visited,
                           QSet<int> &recursionStack,
                           QList<int> &result) const;

    /**
     * Helper for circular dependency detection using DFS
     * @param line Current line being checked
     * @param visited Set of visited lines
     * @param recursionStack Current recursion stack
     * @return True if cycle detected
     */
    bool hasCycleDFS(int line, QSet<int> &visited, QSet<int> &recursionStack) const;
};

// Hash function for Dependency struct (needed for QSet)
inline uint qHash(const DependencyTracker::Dependency &dep, uint seed = 0) {
    return qHash(dep.fromLine, seed) ^ qHash(dep.toLine, seed + 1) ^ qHash(dep.type, seed + 2);
}

// Hash function for CrossSheetReference struct (needed for QSet and QHash)
inline uint qHash(const DependencyTracker::CrossSheetReference &ref, uint seed = 0) {
    return qHash(ref.sheetName.toLower(), seed) ^ qHash(ref.lineNumber, seed + 1);
}
