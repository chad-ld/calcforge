#ifndef LINECHANGEDETECTOR_H
#define LINECHANGEDETECTOR_H

#include <QStringList>
#include <QSet>
#include <QHash>

/**
 * Represents a line change operation (insertion or deletion)
 */
struct LineChange {
    enum Type {
        Insertion,  // Lines were inserted
        Deletion,   // Lines were deleted
        Modification // Line content was modified (no line number changes)
    };
    
    Type type;
    int startLine;      // First line affected (1-based)
    int count;          // Number of lines inserted/deleted
    QStringList oldContent;  // Previous content (for deletions)
    QStringList newContent;  // New content (for insertions)
    
    LineChange() : type(Modification), startLine(0), count(0) {}
    LineChange(Type t, int start, int cnt) : type(t), startLine(start), count(cnt) {}
};

/**
 * Detects line insertions, deletions, and modifications between text versions
 * Provides precise tracking for LN reference auto-update system
 */
class LineChangeDetector
{
public:
    LineChangeDetector();
    
    /**
     * Analyze changes between old and new text content
     * @param oldLines Previous text content split by lines
     * @param newLines Current text content split by lines
     * @return List of detected line changes
     */
    QList<LineChange> detectChanges(const QStringList &oldLines, const QStringList &newLines);
    
    /**
     * Get all line numbers that need LN reference updates
     * @param changes List of line changes
     * @return Set of line numbers that may contain LN references needing updates
     */
    QSet<int> getAffectedLines(const QList<LineChange> &changes) const;
    
    /**
     * Calculate new line number after applying changes
     * @param originalLine Original line number (1-based)
     * @param changes List of changes to apply
     * @return New line number, or -1 if line was deleted
     */
    int calculateNewLineNumber(int originalLine, const QList<LineChange> &changes) const;
    
    /**
     * Check if a line number range is affected by changes
     * @param startLine Start of range (1-based, inclusive)
     * @param endLine End of range (1-based, inclusive)
     * @param changes List of changes
     * @return True if any line in range is affected
     */
    bool isRangeAffected(int startLine, int endLine, const QList<LineChange> &changes) const;

private:
    /**
     * Find the longest common subsequence between two line lists
     * Used for efficient diff algorithm
     */
    QList<QPair<int, int>> findLCS(const QStringList &oldLines, const QStringList &newLines);
    
    /**
     * Convert LCS result into specific line changes
     */
    QList<LineChange> convertLCSToChanges(const QStringList &oldLines, 
                                         const QStringList &newLines,
                                         const QList<QPair<int, int>> &lcs);
    
    /**
     * Merge consecutive changes of the same type for efficiency
     */
    QList<LineChange> mergeConsecutiveChanges(const QList<LineChange> &changes);
};

#endif // LINECHANGEDETECTOR_H
