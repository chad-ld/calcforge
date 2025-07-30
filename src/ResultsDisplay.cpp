#include "ResultsDisplay.h"
#include "ExpressionEditor.h"
#include "SyntaxHighlighter.h"
#include "LineNumberArea.h"
#include "Logger.h"
#include <QScrollBar>
#include <QWheelEvent>
#include <QTextBlock>
#include <QTextCursor>
#include <QAbstractTextDocumentLayout>
#include <QRegularExpression>
#include <QTabWidget>

// Static color definition for current line highlighting (matches Python version)
const QColor ResultsDisplay::s_currentLineBackgroundColor = QColor(65, 65, 66);

ResultsDisplay::ResultsDisplay(QWidget *parent)
    : QTextEdit(parent)
    , m_lineNumberArea(nullptr)
    , m_lineCount(0)
    , m_baseFontSize(15)  // Increased from 10 to 15 (5 steps larger)
    , m_isUpdating(false)
    , m_currentLineHighlightingEnabled(true)
    , m_currentHighlightedLine(-1)
    , m_lastCurrentLineText("")
    , m_crossSheetHighlightedLine(-1)
    , m_crossSheetHighlightColor()
{
    setupDisplay();
    setupConnections();
}

ResultsDisplay::~ResultsDisplay()
{
}

