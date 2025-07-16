#ifndef EXPRESSIONEDITOR_H
#define EXPRESSIONEDITOR_H

#include <QTextEdit>
#include <QTextCursor>
#include <QKeyEvent>
#include <QWheelEvent>
#include <QScrollBar>
#include <QRegularExpression>

class LineNumberArea;
class SyntaxHighlighter;

/**
 * Expression editor widget for entering mathematical expressions
 * Subclass of QTextEdit with CalcForge-specific functionality
 */
class ExpressionEditor : public QTextEdit
{
    Q_OBJECT

public:
    explicit ExpressionEditor(QWidget *parent = nullptr);
    ~ExpressionEditor();
    
    // Line number area management
    void setLineNumberArea(LineNumberArea *lineNumberArea);
    void updateLineNumberAreaWidth();
    void updateLineNumberArea(const QRect &rect, int dy);
    void updateLineNumberArea(); // Overload for signal connections
    
    // Font and display
    void increaseFontSize();
    void decreaseFontSize();
    void resetFontSize();
    void setDefaultFont();
    
    // Content management
    int getLineCount() const;
    QString getLineText(int lineNumber) const;
    void setLineText(int lineNumber, const QString &text);
    
    // Cursor and selection
    void positionCursorAtEnd();
    int getCurrentLineNumber() const;
    void selectCurrentLine();
    void smartParenthesesSelection();

    // Smart navigation
    void jumpToPreviousNumberOrLN();
    void jumpToNextNumberOrLN();

    // Copy functionality
    void handleCopyShortcut();

    // Syntax highlighting
    void setSyntaxHighlightingEnabled(bool enabled);
    bool isSyntaxHighlightingEnabled() const;
    void setColorBlindMode(bool enabled);
    bool isColorBlindMode() const;

    // Current line highlighting
    void highlightCurrentLine();
    void setCurrentLineHighlightingEnabled(bool enabled);
    bool isCurrentLineHighlightingEnabled() const noexcept;

    // Line number area support (expose protected methods)
    QTextBlock getFirstVisibleBlock() const;
    QRectF getBlockBoundingGeometry(const QTextBlock &block) const;
    QPointF getContentOffset() const;
    QRectF getBlockBoundingRect(const QTextBlock &block) const;

signals:
    void contentChanged();
    void lineCountChanged();
    void fontSizeIncreaseRequested();
    void fontSizeDecreaseRequested();
    void fontSizeResetRequested();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onTextChanged();
    void onCursorPositionChanged();

private:
    void setupEditor();
    void setupConnections();
    void updateViewport();
    
    // Line number area
    LineNumberArea *m_lineNumberArea;

    // Syntax highlighting
    SyntaxHighlighter *m_syntaxHighlighter;
    bool m_syntaxHighlightingEnabled;

    // Current line highlighting
    bool m_currentLineHighlightingEnabled;
    static const QColor s_currentLineBackgroundColor;

    // Font management
    QFont m_defaultFont;
    int m_baseFontSize;

    // State tracking
    int m_lastLineCount;
    bool m_isUpdating;
};

#endif // EXPRESSIONEDITOR_H
