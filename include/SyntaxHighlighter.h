#ifndef SYNTAXHIGHLIGHTER_H
#define SYNTAXHIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QTextDocument>
#include <QTextCharFormat>
#include <QRegularExpression>
#include <QColor>
#include <QFont>
#include <QStringList>
#include <QHash>

/**
 * CalcForge Syntax Highlighter
 * 
 * Provides real-time syntax highlighting for the expression editor with:
 * - Numbers (white)
 * - Operators/Functions (bright orange) 
 * - Parentheses (green)
 * - Comments (bright green, bold)
 * - LN variables (17 rotating colors, very bold)
 * - Cross-sheet references (distinct color)
 * - Errors (red)
 * 
 * Optimized for maximum performance with compiled regex patterns,
 * static color objects, and efficient LN color caching.
 */
class SyntaxHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    explicit SyntaxHighlighter(QTextDocument *parent = nullptr);
    ~SyntaxHighlighter();

    // Color blind support
    void setColorBlindMode(bool enabled);
    bool isColorBlindMode() const;

    // LN color access (for background highlighting)
    QColor getLNColor(int lnNumber);

protected:
    void highlightBlock(const QString &text) override;

private:
    // Core highlighting methods
    void highlightNumbers(const QString &text);
    void highlightOperators(const QString &text);
    void highlightFunctions(const QString &text);
    void highlightParentheses(const QString &text);
    void highlightLNReferences(const QString &text);
    void highlightCrossSheetReferences(const QString &text);
    void highlightComments(const QString &text);

    // LN color management (moved to public)
    void initializeLNColors();

    // Format creation helpers
    QTextCharFormat createFormat(const QColor &color, bool bold = false);
    void applyFormat(int start, int length, const QTextCharFormat &format);

    // Static color definitions (for performance)
    static const QColor COLOR_NUMBER;
    static const QColor COLOR_OPERATOR;
    static const QColor COLOR_FUNCTION;
    static const QColor COLOR_PARENTHESES;
    static const QColor COLOR_COMMENT;
    static const QColor COLOR_ERROR;
    static const QColor COLOR_CROSS_SHEET;

    // LN variable colors (17 rotating colors)
    static const QStringList LN_COLORS_NORMAL;
    static const QStringList LN_COLORS_COLORBLIND;

    // Compiled regex patterns (for performance)
    QRegularExpression m_numberPattern;
    QRegularExpression m_operatorPattern;
    QRegularExpression m_functionPattern;
    QRegularExpression m_lnPattern;
    QRegularExpression m_crossSheetPattern;
    QRegularExpression m_commentPattern;

    // Pre-created text formats (for performance)
    QTextCharFormat m_numberFormat;
    QTextCharFormat m_operatorFormat;
    QTextCharFormat m_functionFormat;
    QTextCharFormat m_parenthesesFormat;
    QTextCharFormat m_commentFormat;
    QTextCharFormat m_errorFormat;
    QTextCharFormat m_crossSheetFormat;

    // LN color caching
    QHash<int, QColor> m_lnColorCache;
    QHash<int, QTextCharFormat> m_lnFormatCache;

    // Color blind mode
    bool m_colorBlindMode;

    // Function names for highlighting
    QStringList m_functionNames;
};

#endif // SYNTAXHIGHLIGHTER_H
