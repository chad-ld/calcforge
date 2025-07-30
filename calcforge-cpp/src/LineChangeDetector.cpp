#include "LineChangeDetector.h"
#include "Logger.h"
#include <QDebug>
#include <algorithm>

LineChangeDetector::LineChangeDetector()
{
}

QList<LineChange> LineChangeDetector::detectChanges(const QStringList &oldLines, const QStringList &newLines)
{
    // Handle empty cases
    if (oldLines.isEmpty() && newLines.isEmpty()) {
        return QList<LineChange>();
    }
    
    if (oldLines.isEmpty()) {
        // All lines are insertions
        LineChange change(LineChange::Insertion, 1, newLines.size());
        change.newContent = newLines;
        return QList<LineChange>() << change;
    }
    
    if (newLines.isEmpty()) {
        // All lines are deletions
        LineChange change(LineChange::Deletion, 1, oldLines.size());
        change.oldContent = oldLines;
        return QList<LineChange>() << change;
    }
    
    // Quick check: if line count is the same, only content was modified (no insertions/deletions)
    if (oldLines.size() == newLines.size()) {
        // Only content modifications, no line number changes
        QList<LineChange> modifications;
        for (int i = 0; i < oldLines.size(); ++i) {
            if (oldLines[i] != newLines[i]) {
                LineChange change(LineChange::Modification, i + 1, 1);
                change.oldContent = QStringList() << oldLines[i];
                change.newContent = QStringList() << newLines[i];
                modifications.append(change);
            }
        }

        LOG_DEBUG(QString("Detected %1 content modifications (no line insertions/deletions)")
                  .arg(modifications.size()));

        return modifications;
    }

    // Use LCS algorithm for efficient diff when line count changed
    QList<QPair<int, int>> lcs = findLCS(oldLines, newLines);
    QList<LineChange> changes = convertLCSToChanges(oldLines, newLines, lcs);

    // Merge consecutive changes for efficiency
    changes = mergeConsecutiveChanges(changes);

    LOG_DEBUG(QString("Detected %1 line changes between %2 old lines and %3 new lines")
              .arg(changes.size()).arg(oldLines.size()).arg(newLines.size()));

    return changes;
}

QSet<int> LineChangeDetector::getAffectedLines(const QList<LineChange> &changes) const
{
    QSet<int> affectedLines;
    
    for (const LineChange &change : changes) {
        switch (change.type) {
        case LineChange::Insertion:
            // Lines after insertion point need LN reference updates
            // The insertion itself doesn't affect existing LN references
            // but lines after it will have their numbers shifted
            break;
            
        case LineChange::Deletion:
            // Lines after deletion point need LN reference updates
            // Also, any LN references to deleted lines need handling
            break;
            
        case LineChange::Modification:
            // Only the modified line itself might need updates
            affectedLines.insert(change.startLine);
            break;
        }
    }
    
    return affectedLines;
}

int LineChangeDetector::calculateNewLineNumber(int originalLine, const QList<LineChange> &changes) const
{
    int newLineNumber = originalLine;
    
    // Apply changes in order
    for (const LineChange &change : changes) {
        if (change.type == LineChange::Insertion) {
            // If insertion happens before this line, shift line number up
            if (change.startLine <= originalLine) {
                newLineNumber += change.count;
            }
        } else if (change.type == LineChange::Deletion) {
            // If deletion happens before this line, shift line number down
            if (change.startLine <= originalLine) {
                if (originalLine < change.startLine + change.count) {
                    // This line was deleted
                    return -1;
                } else {
                    // This line comes after the deletion
                    newLineNumber -= change.count;
                }
            }
        }
        // Modifications don't change line numbers
    }
    
    return newLineNumber;
}

bool LineChangeDetector::isRangeAffected(int startLine, int endLine, const QList<LineChange> &changes) const
{
    for (const LineChange &change : changes) {
        // Check if change overlaps with the range
        int changeEnd = change.startLine + change.count - 1;
        
        // Check for overlap: change affects range if:
        // 1. Change starts within range
        // 2. Change ends within range  
        // 3. Change completely encompasses range
        // 4. Range completely encompasses change
        bool overlaps = (change.startLine <= endLine && changeEnd >= startLine);
        
        if (overlaps) {
            return true;
        }
        
        // For insertions/deletions, also check if they affect line numbering
        if (change.type != LineChange::Modification && change.startLine <= endLine) {
            return true;
        }
    }
    
    return false;
}

