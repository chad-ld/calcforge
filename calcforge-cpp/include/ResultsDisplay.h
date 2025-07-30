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
};

#endif // RESULTSDISPLAY_H
