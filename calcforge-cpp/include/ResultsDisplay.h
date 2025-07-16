#ifndef RESULTSDISPLAY_H
#define RESULTSDISPLAY_H

#include <QTextEdit>
#include <QStringList>

class LineNumberArea;

/**
 * Results display widget for showing calculation results
 * Read-only QTextEdit that displays results synchronized with expressions
 */
class ResultsDisplay : public QTextEdit
{
    Q_OBJECT

public:
    explicit ResultsDisplay(QWidget *parent = nullptr);
    ~ResultsDisplay();

    // Line number area management (same as ExpressionEditor)
    void setLineNumberArea(LineNumberArea *lineNumberArea);
    LineNumberArea* getLineNumberArea() const { return m_lineNumberArea; }
    void updateLineNumberAreaWidth();
    void updateLineNumberArea(const QRect &rect, int dy);
    void updateLineNumberArea(); // Overload for signal connections

    // Content management
    void setResults(const QStringList &results);
    void setResult(int lineNumber, const QString &result);
    void clearResults();
    int getResultCount() const;
    QStringList getResults() const;

    // Line number area support (expose protected methods like ExpressionEditor)
    QTextBlock getFirstVisibleBlock() const;
    QRectF getBlockBoundingGeometry(const QTextBlock &block) const;
    QPointF getContentOffset() const;
    QRectF getBlockBoundingRect(const QTextBlock &block) const;
    int getCurrentLineNumber() const;
    int getLineCount() const;
    
    // Font management (synchronized with expression editor)
    void increaseFontSize();
    void decreaseFontSize();
    void resetFontSize();
    void setDefaultFont();
    void synchronizeFontWith(const QFont &font);
    
    // Display formatting
    void updateLineCount(int lineCount);
    void setLineHeight(int height);

    // Comment line detection (for line number coloring)
    bool isCommentLine(int lineNumber) const;

    // Current line highlighting
    void highlightCurrentLine(int lineNumber);
    void highlightCurrentLineWithLNReferences(int lineNumber, const QString &currentLineText);
    void highlightSpecificLine(int lineNumber, const QColor &lnColor = QColor()); // For cross-sheet highlighting
    void clearCrossSheetHighlighting(); // Clear cross-sheet highlighting
    void setCurrentLineHighlightingEnabled(bool enabled);
    bool isCurrentLineHighlightingEnabled() const noexcept;

signals:
    void scrollRequested(int value);

protected:
    void wheelEvent(QWheelEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onVerticalScrollChanged(int value);

private:
    void setupDisplay();
    void setupConnections();
    void updateContent();
    void updateContentForced();
    void formatResults();

    // Line number area
    LineNumberArea *m_lineNumberArea;

    // Content storage
    QStringList m_results;
    int m_lineCount;

    // Font management
    QFont m_defaultFont;
    int m_baseFontSize;

    // Display state
    bool m_isUpdating;

    // Current line highlighting
    bool m_currentLineHighlightingEnabled;
    int m_currentHighlightedLine;
    int m_crossSheetHighlightedLine; // For cross-sheet highlighting persistence
    QColor m_crossSheetHighlightColor; // Store the LN color for cross-sheet highlighting
    static const QColor s_currentLineBackgroundColor;
};

#endif // RESULTSDISPLAY_H