void ResultsDisplay::setupDisplay()
{
    // Make read-only
    setReadOnly(true);
    
    // Set default font
    setDefaultFont();
    
    // Configure display properties
    setLineWrapMode(QTextEdit::NoWrap);
    setTabStopDistance(40);

    // Always show horizontal scrollbar for consistency with ExpressionEditor
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    
    // Set background and text colors to EXACTLY match ExpressionEditor with Material Design flat scrollbars
    setStyleSheet(
        "QTextEdit {"
        "  background-color: #161B22;"
        "  color: #ffffff;"
        "  border: none;"
        "  font-family: 'Consolas', 'Monaco', monospace;"
        "  padding: 0px;"
        "}"

        /* Material Design flat scrollbars - no shadows or gradients */
        "QScrollBar:vertical {"
        "  background-color: #161B22;"
        "  width: 12px;"
        "  border: none;"
        "  margin: 0px;"
        "}"
        "QScrollBar::handle:vertical {"
        "  background-color: #30363D;"
        "  border: none;"
        "  border-radius: 6px;"
        "  margin: 2px;"
        "  min-height: 20px;"
        "}"
        "QScrollBar::handle:vertical:hover {"
        "  background-color: #484F58;"
        "}"
        "QScrollBar::handle:vertical:pressed {"
        "  background-color: #656D76;"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
        "  height: 0px;"
        "  border: none;"
        "  background: none;"
        "}"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {"
        "  background: none;"
        "}"

        "QScrollBar:horizontal {"
        "  background-color: #161B22;"
        "  height: 12px;"
        "  border: none;"
        "  margin: 0px;"
        "}"
        "QScrollBar::handle:horizontal {"
        "  background-color: #30363D;"
        "  border: none;"
        "  border-radius: 6px;"
        "  margin: 2px;"
        "  min-width: 20px;"
        "}"
        "QScrollBar::handle:horizontal:hover {"
        "  background-color: #484F58;"
        "}"
        "QScrollBar::handle:horizontal:pressed {"
        "  background-color: #656D76;"
        "}"
        "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {"
        "  width: 0px;"
        "  border: none;"
        "  background: none;"
        "}"
        "QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal {"
        "  background: none;"
        "}"
    );
    
    // Hide cursor
    setCursorWidth(0);
    
    // Set margins
    setViewportMargins(0, 0, 0, 0);
    
    // Disable context menu
    setContextMenuPolicy(Qt::NoContextMenu);
}

void ResultsDisplay::setupConnections()
{
    connect(verticalScrollBar(), &QScrollBar::valueChanged,
            this, &ResultsDisplay::onVerticalScrollChanged);

    // Connect viewport update for line number area synchronization (QTextEdit doesn't have updateRequest)
    connect(this->verticalScrollBar(), &QScrollBar::valueChanged,
            this, [this]() { updateLineNumberArea(); });
    connect(this, &QTextEdit::textChanged,
            this, [this]() { updateLineNumberArea(); });
}

void ResultsDisplay::setResults(const QStringList &results)
{
    // Don't check m_isUpdating here - we want to allow updates from setResults
    m_results = results;
    updateContentForced();
}

void ResultsDisplay::setResult(int lineNumber, const QString &result)
{
    if (lineNumber >= 1 && lineNumber <= m_results.size()) {
        m_results[lineNumber - 1] = result;
        updateContent();
    }
}

void ResultsDisplay::clearResults()
{
    m_results.clear();
    clear();
}

int ResultsDisplay::getResultCount() const
{
    return m_results.size();
}

QStringList ResultsDisplay::getResults() const
{
    return m_results;
}

void ResultsDisplay::setDefaultFont()
{
    // Use Roboto Mono to match the HTML design, fallback to system monospace fonts
    m_defaultFont = QFont("Roboto Mono", m_baseFontSize);
    if (!m_defaultFont.exactMatch()) {
        m_defaultFont = QFont("Consolas", m_baseFontSize);
        if (!m_defaultFont.exactMatch()) {
            m_defaultFont = QFont("Courier New", m_baseFontSize);
        }
    }
    setFont(m_defaultFont);
}

void ResultsDisplay::increaseFontSize()
{
    QFont currentFont = font();
    int currentSize = currentFont.pixelSize(); // Use pixelSize instead of pointSize

    // If pixelSize returns -1, use a default pixel size
    if (currentSize <= 0) {
        currentSize = 12; // Default pixel size
    }

    int newSize = currentSize + 1;
    if (newSize <= 32) { // Maximum pixel size
        currentFont.setPixelSize(newSize);
        setFont(currentFont);

        // Update line number area font
        if (m_lineNumberArea) {
            m_lineNumberArea->setFont(currentFont);
            updateLineNumberAreaWidth();
        }
    }
}

void ResultsDisplay::decreaseFontSize()
{
    QFont currentFont = font();
    int currentSize = currentFont.pixelSize(); // Use pixelSize instead of pointSize

    // If pixelSize returns -1, use a default pixel size
    if (currentSize <= 0) {
        currentSize = 12; // Default pixel size
    }

    int newSize = currentSize - 1;
    if (newSize >= 8) { // Minimum pixel size
        currentFont.setPixelSize(newSize);
        setFont(currentFont);

        // Update line number area font
        if (m_lineNumberArea) {
            m_lineNumberArea->setFont(currentFont);
            updateLineNumberAreaWidth();
        }
    }
}

void ResultsDisplay::resetFontSize()
{
    QFont resetFont = m_defaultFont;
    resetFont.setPixelSize(12); // Reset to default pixel size
    setFont(resetFont);

    // Update line number area font
    if (m_lineNumberArea) {
        m_lineNumberArea->setFont(resetFont);
        updateLineNumberAreaWidth();
    }
}

void ResultsDisplay::synchronizeFontWith(const QFont &font)
{
    setFont(font);
}

void ResultsDisplay::updateLineCount(int lineCount)
{
    int oldLineCount = m_lineCount;
    int oldResultsSize = m_results.size();
    int oldDocumentLineCount = document()->blockCount();

    LOG_DEBUG(QString("=== ResultsDisplay::updateLineCount ==="));
    LOG_DEBUG(QString("  Input lineCount: %1").arg(lineCount));
    LOG_DEBUG(QString("  Old m_lineCount: %1").arg(oldLineCount));
    LOG_DEBUG(QString("  Old results size: %1").arg(oldResultsSize));
    LOG_DEBUG(QString("  Old document blockCount: %1").arg(oldDocumentLineCount));

    if (m_lineCount != lineCount) {
        m_lineCount = lineCount;
        LOG_DEBUG(QString("  Line count changed, updating from %1 to %2").arg(oldLineCount).arg(lineCount));

        // Ensure we have enough results for all lines
        while (m_results.size() < lineCount) {
            m_results.append("");
            // LOG_DEBUG(QString("    Added empty result, size now: %1").arg(m_results.size())); // Commented out - too verbose
        }

        // Remove excess results
        while (m_results.size() > lineCount) {
            m_results.removeLast();
            // LOG_DEBUG(QString("    Removed excess result, size now: %1").arg(m_results.size())); // Commented out - too verbose
        }

        LOG_DEBUG(QString("  Calling updateContent() with %1 results").arg(m_results.size()));
        updateContent();

        int newDocumentLineCount = document()->blockCount();
        LOG_DEBUG(QString("  After updateContent: document blockCount: %1").arg(newDocumentLineCount));
    } else {
        LOG_DEBUG(QString("  No change needed, lineCount already %1").arg(lineCount));
    }
    LOG_DEBUG(QString("=== END ResultsDisplay::updateLineCount ==="));
}

void ResultsDisplay::setLineHeight(int height)
{
    Q_UNUSED(height)
    // TODO: Implement line height synchronization if needed
}

void ResultsDisplay::updateContent()
{
    if (m_isUpdating) {
        return;
    }

    updateContentForced();
}

void ResultsDisplay::updateContentForced()
{
    m_isUpdating = true;

    // Save current scroll position and highlighting state
    int scrollPosition = verticalScrollBar()->value();
    int savedHighlightedLine = m_currentHighlightedLine;
    QString savedCurrentLineText = m_lastCurrentLineText; // Save the line text for LN reference highlighting

    LOG_DEBUG(QString("=== ResultsDisplay::updateContentForced ==="));
    LOG_DEBUG(QString("  Saving highlighted line: %1").arg(savedHighlightedLine));
    LOG_DEBUG(QString("  Saving current line text: '%1'").arg(savedCurrentLineText));

    // Build content string
    QString content = m_results.join('\n');

    // Set the content (this clears all highlighting)
    setPlainText(content);

    // Restore scroll position
    verticalScrollBar()->setValue(scrollPosition);

    // Restore full highlighting (including LN references) if we had a valid highlighted line
    if (savedHighlightedLine > 0 && savedHighlightedLine <= document()->blockCount()) {
        LOG_DEBUG(QString("  Restoring full highlighting to line: %1 with text: '%2'")
                  .arg(savedHighlightedLine).arg(savedCurrentLineText));
        // Use the full highlighting method to restore both current line and LN reference highlighting
        highlightCurrentLineWithLNReferences(savedHighlightedLine, savedCurrentLineText);
    } else {
        LOG_DEBUG(QString("  Not restoring highlighting (line %1 invalid for %2 blocks)")
                  .arg(savedHighlightedLine).arg(document()->blockCount()));
    }

    m_isUpdating = false;
    LOG_DEBUG(QString("=== END ResultsDisplay::updateContentForced ==="));
}

void ResultsDisplay::formatResults()
{
    // TODO: Implement syntax highlighting for results
    // This would include coloring numbers, errors, comments, etc.
}

bool ResultsDisplay::isCommentLine(int lineNumber) const
{
    // Check if the corresponding line in results is a comment
    // Comment lines show empty results and correspond to expression lines starting with ":::"
    if (lineNumber < 1 || lineNumber > m_results.size()) {
        return false;
    }

    // Get the result for this line (1-based to 0-based conversion)
    QString result = m_results[lineNumber - 1];

    // Comment lines typically show empty results or specific comment indicators
    // We'll need to coordinate with the expression editor to determine if a line is a comment
    // For now, we'll check if the result is empty (which is typical for comment lines)
    return result.trimmed().isEmpty();
}

// Line number area management (same as ExpressionEditor)
void ResultsDisplay::setLineNumberArea(LineNumberArea *lineNumberArea)
{
    m_lineNumberArea = lineNumberArea;
}

void ResultsDisplay::updateLineNumberAreaWidth()
{
    if (m_lineNumberArea) {
        // Don't set left margin - we want text to start at the left edge like expression editor
        setViewportMargins(0, 0, 0, 0);
    }
}

void ResultsDisplay::updateLineNumberArea(const QRect &rect, int dy)
{
    if (m_lineNumberArea) {
        if (dy) {
            m_lineNumberArea->scroll(0, dy);
        } else {
            m_lineNumberArea->update(0, rect.y(), m_lineNumberArea->width(), rect.height());
        }

        if (rect.contains(viewport()->rect())) {
            updateLineNumberAreaWidth();
        }
    }
}

void ResultsDisplay::updateLineNumberArea()
{
    if (m_lineNumberArea) {
        m_lineNumberArea->update();
    }
}

// Line number area support methods (expose protected methods like ExpressionEditor)
QTextBlock ResultsDisplay::getFirstVisibleBlock() const
{
    // For QTextEdit, we need to find the first visible block manually
    QTextBlock block = document()->begin();
    QScrollBar *vScrollBar = verticalScrollBar();
    int scrollValue = vScrollBar->value();

    // Find the first block that's visible in the viewport
    while (block.isValid()) {
        QRectF blockRect = document()->documentLayout()->blockBoundingRect(block);
        if (blockRect.bottom() > scrollValue) {
            return block;
        }
        block = block.next();
    }
    return document()->begin();
}

QRectF ResultsDisplay::getBlockBoundingGeometry(const QTextBlock &block) const
{
    // Use document layout to get block bounding rectangle
    return document()->documentLayout()->blockBoundingRect(block);
}

QPointF ResultsDisplay::getContentOffset() const
{
    // Calculate content offset based on scroll position
    QScrollBar *hScrollBar = horizontalScrollBar();
    QScrollBar *vScrollBar = verticalScrollBar();
    return QPointF(-hScrollBar->value(), -vScrollBar->value());
}

QRectF ResultsDisplay::getBlockBoundingRect(const QTextBlock &block) const
{
    // Same as getBlockBoundingGeometry for QTextEdit
    return document()->documentLayout()->blockBoundingRect(block);
}

int ResultsDisplay::getCurrentLineNumber() const
{
    // Return the currently highlighted line number (synchronized with ExpressionEditor)
    // This is used for line number area highlighting, not cursor position
    return m_currentHighlightedLine;
}

int ResultsDisplay::getLineCount() const
{
    return document()->blockCount();
}

void ResultsDisplay::wheelEvent(QWheelEvent *event)
{
    // Handle Ctrl+Wheel for font size (synchronized with expression editor)
    if (event->modifiers() & Qt::ControlModifier) {
        if (event->angleDelta().y() > 0) {
            increaseFontSize();
        } else {
            decreaseFontSize();
        }
        return;
    }
    
    // Handle normal wheel events
    QTextEdit::wheelEvent(event);
}

void ResultsDisplay::resizeEvent(QResizeEvent *event)
{
    QTextEdit::resizeEvent(event);

    // Update line number area geometry (like ExpressionEditor)
    if (m_lineNumberArea) {
        QRect cr = contentsRect();
        m_lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), m_lineNumberArea->getWidth(), cr.height()));
    }
}

