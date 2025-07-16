#include "SyntaxHighlighter.h"
#include "Logger.h"
#include <vector>
#include <algorithm>

// Static color definitions (exact match to Electron version)
const QColor SyntaxHighlighter::COLOR_NUMBER = QColor("#FFFFFF");
const QColor SyntaxHighlighter::COLOR_OPERATOR = QColor("#FF8C00");
const QColor SyntaxHighlighter::COLOR_FUNCTION = QColor("#FF8C00");
const QColor SyntaxHighlighter::COLOR_PARENTHESES = QColor("#00FF00");
const QColor SyntaxHighlighter::COLOR_COMMENT = QColor("#00FF00");
const QColor SyntaxHighlighter::COLOR_ERROR = QColor("#FF0000");
const QColor SyntaxHighlighter::COLOR_CROSS_SHEET = QColor("#87CEEB");

// LN variable colors - 17 rotating colors (normal mode)
const QStringList SyntaxHighlighter::LN_COLORS_NORMAL = {
    "#FF6B6B", "#4ECDC4", "#45B7D1", "#96CEB4", "#FFEAA7",
    "#DDA0DD", "#98D8C8", "#F7DC6F", "#BB8FCE", "#85C1E9",
    "#F8C471", "#82E0AA", "#F1948A", "#85CDFD", "#D7BDE2",
    "#A9DFBF", "#F9E79F"
};

// LN variable colors - color blind friendly
const QStringList SyntaxHighlighter::LN_COLORS_COLORBLIND = {
    "#E69F00", "#56B4E9", "#009E73", "#F0E442", "#0072B2",
    "#D55E00", "#CC79A7", "#E69F00", "#56B4E9", "#009E73",
    "#F0E442", "#0072B2", "#D55E00", "#CC79A7", "#E69F00",
    "#56B4E9", "#009E73"
};

SyntaxHighlighter::SyntaxHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
    , m_colorBlindMode(false)
{
    LOG_DEBUG("Initializing SyntaxHighlighter");

    // Initialize function names for highlighting
    m_functionNames = {
        // Basic math functions
        "sin", "cos", "tan", "asin", "acos", "atan",
        "sinh", "cosh", "tanh", "asinh", "acosh", "atanh",
        "sqrt", "abs", "log", "log10", "log2", "exp",
        "floor", "ceil", "round", "truncate", "TR",
        "degrees", "radians", "factorial", "gcd", "lcm", "pow",
        
        // Statistical functions
        "sum", "mean", "min", "max", "count", "product",
        "range", "median", "variance", "stdev", "geomean",
        "harmmean", "sumsq", "mode", "perc5", "perc95", "meanfps",
        
        // Special functions
        "D", "TC", "AR", "percent", "S"
    };

    // Compile regex patterns for performance
    m_numberPattern = QRegularExpression(R"(\b\d+(?:\.\d+)?\b)");
    m_operatorPattern = QRegularExpression(R"(\bto\b|[+\-*/%^=])");
    m_lnPattern = QRegularExpression(R"(\bLN(\d+)\b)", QRegularExpression::CaseInsensitiveOption);
    m_crossSheetPattern = QRegularExpression(R"(S\.[^.]+\.LN\d+\b)", QRegularExpression::CaseInsensitiveOption);
    m_commentPattern = QRegularExpression(R"(^:::.*$)");

    // Build function pattern dynamically
    QString functionPatternStr = R"(\b()" + m_functionNames.join("|") + R"()\b(?=\s*\())";
    m_functionPattern = QRegularExpression(functionPatternStr, QRegularExpression::CaseInsensitiveOption);

    // Pre-create text formats for performance
    m_numberFormat = createFormat(COLOR_NUMBER);
    m_operatorFormat = createFormat(COLOR_OPERATOR);
    m_functionFormat = createFormat(COLOR_FUNCTION);
    m_parenthesesFormat = createFormat(COLOR_PARENTHESES);
    m_commentFormat = createFormat(COLOR_COMMENT, true); // Bold
    m_errorFormat = createFormat(COLOR_ERROR);
    m_crossSheetFormat = createFormat(COLOR_CROSS_SHEET);

    LOG_DEBUG("SyntaxHighlighter initialized successfully");
}

SyntaxHighlighter::~SyntaxHighlighter()
{
    LOG_DEBUG("SyntaxHighlighter destroyed");
}

void SyntaxHighlighter::highlightBlock(const QString &text)
{
    if (text.isEmpty()) {
        return;
    }

    // Check for comment lines first (highest priority)
    if (text.trimmed().startsWith(":::")) {
        highlightComments(text);
        return;
    }

    // Apply highlighting in order of precedence (LN variables have high priority)
    highlightNumbers(text);
    highlightOperators(text);
    highlightFunctions(text);
    highlightParentheses(text);
    highlightLNReferences(text);        // Higher priority than cross-sheet
    highlightCrossSheetReferences(text);
}

void SyntaxHighlighter::highlightNumbers(const QString &text)
{
    QRegularExpressionMatchIterator iterator = m_numberPattern.globalMatch(text);
    while (iterator.hasNext()) {
        QRegularExpressionMatch match = iterator.next();
        applyFormat(match.capturedStart(), match.capturedLength(), m_numberFormat);
    }
}

void SyntaxHighlighter::highlightOperators(const QString &text)
{
    QRegularExpressionMatchIterator iterator = m_operatorPattern.globalMatch(text);
    while (iterator.hasNext()) {
        QRegularExpressionMatch match = iterator.next();
        applyFormat(match.capturedStart(), match.capturedLength(), m_operatorFormat);
    }
}

