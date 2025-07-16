#include "ResultsDisplay.h"
#include "LineNumberArea.h"
#include "Logger.h"
#include <QScrollBar>
#include <QWheelEvent>
#include <QTextBlock>
#include <QAbstractTextDocumentLayout>

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
    if (m_lineCount != lineCount) {
        m_lineCount = lineCount;
        
        // Ensure we have enough results for all lines
        while (m_results.size() < lineCount) {
            m_results.append("");
        }
        
        // Remove excess results
        while (m_results.size() > lineCount) {
            m_results.removeLast();
        }
        
        updateContent();
    }
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

    // Save current scroll position
    int scrollPosition = verticalScrollBar()->value();

    // Build content string
    QString content = m_results.join('\n');

    // Set the content
    setPlainText(content);

    // Restore scroll position
    verticalScrollBar()->setValue(scrollPosition);

    m_isUpdating = false;
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
    return textCursor().blockNumber() + 1;
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

    // Only update if the line has changed
    if (m_currentHighlightedLine == lineNumber) {
        return;
    }

    m_currentHighlightedLine = lineNumber;

    // Create extra selection for current line highlighting
    QList<QTextEdit::ExtraSelection> extraSelections;

    // Find the block for the specified line number (1-based to 0-based conversion)
    QTextBlock block = document()->findBlockByNumber(lineNumber - 1);
    if (block.isValid()) {
        QTextEdit::ExtraSelection selection;
        selection.format.setBackground(s_currentLineBackgroundColor);
        selection.format.setProperty(QTextCharFormat::FullWidthSelection, true);
        selection.cursor = QTextCursor(block);

        extraSelections.append(selection);
    }

    setExtraSelections(extraSelections);
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