void ResultsDisplay::onVerticalScrollChanged(int value)
{
    if (!m_isUpdating) {
        emit scrollRequested(value);
    }
}

// Current line highlighting methods
void ResultsDisplay::highlightCurrentLine(int lineNumber)
{
    if (!m_currentLineHighlightingEnabled || lineNumber < 1) {
        // Clear any existing extra selections if highlighting is disabled or invalid line
        setExtraSelections({});
        m_currentHighlightedLine = -1;
        return;
    }

    // Check if the line number is beyond our document's line count
    int documentLineCount = document()->blockCount();
    if (lineNumber > documentLineCount) {
        LOG_DEBUG(QString("ResultsDisplay: Line number %1 exceeds document line count %2, skipping highlighting")
                  .arg(lineNumber).arg(documentLineCount));
        // Clear highlighting since we can't highlight a line that doesn't exist
        setExtraSelections({});
        m_currentHighlightedLine = -1;
        return;
    }

    // Only update if the line has changed
    if (m_currentHighlightedLine == lineNumber) {
        return;
    }

    m_currentHighlightedLine = lineNumber;

    // Create extra selection for current line highlighting
    QList<QTextEdit::ExtraSelection> extraSelections;

    // Find the block for the specified line number (1-based to 0-based conversion)
    QTextBlock currentBlock = document()->findBlockByNumber(lineNumber - 1);
    if (currentBlock.isValid()) {
        QTextEdit::ExtraSelection selection;
        selection.format.setBackground(s_currentLineBackgroundColor);
        selection.format.setProperty(QTextCharFormat::FullWidthSelection, true);
        selection.cursor = QTextCursor(currentBlock);

        extraSelections.append(selection);
    }

    setExtraSelections(extraSelections);
}

