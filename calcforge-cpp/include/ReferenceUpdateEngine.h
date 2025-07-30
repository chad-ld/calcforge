#ifndef REFERENCEUPDATEENGINE_H
#define REFERENCEUPDATEENGINE_H

#include "LineChangeDetector.h"
#include <QStringList>
#include <QRegularExpression>
#include <QSet>
#include <QHash>

/**
 * Represents a reference found in an expression
 */
struct ExpressionReference {
    enum Type {
        LNReference,        // LN1, LN2, etc.
        StatisticalRange,   // sum(2-5), mean(1,3,5), etc.
        RelativeRange       // sum(above), sum(below) - these don't change
    };
    
    Type type;
    int startPos;           // Start position in expression
    int length;             // Length of reference text
    QString originalText;   // Original reference text (e.g., "LN5", "sum(2-5)")
    QList<int> lineNumbers; // Referenced line numbers
    
    ExpressionReference() : type(LNReference), startPos(0), length(0) {}
};

/**
 * Engine for updating LN references and statistical function ranges
 * when lines are inserted or deleted
 */
class ReferenceUpdateEngine
{
public:
    ReferenceUpdateEngine();
    
    /**
     * Update all LN references in worksheet content based on line changes
     * @param content Current worksheet content (will be modified)
     * @param changes List of line changes that occurred
     * @return True if any references were updated
     */
    bool updateReferences(QStringList &content, const QList<LineChange> &changes);
    
    /**
     * Update LN references in a single expression
     * @param expression Expression to update (will be modified)
     * @param changes List of line changes
     * @return True if expression was modified
     */
    bool updateExpressionReferences(QString &expression, const QList<LineChange> &changes);
    
    /**
     * Find all references in an expression
     * @param expression Expression to analyze
     * @return List of found references
     */
    QList<ExpressionReference> findReferences(const QString &expression);
    
    /**
     * Calculate new line number for a reference after changes
     * @param originalLine Original line number
     * @param changes List of changes
     * @return New line number, or -1 if line was deleted
     */
    int calculateNewLineNumber(int originalLine, const QList<LineChange> &changes);
    
    /**
     * Update a range of line numbers (for statistical functions)
     * @param startLine Original start line
     * @param endLine Original end line
     * @param changes List of changes
     * @param newStart Output: new start line
     * @param newEnd Output: new end line
     * @return True if range is still valid after changes
     */
    bool updateRange(int startLine, int endLine, const QList<LineChange> &changes, 
                    int &newStart, int &newEnd);

private:
    /**
     * Parse LN references (LN1, LN2, etc.)
     */
    QList<ExpressionReference> parseLNReferences(const QString &expression);
    
    /**
     * Parse statistical function ranges (sum(2-5), mean(1,3,5), etc.)
     */
    QList<ExpressionReference> parseStatisticalRanges(const QString &expression);
    
    /**
     * Update a single LN reference
     */
    QString updateLNReference(const ExpressionReference &ref, const QList<LineChange> &changes);
    
    /**
     * Update a statistical function range
     */
    QString updateStatisticalRange(const ExpressionReference &ref, const QList<LineChange> &changes);
    
    /**
     * Parse comma-separated line numbers (e.g., "1,3,5")
     */
    QList<int> parseCommaSeparatedNumbers(const QString &text);
    
    /**
     * Parse range notation (e.g., "2-5")
     */
    QPair<int, int> parseRange(const QString &text);
    
    // Regular expressions for different reference types
    QRegularExpression m_lnRegex;
    QRegularExpression m_statisticalRegex;
    QRegularExpression m_rangeRegex;
    QRegularExpression m_commaListRegex;
    
    // Statistical function names that use line references
    QSet<QString> m_statisticalFunctions;
};

#endif // REFERENCEUPDATEENGINE_H