QList<QPair<int, int>> LineChangeDetector::findLCS(const QStringList &oldLines, const QStringList &newLines)
{
    int oldSize = oldLines.size();
    int newSize = newLines.size();
    
    // Dynamic programming table for LCS
    QVector<QVector<int>> dp(oldSize + 1, QVector<int>(newSize + 1, 0));
    
    // Fill the DP table
    for (int i = 1; i <= oldSize; ++i) {
        for (int j = 1; j <= newSize; ++j) {
            if (oldLines[i-1] == newLines[j-1]) {
                dp[i][j] = dp[i-1][j-1] + 1;
            } else {
                dp[i][j] = std::max(dp[i-1][j], dp[i][j-1]);
            }
        }
    }
    
    // Backtrack to find the actual LCS
    QList<QPair<int, int>> lcs;
    int i = oldSize, j = newSize;
    
    while (i > 0 && j > 0) {
        if (oldLines[i-1] == newLines[j-1]) {
            lcs.prepend(QPair<int, int>(i-1, j-1));  // 0-based indices
            i--;
            j--;
        } else if (dp[i-1][j] > dp[i][j-1]) {
            i--;
        } else {
            j--;
        }
    }
    
    return lcs;
}

QList<LineChange> LineChangeDetector::convertLCSToChanges(const QStringList &oldLines, 
                                                         const QStringList &newLines,
                                                         const QList<QPair<int, int>> &lcs)
{
    QList<LineChange> changes;
    
    int oldIndex = 0, newIndex = 0;
    
    for (const QPair<int, int> &match : lcs) {
        int oldMatchIndex = match.first;
        int newMatchIndex = match.second;
        
        // Handle deletions before this match
        if (oldIndex < oldMatchIndex) {
            LineChange deletion(LineChange::Deletion, oldIndex + 1, oldMatchIndex - oldIndex);
            for (int i = oldIndex; i < oldMatchIndex; ++i) {
                deletion.oldContent.append(oldLines[i]);
            }
            changes.append(deletion);
        }
        
        // Handle insertions before this match
        if (newIndex < newMatchIndex) {
            LineChange insertion(LineChange::Insertion, oldIndex + 1, newMatchIndex - newIndex);
            for (int i = newIndex; i < newMatchIndex; ++i) {
                insertion.newContent.append(newLines[i]);
            }
            changes.append(insertion);
        }
        
        oldIndex = oldMatchIndex + 1;
        newIndex = newMatchIndex + 1;
    }
    
    // Handle remaining deletions
    if (oldIndex < oldLines.size()) {
        LineChange deletion(LineChange::Deletion, oldIndex + 1, oldLines.size() - oldIndex);
        for (int i = oldIndex; i < oldLines.size(); ++i) {
            deletion.oldContent.append(oldLines[i]);
        }
        changes.append(deletion);
    }
    
    // Handle remaining insertions
    if (newIndex < newLines.size()) {
        LineChange insertion(LineChange::Insertion, oldIndex + 1, newLines.size() - newIndex);
        for (int i = newIndex; i < newLines.size(); ++i) {
            insertion.newContent.append(newLines[i]);
        }
        changes.append(insertion);
    }
    
    return changes;
}

QList<LineChange> LineChangeDetector::mergeConsecutiveChanges(const QList<LineChange> &changes)
{
    if (changes.isEmpty()) {
        return changes;
    }
    
    QList<LineChange> merged;
    LineChange current = changes.first();
    
    for (int i = 1; i < changes.size(); ++i) {
        const LineChange &next = changes[i];
        
        // Check if we can merge with current change
        bool canMerge = (current.type == next.type && 
                        current.startLine + current.count == next.startLine);
        
        if (canMerge) {
            // Merge the changes
            current.count += next.count;
            current.oldContent.append(next.oldContent);
            current.newContent.append(next.newContent);
        } else {
            // Can't merge, add current to result and start new one
            merged.append(current);
            current = next;
        }
    }
    
    // Add the last change
    merged.append(current);
    
    return merged;
}
