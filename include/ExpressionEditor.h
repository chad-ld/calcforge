#ifndef EXPRESSIONEDITOR_H
#define EXPRESSIONEDITOR_H

#include <QTextEdit>
#include <QTextCursor>
#include <QKeyEvent>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QScrollBar>
#include <QRegularExpression>
#include <QToolTip>

// Forward declarations
class AutoCompleteManager;

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
    void positionCursorAtStart();
    int getCurrentLineNumber() const;
    void selectCurrentLine();
    void smartParenthesesSelection();

    // Smart navigation
    void jumpToPreviousNumberOrLN();
    void jumpToNextNumberOrLN();

    // Cross-sheet navigation
    void handleCrossSheetNavigation();
    void handleCrossSheetReturn();

    // Copy functionality
    void handleCopyShortcut();

    // Syntax highlighting
    void setSyntaxHighlightingEnabled(bool enabled);
    bool isSyntaxHighlightingEnabled() const;
    void setColorBlindMode(bool enabled);
    bool isColorBlindMode() const;
    SyntaxHighlighter* getSyntaxHighlighter() const { return m_syntaxHighlighter; }

    // Current line highlighting
    void highlightCurrentLine();
    void highlightSpecificLine(int lineNumber, const QColor &lnColor = QColor()); // For cross-sheet highlighting
    void clearCrossSheetHighlighting(); // Clear cross-sheet highlighting
    void setCurrentLineHighlightingEnabled(bool enabled);
    bool isCurrentLineHighlightingEnabled() const noexcept;

    // Tooltip functionality
    void setTooltipsEnabled(bool enabled);
    bool areTooltipsEnabled() const;

    // Operator highlighting functionality
    void setOperatorHighlightingEnabled(bool enabled);
    bool isOperatorHighlightingEnabled() const;

    // Autocomplete functionality
    void setAutoCompleteEnabled(bool enabled);
    bool isAutoCompleteEnabled() const;

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
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;

private slots:
    void onTextChanged();
    void onCursorPositionChanged();

private:
    void setupEditor();
    void setupConnections();
    void updateViewport();

    // Key handling methods
    void handleEnterKey(QKeyEvent *event);

    // Cross-sheet navigation helper methods
    QString getCurrentSheetName(class MainWindow *mainWindow) const;
    void navigateToSheet(class MainWindow *mainWindow, const QString &sheetName, int lineNumber, int cursorPosition = -1);

    // Tooltip helper methods
    void showTooltipForPosition(const QPoint &globalPos, int textPosition);
    QString getLNVariableTooltip(const QString &text, int position);
    QString getOperatorTooltip(const QString &text, int position);
    QString evaluateSubExpression(const QString &expression, int operatorPos);
    QPair<int, int> findSubExpressionBounds(const QString &text, int operatorPos);
    QString extractLeftOperand(const QString &expression, int operatorPos);
    QString extractRightOperand(const QString &expression, int operatorPos);

    // Operator highlighting helper methods
    void updateOperatorHighlighting(const QString &text, int positionInLine, int absolutePosition, int blockStart);
    void clearOperatorHighlighting();
    QPair<int, int> getOperatorExpressionBounds(const QString &text, int operatorPos);
    QColor getOperatorHighlightColor(QChar operatorChar, bool isCurrentLineHighlighted) const;
    
    // Line number area
    LineNumberArea *m_lineNumberArea;

    // Syntax highlighting
    SyntaxHighlighter *m_syntaxHighlighter;
    bool m_syntaxHighlightingEnabled;

    // Current line highlighting
    bool m_currentLineHighlightingEnabled;
    int m_crossSheetHighlightedLine; // For cross-sheet highlighting persistence
    QColor m_crossSheetHighlightColor; // Store the LN color for cross-sheet highlighting
    static const QColor s_currentLineBackgroundColor;

    // Tooltip functionality
    bool m_tooltipsEnabled;

    // Operator highlighting functionality
    bool m_operatorHighlightingEnabled;
    int m_currentOperatorAbsolutePosition; // Absolute position in document of currently highlighted operator (-1 if none)
    QPair<int, int> m_currentOperatorAbsoluteBounds; // Absolute start and end positions of highlighted expression
    QChar m_currentOperatorChar; // The operator character being highlighted

    // Autocomplete functionality
    AutoCompleteManager *m_autoCompleteManager;
    bool m_autoCompleteEnabled;

    // Font management
    QFont m_defaultFont;
    int m_baseFontSize;

    // State tracking
    int m_lastLineCount;
    bool m_isUpdating;
};

#endif // EXPRESSIONEDITOR_H