void SyntaxHighlighter::highlightFunctions(const QString &text)
{
    QRegularExpressionMatchIterator iterator = m_functionPattern.globalMatch(text);
    while (iterator.hasNext()) {
        QRegularExpressionMatch match = iterator.next();
        applyFormat(match.capturedStart(), match.capturedLength(), m_functionFormat);
    }
}

void SyntaxHighlighter::highlightParentheses(const QString &text)
{
    for (int i = 0; i < text.length(); ++i) {
        if (text[i] == '(' || text[i] == ')') {
            applyFormat(i, 1, m_parenthesesFormat);
        }
    }
}

void SyntaxHighlighter::highlightLNReferences(const QString &text)
{
    // Use C++ strengths: efficient iteration and strong typing
    QRegularExpressionMatchIterator iterator = m_lnPattern.globalMatch(text);

    // Pre-allocate vector for better performance with known capacity
    std::vector<std::pair<int, int>> lnMatches;
    lnMatches.reserve(8); // Reserve space for typical number of LN refs per line

    // First pass: collect all LN matches for efficient processing
    while (iterator.hasNext()) {
        const QRegularExpressionMatch match = iterator.next();
        const int lnNumber = match.captured(1).toInt();
        const int startPos = match.capturedStart();
        const int length = match.capturedLength();

        // Always color LN variables, even in cross-sheet references
        // This ensures s.data.LN1 shows LN1 in the proper color

        // Get or create format for this LN number using efficient caching
        QTextCharFormat format;
        auto cacheIt = m_lnFormatCache.constFind(lnNumber);
        if (cacheIt != m_lnFormatCache.constEnd()) {
            format = cacheIt.value();
        } else {
            const QColor color = getLNColor(lnNumber);
            format = createFormat(color, true); // Very bold for LN variables
            format.setFontWeight(QFont::Black); // Extra bold (900 weight)

            // Use efficient insert for caching
            m_lnFormatCache.insert(lnNumber, format);
        }

        // Apply format immediately for better visual feedback
        applyFormat(startPos, length, format);

        // Store for potential background highlighting
        lnMatches.emplace_back(lnNumber, startPos);
    }


}

void SyntaxHighlighter::highlightCrossSheetReferences(const QString &text)
{
    // Pattern to match cross-sheet references and capture parts separately
    QRegularExpression detailedPattern(R"(\b(S\.)([^.]+)(\.)LN(\d+)\b)", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatchIterator iterator = detailedPattern.globalMatch(text);

    while (iterator.hasNext()) {
        QRegularExpressionMatch match = iterator.next();

        // Only highlight the "S.sheetname." part, leave "LN1" with its LN color
        int sPartStart = match.capturedStart(1); // "S."
        int sheetNameStart = match.capturedStart(2); // sheet name
        int dotStart = match.capturedStart(3); // "."

        // Highlight "S." part
        applyFormat(sPartStart, match.capturedLength(1), m_crossSheetFormat);

        // Highlight sheet name part
        applyFormat(sheetNameStart, match.capturedLength(2), m_crossSheetFormat);

        // Highlight "." part
        applyFormat(dotStart, match.capturedLength(3), m_crossSheetFormat);

        // Don't highlight the LN part - it's already colored by highlightLNReferences
    }
}

void SyntaxHighlighter::highlightComments(const QString &text)
{
    // Highlight entire line as comment
    applyFormat(0, text.length(), m_commentFormat);
}

QColor SyntaxHighlighter::getLNColor(int lnNumber)
{
    // Use C++ strengths: efficient const lookup with iterators
    auto cacheIt = m_lnColorCache.constFind(lnNumber);
    if (cacheIt != m_lnColorCache.constEnd()) {
        return cacheIt.value();
    }

    // Assign color based on the order of appearance (like Python version)
    // Use const reference for efficiency and strong typing
    const QStringList &colors = m_colorBlindMode ? LN_COLORS_COLORBLIND : LN_COLORS_NORMAL;
    const int colorIndex = m_lnColorCache.size() % colors.size();

    // Ensure we have valid colors array
    Q_ASSERT(!colors.isEmpty());
    Q_ASSERT(colorIndex >= 0 && colorIndex < colors.size());

    const QColor color(colors.at(colorIndex)); // Use at() for bounds checking in debug

    // Cache the color using efficient insert
    m_lnColorCache.insert(lnNumber, color);

    return color;
}

QTextCharFormat SyntaxHighlighter::createFormat(const QColor &color, bool bold)
{
    QTextCharFormat format;
    format.setForeground(color);
    if (bold) {
        format.setFontWeight(QFont::Bold);
    }
    return format;
}

void SyntaxHighlighter::applyFormat(int start, int length, const QTextCharFormat &format)
{
    setFormat(start, length, format);
}

void SyntaxHighlighter::setColorBlindMode(bool enabled)
{
    if (m_colorBlindMode != enabled) {
        m_colorBlindMode = enabled;
        
        // Clear LN color caches to force regeneration with new colors
        m_lnColorCache.clear();
        m_lnFormatCache.clear();
        
        // Trigger re-highlighting
        rehighlight();
        
        LOG_INFO(QString("Color blind mode %1").arg(enabled ? "enabled" : "disabled"));
    }
}

bool SyntaxHighlighter::isColorBlindMode() const
{
    return m_colorBlindMode;
}