void ResultsDisplay::highlightCurrentLineWithLNReferences(int lineNumber, const QString &currentLineText)
{
    // Comprehensive logging for debugging
    int documentLineCount = document()->blockCount();
    int resultsCount = m_results.size();
    int storedLineCount = m_lineCount;

    LOG_DEBUG(QString("=== ResultsDisplay::highlightCurrentLineWithLNReferences ==="));
    LOG_DEBUG(QString("  Input lineNumber: %1").arg(lineNumber));
    LOG_DEBUG(QString("  Input currentLineText: '%1'").arg(currentLineText));
    LOG_DEBUG(QString("  Document blockCount: %1").arg(documentLineCount));
    LOG_DEBUG(QString("  Results array size: %1").arg(resultsCount));
    LOG_DEBUG(QString("  Stored m_lineCount: %1").arg(storedLineCount));
    LOG_DEBUG(QString("  Current highlighted line: %1").arg(m_currentHighlightedLine));
    LOG_DEBUG(QString("  Highlighting enabled: %1").arg(m_currentLineHighlightingEnabled));

    if (!m_currentLineHighlightingEnabled || lineNumber < 1) {
        LOG_DEBUG("  RESULT: Clearing selections (highlighting disabled or invalid line)");
        // Clear any existing extra selections if highlighting is disabled or invalid line
        setExtraSelections({});
        m_currentHighlightedLine = -1;
        return;
    }

    // Check if the line number is beyond our document's line count
    if (lineNumber > documentLineCount) {
        LOG_DEBUG(QString("  RESULT: Line number %1 exceeds document line count %2, clearing highlighting")
                  .arg(lineNumber).arg(documentLineCount));
        // Clear highlighting since we can't highlight a line that doesn't exist
        setExtraSelections({});
        m_currentHighlightedLine = -1;
        return;
    }

    LOG_DEBUG(QString("  PROCEEDING: Setting m_currentHighlightedLine to %1").arg(lineNumber));
    m_currentHighlightedLine = lineNumber;
    m_lastCurrentLineText = currentLineText; // Save the current line text for restoration after document rebuilds

    QList<QTextEdit::ExtraSelection> extraSelections;

    // Check for LN references in the current line text
    QRegularExpression lnPattern(R"(\bLN(\d+)\b)", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatchIterator iterator = lnPattern.globalMatch(currentLineText);

    // Collect LN references, excluding cross-sheet references
    std::vector<int> referencedLNs;
    while (iterator.hasNext()) {
        QRegularExpressionMatch match = iterator.next();
        int lnNumber = match.captured(1).toInt();

        // Check if this LN is part of a cross-sheet reference (S.Sheet.LN1)
        bool isPartOfCrossSheet = false;
        int startPos = match.capturedStart();
        if (startPos >= 2) {
            QString beforeMatch = currentLineText.left(startPos);
            QRegularExpression crossSheetCheck(R"(S\.[^.]+\.$)", QRegularExpression::CaseInsensitiveOption);
            if (crossSheetCheck.match(beforeMatch).hasMatch()) {
                isPartOfCrossSheet = true;
                LOG_DEBUG(QString("LN%1 is part of cross-sheet reference, excluding from local highlighting").arg(lnNumber));
            } else {
                LOG_DEBUG(QString("LN%1 beforeMatch='%2' - NOT cross-sheet, including in local highlighting").arg(lnNumber).arg(beforeMatch));
            }
        }

        if (!isPartOfCrossSheet) {
            referencedLNs.push_back(lnNumber);
        }
    }

    LOG_DEBUG(QString("  Found %1 LN references: %2").arg(referencedLNs.size())
              .arg(referencedLNs.empty() ? "none" : QString::number(referencedLNs[0])));

    if (!referencedLNs.empty()) {
        // Highlight referenced LN lines with darker shades of their LN colors
        for (int lnNumber : referencedLNs) {
            // LOG_DEBUG(QString("  Processing LN reference: LN%1").arg(lnNumber)); // Commented out - too verbose

            // Find the line with this LN number (1-based line numbers)
            QTextBlock targetBlock = document()->findBlockByNumber(lnNumber - 1);
            bool blockValid = targetBlock.isValid();
            // LOG_DEBUG(QString("    Target block for line %1: %2").arg(lnNumber).arg(blockValid ? "VALID" : "INVALID")); // Commented out - too verbose

            if (blockValid) {
                // Calculate LN color (same logic as syntax highlighter)
                const QStringList colors = {
                    "#FF6B6B", "#4ECDC4", "#45B7D1", "#96CEB4", "#FFEAA7",
                    "#DDA0DD", "#98D8C8", "#F7DC6F", "#BB8FCE", "#85C1E9",
                    "#F8C471", "#82E0AA", "#F1948A", "#85CDFD", "#D7BDE2",
                    "#A9DFBF", "#F9E79F"
                };
                int colorIndex = (lnNumber - 1) % colors.size();
                QColor lnColor(colors[colorIndex]);

                // Create darker background color (reduce brightness by ~60%)
                QColor bgColor = lnColor.darker(250); // Make it darker
                bgColor.setAlpha(80); // Make it semi-transparent

                QTextEdit::ExtraSelection lnSelection;
                lnSelection.format.setBackground(bgColor);
                lnSelection.format.setProperty(QTextCharFormat::FullWidthSelection, true);
                lnSelection.cursor = QTextCursor(targetBlock);
                extraSelections.append(lnSelection);
            }
        }
    }

    // Add cross-sheet highlighting if present (before current line highlighting)
    if (m_crossSheetHighlightedLine > 0 && m_crossSheetHighlightedLine != lineNumber) {
        QTextBlock crossSheetBlock = document()->findBlockByNumber(m_crossSheetHighlightedLine - 1);
        if (crossSheetBlock.isValid()) {
            QTextEdit::ExtraSelection crossSheetSelection;
            // Use the stored LN color with reduced alpha for cross-sheet highlighting
            QColor crossSheetColor = m_crossSheetHighlightColor.isValid() ? m_crossSheetHighlightColor : s_currentLineBackgroundColor;
            crossSheetColor.setAlpha(64); // Semi-transparent for cross-sheet
            crossSheetSelection.format.setBackground(crossSheetColor);
            crossSheetSelection.format.setProperty(QTextCharFormat::FullWidthSelection, true);
            crossSheetSelection.cursor = QTextCursor(crossSheetBlock);
            extraSelections.append(crossSheetSelection);
        }
    }

    // Add current line highlight (always on top)
    QTextBlock currentBlock = document()->findBlockByNumber(lineNumber - 1);
    bool currentBlockValid = currentBlock.isValid();
    LOG_DEBUG(QString("  Current line block for line %1: %2").arg(lineNumber).arg(currentBlockValid ? "VALID" : "INVALID"));

    if (currentBlockValid) {
        QTextEdit::ExtraSelection currentLineSelection;
        currentLineSelection.format.setBackground(s_currentLineBackgroundColor);
        currentLineSelection.format.setProperty(QTextCharFormat::FullWidthSelection, true);
        currentLineSelection.cursor = QTextCursor(currentBlock);

        extraSelections.append(currentLineSelection);
        // LOG_DEBUG(QString("  Added current line selection for line %1").arg(lineNumber)); // Commented out - too verbose
    } else {
        LOG_DEBUG(QString("  FAILED to add current line selection - block invalid for line %1").arg(lineNumber));
    }

    LOG_DEBUG(QString("  FINAL: Setting %1 extra selections").arg(extraSelections.size()));
    setExtraSelections(extraSelections);
    LOG_DEBUG(QString("=== END highlightCurrentLineWithLNReferences ==="));
}

void ResultsDisplay::highlightCurrentLineFromEditor(ExpressionEditor* editor)
{
    if (!editor) {
        LOG_DEBUG("ResultsDisplay::highlightCurrentLineFromEditor: No editor provided");
        return;
    }

    // Get the current line number and text directly from the editor's cursor
    // This ensures we always highlight the correct line, even after document changes
    int currentLine = editor->getCurrentLineNumber();
    QString currentLineText = editor->textCursor().block().text();

    LOG_DEBUG(QString("=== ResultsDisplay::highlightCurrentLineFromEditor ==="));
    LOG_DEBUG(QString("  Editor current line: %1").arg(currentLine));
    LOG_DEBUG(QString("  Editor current line text: '%1'").arg(currentLineText));

    // Use the enhanced method with the editor for consistent LN color retrieval
    highlightCurrentLineWithLNReferencesFromEditor(currentLine, currentLineText, editor);

    LOG_DEBUG(QString("=== END highlightCurrentLineFromEditor ==="));
}

void ResultsDisplay::highlightCurrentLineWithLNReferencesFromEditor(int lineNumber, const QString &currentLineText, ExpressionEditor* editor)
{
    LOG_DEBUG(QString("=== ResultsDisplay::highlightCurrentLineWithLNReferencesFromEditor ==="));
    LOG_DEBUG(QString("  Line number: %1").arg(lineNumber));
    LOG_DEBUG(QString("  Line text: '%1'").arg(currentLineText));

    if (lineNumber < 1) {
        LOG_DEBUG("  EARLY EXIT: Invalid line number");
        return;
    }

    LOG_DEBUG(QString("  PROCEEDING: Setting m_currentHighlightedLine to %1").arg(lineNumber));
    m_currentHighlightedLine = lineNumber;
    m_lastCurrentLineText = currentLineText; // Save the current line text for restoration after document rebuilds

    QList<QTextEdit::ExtraSelection> extraSelections;

    // Check for LN references in the current line text
    QRegularExpression lnPattern(R"(\bLN(\d+)\b)", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatchIterator iterator = lnPattern.globalMatch(currentLineText);

    // Collect LN references, excluding cross-sheet references
    std::vector<int> referencedLNs;
    while (iterator.hasNext()) {
        QRegularExpressionMatch match = iterator.next();
        int lnNumber = match.captured(1).toInt();

        // Check if this LN is part of a cross-sheet reference (S.Sheet.LN1)
        bool isPartOfCrossSheet = false;
        int startPos = match.capturedStart();
        if (startPos >= 2) {
            QString beforeMatch = currentLineText.left(startPos);
            QRegularExpression crossSheetCheck(R"(S\.[^.]+\.$)", QRegularExpression::CaseInsensitiveOption);
            if (crossSheetCheck.match(beforeMatch).hasMatch()) {
                isPartOfCrossSheet = true;
                LOG_DEBUG(QString("LN%1 is part of cross-sheet reference, excluding from local highlighting").arg(lnNumber));
            } else {
                LOG_DEBUG(QString("LN%1 beforeMatch='%2' - NOT cross-sheet, including in local highlighting").arg(lnNumber).arg(beforeMatch));
            }
        }

        if (!isPartOfCrossSheet) {
            referencedLNs.push_back(lnNumber);
        }
    }

    LOG_DEBUG(QString("  Found %1 LN references: %2").arg(referencedLNs.size())
              .arg(referencedLNs.empty() ? "none" : QString::number(referencedLNs[0])));

    if (!referencedLNs.empty()) {
        // Highlight referenced LN lines with darker shades of their LN colors
        for (int lnNumber : referencedLNs) {
            // LOG_DEBUG(QString("  Processing LN reference: LN%1").arg(lnNumber)); // Commented out - too verbose

            // Find the line with this LN number (1-based line numbers)
            QTextBlock targetBlock = document()->findBlockByNumber(lnNumber - 1);
            bool blockValid = targetBlock.isValid();
            // LOG_DEBUG(QString("    Target block for line %1: %2").arg(lnNumber).arg(blockValid ? "VALID" : "INVALID")); // Commented out - too verbose

            if (blockValid) {
                // Get LN color from the ExpressionEditor's syntax highlighter for consistency
                QColor lnColor;
                if (editor && editor->getSyntaxHighlighter()) {
                    lnColor = editor->getSyntaxHighlighter()->getLNColor(lnNumber);
                    // LOG_DEBUG(QString("    Got LN color from syntax highlighter: %1").arg(lnColor.name())); // Commented out - too verbose
                } else {
                    // Fallback to hardcoded colors if no syntax highlighter available
                    const QStringList colors = {
                        "#FF6B6B", "#4ECDC4", "#45B7D1", "#96CEB4", "#FFEAA7",
                        "#DDA0DD", "#98D8C8", "#F7DC6F", "#BB8FCE", "#85C1E9",
                        "#F8C471", "#82E0AA", "#F1948A", "#85CDFD", "#D7BDE2",
                        "#A9DFBF", "#F9E79F"
                    };
                    int colorIndex = (lnNumber - 1) % colors.size();
                    lnColor = QColor(colors[colorIndex]);
                    // LOG_DEBUG(QString("    Using fallback LN color: %1").arg(lnColor.name())); // Commented out - too verbose
                }

                // Create darker background color (reduce brightness by ~60%)
                QColor bgColor = lnColor.darker(250); // Make it darker
                bgColor.setAlpha(80); // Make it semi-transparent

                QTextEdit::ExtraSelection lnSelection;
                lnSelection.format.setBackground(bgColor);
                lnSelection.format.setProperty(QTextCharFormat::FullWidthSelection, true);
                lnSelection.cursor = QTextCursor(targetBlock);
                extraSelections.append(lnSelection);
            }
        }
    }

    // Add cross-sheet highlighting if present (before current line highlighting)
    if (m_crossSheetHighlightedLine > 0 && m_crossSheetHighlightedLine != lineNumber) {
        QTextBlock crossSheetBlock = document()->findBlockByNumber(m_crossSheetHighlightedLine - 1);
        if (crossSheetBlock.isValid()) {
            QTextEdit::ExtraSelection crossSheetSelection;
            // Use the stored LN color with reduced alpha for cross-sheet highlighting
            QColor crossSheetColor = m_crossSheetHighlightColor.isValid() ? m_crossSheetHighlightColor : s_currentLineBackgroundColor;
            crossSheetColor.setAlpha(64); // Semi-transparent for cross-sheet
            crossSheetSelection.format.setBackground(crossSheetColor);
            crossSheetSelection.format.setProperty(QTextCharFormat::FullWidthSelection, true);
            crossSheetSelection.cursor = QTextCursor(crossSheetBlock);
            extraSelections.append(crossSheetSelection);
        }
    }

    // Add current line highlighting (after LN highlighting so it appears on top)
    if (m_currentLineHighlightingEnabled) {
        QTextBlock currentBlock = document()->findBlockByNumber(lineNumber - 1);
        if (currentBlock.isValid()) {
            QTextEdit::ExtraSelection currentLineSelection;
            currentLineSelection.format.setBackground(s_currentLineBackgroundColor);
            currentLineSelection.format.setProperty(QTextCharFormat::FullWidthSelection, true);
            currentLineSelection.cursor = QTextCursor(currentBlock);
            extraSelections.append(currentLineSelection);
        }
    }

    LOG_DEBUG(QString("  FINAL: Setting %1 extra selections").arg(extraSelections.size()));
    setExtraSelections(extraSelections);
    LOG_DEBUG(QString("=== END highlightCurrentLineWithLNReferencesFromEditor ==="));
}

void ResultsDisplay::highlightSpecificLine(int lineNumber, const QColor &lnColor)
{
    // Simple method to highlight a specific line (for cross-sheet highlighting)
    if (lineNumber < 1) {
        return;
    }

    // Check if the line number is beyond our document's line count
    int documentLineCount = document()->blockCount();
    if (lineNumber > documentLineCount) {
        LOG_DEBUG(QString("ResultsDisplay: Cross-sheet line number %1 exceeds document line count %2, skipping highlighting")
                  .arg(lineNumber).arg(documentLineCount));
        return;
    }

    // Store the cross-sheet highlighted line and color for persistence
    m_crossSheetHighlightedLine = lineNumber;
    m_crossSheetHighlightColor = lnColor;

    // Refresh the current highlighting to include the cross-sheet highlight
    // This ensures proper coordination with existing highlights
    if (m_currentLineHighlightingEnabled && m_currentHighlightedLine > 0) {
        // Get current line text to refresh all highlighting properly
        QTextBlock currentBlock = document()->findBlockByNumber(m_currentHighlightedLine - 1);
        if (currentBlock.isValid()) {
            QString currentLineText = currentBlock.text();
            highlightCurrentLineWithLNReferences(m_currentHighlightedLine, currentLineText);
        }
    } else {
        // No current line highlighting, just show the cross-sheet highlight
        QList<QTextEdit::ExtraSelection> extraSelections;

        // Find the block for the specified line number (1-based to 0-based conversion)
        QTextBlock targetBlock = document()->findBlockByNumber(lineNumber - 1);
        if (targetBlock.isValid()) {
            QTextEdit::ExtraSelection selection;
            // Use the LN color with reduced alpha for cross-sheet highlighting
            QColor crossSheetColor = lnColor;
            crossSheetColor.setAlpha(64); // Semi-transparent background highlighting
            selection.format.setBackground(crossSheetColor);
            selection.format.setProperty(QTextCharFormat::FullWidthSelection, true);
            selection.cursor = QTextCursor(targetBlock);

            extraSelections.append(selection);
        }

        setExtraSelections(extraSelections);
    }
}

void ResultsDisplay::clearCrossSheetHighlighting()
{
    // Clear cross-sheet highlighting and refresh current highlighting
    m_crossSheetHighlightedLine = -1;
    m_crossSheetHighlightColor = QColor(); // Clear the stored color

    // Refresh current highlighting to remove cross-sheet highlights
    if (m_currentLineHighlightingEnabled && m_currentHighlightedLine > 0) {
        // Get current line text to refresh LN highlighting
        QTextBlock currentBlock = document()->findBlockByNumber(m_currentHighlightedLine - 1);
        if (currentBlock.isValid()) {
            QString currentLineText = currentBlock.text();
            highlightCurrentLineWithLNReferences(m_currentHighlightedLine, currentLineText);
        }
    }
}

void ResultsDisplay::setCurrentLineHighlightingEnabled(bool enabled)
{
    if (m_currentLineHighlightingEnabled != enabled) {
        m_currentLineHighlightingEnabled = enabled;

        // Update highlighting immediately
        if (enabled && m_currentHighlightedLine > 0) {
            highlightCurrentLine(m_currentHighlightedLine);
        } else {
            setExtraSelections({});
        }
    }
}

bool ResultsDisplay::isCurrentLineHighlightingEnabled() const noexcept
{
    return m_currentLineHighlightingEnabled;
}
