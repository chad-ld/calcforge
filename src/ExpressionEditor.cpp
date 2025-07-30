#include "ExpressionEditor.h"
#include "LineNumberArea.h"
#include "SyntaxHighlighter.h"
#include "Logger.h"
#include "MainWindow.h"
#include "AutoCompleteWidget.h"
#include "WorksheetWidget.h"
#include <QTextBlock>
#include <QScrollBar>
#include <QApplication>
#include <QFontMetrics>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QCoreApplication>
#include <QDir>
#include <QAbstractTextDocumentLayout>
#include <QRegularExpression>

// Static color definition for current line highlighting (matches Python version)
const QColor ExpressionEditor::s_currentLineBackgroundColor = QColor(65, 65, 66);
#include <algorithm>
#include <vector>
#include <array>
#include <QClipboard>

// Simple logging function for debugging
void logEditorDebug(const QString &message) {
    static QString logPath;
    if (logPath.isEmpty()) {
        QString appDir = QCoreApplication::applicationDirPath();
        QDir dir(appDir);
        if (!dir.exists("logs")) {
            dir.mkpath("logs");
        }
        logPath = dir.absoluteFilePath("logs/font_debug.log");
    }

    QFile logFile(logPath);
    if (logFile.open(QIODevice::WriteOnly | QIODevice::Append)) {
        QTextStream stream(&logFile);
        stream << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz")
               << " - [EDITOR] " << message << "\n";
        logFile.close();
    }
}

ExpressionEditor::ExpressionEditor(QWidget *parent)
    : QTextEdit(parent)
    , m_lineNumberArea(nullptr)
    , m_syntaxHighlighter(nullptr)
    , m_syntaxHighlightingEnabled(true)
    , m_currentLineHighlightingEnabled(true)
    , m_crossSheetHighlightedLine(-1)
    , m_crossSheetHighlightColor()
    , m_baseFontSize(15)  // Increased from 10 to 15 (5 steps larger)
    , m_lastLineCount(0)
    , m_isUpdating(false)
    , m_tooltipsEnabled(true)
    , m_operatorHighlightingEnabled(true)
    , m_currentOperatorAbsolutePosition(-1)
    , m_currentOperatorAbsoluteBounds(-1, -1)
    , m_currentOperatorChar()
    , m_autoCompleteManager(nullptr)
    , m_autoCompleteEnabled(true)
{
    setupEditor();
    setupConnections();

    // Initialize autocomplete manager
    LOG_DEBUG("ExpressionEditor: Creating AutoCompleteManager");
    m_autoCompleteManager = new AutoCompleteManager(this, this);
    LOG_DEBUG(QString("ExpressionEditor: AutoCompleteManager created successfully: %1").arg(m_autoCompleteManager != nullptr));
}

ExpressionEditor::~ExpressionEditor()
{
    // Cleanup autocomplete manager
    if (m_autoCompleteManager) {
        delete m_autoCompleteManager;
        m_autoCompleteManager = nullptr;
    }

    // Syntax highlighter is automatically deleted by Qt when document is destroyed
}

void ExpressionEditor::setupEditor()
{
    // Set default font
    setDefaultFont();
    
    // Configure editor properties
    setLineWrapMode(QTextEdit::NoWrap);
    setTabStopDistance(40);

    // Always show horizontal scrollbar for consistency
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    
    // Set background and text colors to EXACTLY match ResultsDisplay with Material Design flat scrollbars
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
    
    // Configure cursor
    setCursorWidth(2);

    // Set margins
    setViewportMargins(0, 0, 0, 0);

    // Initialize syntax highlighting
    m_syntaxHighlighter = new SyntaxHighlighter(document());
    setSyntaxHighlightingEnabled(m_syntaxHighlightingEnabled);

    // Connect cursor position changes to current line highlighting
    connect(this, &QTextEdit::cursorPositionChanged, this, &ExpressionEditor::highlightCurrentLine);

    // Initial current line highlighting
    highlightCurrentLine();
}

void ExpressionEditor::setupConnections()
{
    connect(this, &QTextEdit::textChanged, this, &ExpressionEditor::onTextChanged);
    connect(this, &QTextEdit::cursorPositionChanged, this, &ExpressionEditor::onCursorPositionChanged);

    // Connect viewport update for line number area synchronization (QTextEdit doesn't have updateRequest)
    connect(this->verticalScrollBar(), &QScrollBar::valueChanged,
            this, [this]() { updateLineNumberArea(); });
    connect(this, &QTextEdit::textChanged,
            this, [this]() { updateLineNumberArea(); });
}

void ExpressionEditor::setLineNumberArea(LineNumberArea *lineNumberArea)
{
    m_lineNumberArea = lineNumberArea;
    updateLineNumberAreaWidth();
}

void ExpressionEditor::updateLineNumberAreaWidth()
{
    if (m_lineNumberArea) {
        // Don't set left margin - we want text to start at the left edge like results
        setViewportMargins(0, 0, 0, 0);
    }
}

void ExpressionEditor::updateLineNumberArea(const QRect &rect, int dy)
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

void ExpressionEditor::updateLineNumberArea()
{
    if (m_lineNumberArea) {
        m_lineNumberArea->update();
    }
}

void ExpressionEditor::setDefaultFont()
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
    
    // Update line number area font if available
    if (m_lineNumberArea) {
        m_lineNumberArea->setFont(m_defaultFont);
    }
}

void ExpressionEditor::increaseFontSize()
{
    QFont currentFont = font();
    int currentSize = currentFont.pixelSize(); // Use pixelSize instead of pointSize

    logEditorDebug(QString("ExpressionEditor::increaseFontSize() - Current font: %1, pointSize: %2, pixelSize: %3")
                  .arg(currentFont.family()).arg(currentFont.pointSize()).arg(currentSize));

    // If pixelSize returns -1, use a default pixel size
    if (currentSize <= 0) {
        currentSize = 12; // Default pixel size
        logEditorDebug(QString("pixelSize was %1, using default size %2").arg(currentFont.pixelSize()).arg(currentSize));
    }

    int newSize = currentSize + 1;
    if (newSize <= 32) { // Maximum pixel size
        currentFont.setPixelSize(newSize);
        setFont(currentFont);

        logEditorDebug(QString("Set new pixel size to %1").arg(newSize));

        // Verify the font was actually set
        QFont verifyFont = font();
        logEditorDebug(QString("After setting font - pointSize: %1, pixelSize: %2")
                      .arg(verifyFont.pointSize()).arg(verifyFont.pixelSize()));

        if (m_lineNumberArea) {
            m_lineNumberArea->setFont(currentFont);
            updateLineNumberAreaWidth();
        }
    } else {
        logEditorDebug(QString("Font size %1 exceeds maximum (32)").arg(newSize));
    }
}

void ExpressionEditor::decreaseFontSize()
{
    QFont currentFont = font();
    int currentSize = currentFont.pixelSize(); // Use pixelSize instead of pointSize

    logEditorDebug(QString("ExpressionEditor::decreaseFontSize() - Current pixelSize: %1").arg(currentSize));

    // If pixelSize returns -1, use a default pixel size
    if (currentSize <= 0) {
        currentSize = 12; // Default pixel size
    }

    int newSize = currentSize - 1;
    if (newSize >= 8) { // Minimum pixel size
        currentFont.setPixelSize(newSize);
        setFont(currentFont);

        logEditorDebug(QString("Set new pixel size to %1").arg(newSize));

        if (m_lineNumberArea) {
            m_lineNumberArea->setFont(currentFont);
            updateLineNumberAreaWidth();
        }
    } else {
        logEditorDebug(QString("Font size %1 below minimum (8)").arg(newSize));
    }
}

void ExpressionEditor::resetFontSize()
{
    QFont resetFont = m_defaultFont;
    resetFont.setPixelSize(17); // Reset to default pixel size (15 + 2 for pixel adjustment)
    setFont(resetFont);

    logEditorDebug("Reset font to default pixel size 17");

    if (m_lineNumberArea) {
        m_lineNumberArea->setFont(resetFont);
        updateLineNumberAreaWidth();
    }
}

int ExpressionEditor::getLineCount() const
{
    return document()->blockCount();
}

QString ExpressionEditor::getLineText(int lineNumber) const
{
    QTextBlock block = document()->findBlockByLineNumber(lineNumber - 1);
    return block.isValid() ? block.text() : QString();
}

void ExpressionEditor::setLineText(int lineNumber, const QString &text)
{
    QTextBlock block = document()->findBlockByLineNumber(lineNumber - 1);
    if (block.isValid()) {
        QTextCursor cursor(block);
        cursor.select(QTextCursor::LineUnderCursor);
        cursor.insertText(text);
    }
}

void ExpressionEditor::positionCursorAtEnd()
{
    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::End);
    setTextCursor(cursor);
}

void ExpressionEditor::positionCursorAtStart()
{
    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::Start);
    setTextCursor(cursor);
}

int ExpressionEditor::getCurrentLineNumber() const
{
    return textCursor().blockNumber() + 1;
}

QTextBlock ExpressionEditor::getFirstVisibleBlock() const
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

QRectF ExpressionEditor::getBlockBoundingGeometry(const QTextBlock &block) const
{
    // Use document layout to get block bounding rectangle
    return document()->documentLayout()->blockBoundingRect(block);
}

QPointF ExpressionEditor::getContentOffset() const
{
    // Calculate content offset based on scroll position
    QScrollBar *hScrollBar = horizontalScrollBar();
    QScrollBar *vScrollBar = verticalScrollBar();
    return QPointF(-hScrollBar->value(), -vScrollBar->value());
}

QRectF ExpressionEditor::getBlockBoundingRect(const QTextBlock &block) const
{
    // Same as getBlockBoundingGeometry for QTextEdit
    return document()->documentLayout()->blockBoundingRect(block);
}

void ExpressionEditor::keyPressEvent(QKeyEvent *event)
{
    // Font size shortcuts are now handled by MainWindow, so we don't need detailed logging here
    // Just handle normal text editing

    LOG_DEBUG(QString("ExpressionEditor::keyPressEvent: key=%1, text='%2', printable=%3")
              .arg(event->key()).arg(event->text()).arg(!event->text().isEmpty() && event->text().at(0).isPrint()));

    // Handle Ctrl key combinations
    if (event->modifiers() & Qt::ControlModifier) {
        // Handle cross-sheet navigation shortcuts first
        if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) {
            // Ctrl+Enter: Navigate to cross-sheet reference
            handleCrossSheetNavigation();
            event->accept();
            return;
        } else if (event->key() == Qt::Key_Backspace) {
            // Ctrl+Backspace: Return to previous cross-sheet reference
            handleCrossSheetReturn();
            event->accept();
            return;
        }
        // Handle smart navigation shortcuts
        else if (event->key() == Qt::Key_Left) {
            jumpToPreviousNumberOrLN();
            event->accept();
            return;
        } else if (event->key() == Qt::Key_Right) {
            jumpToNextNumberOrLN();
            event->accept();
            return;
        } else if (event->key() == Qt::Key_Down) {
            selectCurrentLine();
            event->accept();
            return;
        } else if (event->key() == Qt::Key_Up) {
            smartParenthesesSelection();
            event->accept();
            return;
        } else if (event->key() == Qt::Key_C) {
            // Handle Ctrl+C: Copy result if no selection, otherwise use default copy
            handleCopyShortcut();
            event->accept();
            return;
        }

        logEditorDebug(QString("CTRL COMBO: Ctrl+%1 (key code: %2, text: '%3')")
                      .arg(QKeySequence(event->key()).toString()).arg(event->key()).arg(event->text()));

        // Log specific key checks with actual key code values
        logEditorDebug(QString("Key comparison: event->key()=%1, Qt::Key_Plus=%2, Qt::Key_Minus=%3, Qt::Key_Equal=%4, Qt::Key_0=%5")
                      .arg(event->key()).arg(Qt::Key_Plus).arg(Qt::Key_Minus).arg(Qt::Key_Equal).arg(Qt::Key_0));

        // DISABLED: Let MainWindow shortcuts handle this instead
        /*
        // Check for font size increase keys (Plus, Equal, or any key that produces '+' or '=' text)
        if (event->key() == Qt::Key_Plus || event->key() == Qt::Key_Equal ||
            event->text() == "+" || event->text() == "=") {
            logEditorDebug("Emitting fontSizeIncreaseRequested signal");
            emit fontSizeIncreaseRequested();
            event->accept();
            return;
        }
        // Check for font size decrease keys (Minus, Underscore, key code 173, or any key that produces '-' or '_' text)
        else if (event->key() == Qt::Key_Minus || event->key() == Qt::Key_Underscore ||
                 event->key() == 173 || event->text() == "-" || event->text() == "_") {
            logEditorDebug(QString("Emitting fontSizeDecreaseRequested signal (key code: %1, text: '%2')").arg(event->key()).arg(event->text()));
            emit fontSizeDecreaseRequested();
            event->accept();
            return;
        }
        // Check for font size reset key (0 or any key that produces '0' text)
        else if (event->key() == Qt::Key_0 || event->text() == "0") {
            logEditorDebug("Emitting fontSizeResetRequested signal");
            emit fontSizeResetRequested();
            event->accept();
            return;
        }
        */
    }

    // Handle autocomplete key events
    if (m_autoCompleteEnabled && m_autoCompleteManager) {
        if (m_autoCompleteManager->isVisible()) {
            // Autocomplete is visible - let it handle navigation keys
            if (event->key() == Qt::Key_Up || event->key() == Qt::Key_Down ||
                event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter ||
                event->key() == Qt::Key_Tab || event->key() == Qt::Key_Escape) {
                m_autoCompleteManager->handleKeyPress(event);
                if (event->isAccepted()) {
                    return;
                }
            }
        }
    }

    // Handle Enter key specially to preserve scroll position and highlighting
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        handleEnterKey(event);
        return;
    }

    // Handle other key events
    QTextEdit::keyPressEvent(event);

    // Trigger autocomplete after text changes (for typing)
    if (m_autoCompleteEnabled && m_autoCompleteManager && !event->text().isEmpty()) {
        // Only trigger for printable characters
        if (event->text().at(0).isPrint()) {
            LOG_DEBUG(QString("ExpressionEditor: Triggering autocomplete for text: '%1'").arg(event->text()));
            m_autoCompleteManager->handleTextChanged();
        } else {
            LOG_DEBUG(QString("ExpressionEditor: Skipping autocomplete for non-printable character: %1").arg(event->key()));
        }
    } else {
        LOG_DEBUG(QString("ExpressionEditor: Autocomplete not triggered - enabled: %1, manager: %2, text: '%3'")
                  .arg(m_autoCompleteEnabled).arg(m_autoCompleteManager != nullptr).arg(event->text()));
    }
}

void ExpressionEditor::handleEnterKey(QKeyEvent *event)
{
    LOG_DEBUG("ExpressionEditor::handleEnterKey() called");

    // Save current vertical scroll position (preserve vertical scrolling)
    QScrollBar *vScrollBar = verticalScrollBar();
    QScrollBar *hScrollBar = horizontalScrollBar();
    int savedVScrollPos = vScrollBar->value();

    // Save current cursor position
    QTextCursor cursor = textCursor();
    int cursorPosition = cursor.position();

    LOG_DEBUG(QString("  Saved vertical scroll position: V=%1").arg(savedVScrollPos));
    LOG_DEBUG(QString("  Saved cursor position: %1").arg(cursorPosition));

    // Let QTextEdit handle the Enter key normally
    QTextEdit::keyPressEvent(event);

    // Restore vertical scroll position and reset horizontal scroll to left
    vScrollBar->setValue(savedVScrollPos);
    hScrollBar->setValue(0); // Reset horizontal scroll to leftmost position

    // Force current line highlighting update
    // The cursor should now be on the new line after the Enter
    highlightCurrentLine();

    LOG_DEBUG(QString("  Restored vertical scroll: V=%1, Reset horizontal scroll to: H=0").arg(savedVScrollPos));
    LOG_DEBUG(QString("  New cursor position: %1").arg(textCursor().position()));
}

void ExpressionEditor::wheelEvent(QWheelEvent *event)
{
    // Handle Ctrl+Wheel for font size
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

void ExpressionEditor::resizeEvent(QResizeEvent *event)
{
    QTextEdit::resizeEvent(event);

    if (m_lineNumberArea) {
        QRect cr = contentsRect();
        m_lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), m_lineNumberArea->getWidth(), cr.height()));
    }
}

void ExpressionEditor::mouseMoveEvent(QMouseEvent *event)
{
    if (m_tooltipsEnabled || m_operatorHighlightingEnabled) {
        // Get the text position under the mouse cursor
        QTextCursor cursor = cursorForPosition(event->pos());
        int absoluteTextPosition = cursor.position();

        // Get the current line text for both tooltip and highlighting
        QTextBlock block = cursor.block();
        QString lineText = block.text();
        int positionInLine = absoluteTextPosition - block.position();

        if (m_tooltipsEnabled) {
            // Show tooltip for the position
            showTooltipForPosition(event->globalPosition().toPoint(), absoluteTextPosition);
        }

        if (m_operatorHighlightingEnabled) {
            // Update operator highlighting with absolute position
            updateOperatorHighlighting(lineText, positionInLine, absoluteTextPosition, block.position());
        }
    }

    // Call parent implementation for normal mouse handling
    QTextEdit::mouseMoveEvent(event);
}

void ExpressionEditor::leaveEvent(QEvent *event)
{
    // Hide tooltip when mouse leaves the editor
    QToolTip::hideText();

    // Clear operator highlighting when mouse leaves the editor
    if (m_operatorHighlightingEnabled) {
        clearOperatorHighlighting();
    }

    QTextEdit::leaveEvent(event);
}

void ExpressionEditor::onTextChanged()
{
    if (m_isUpdating) {
        return;
    }

    LOG_DEBUG("ExpressionEditor::onTextChanged() called");

    // Trigger autocomplete for text changes (only if not updating programmatically)
    if (m_autoCompleteEnabled && m_autoCompleteManager) {
        LOG_DEBUG("ExpressionEditor: Triggering autocomplete from onTextChanged");
        m_autoCompleteManager->handleTextChanged();
    }

    int currentLineCount = getLineCount();
    if (currentLineCount != m_lastLineCount) {
        m_lastLineCount = currentLineCount;
        emit lineCountChanged();

        if (m_lineNumberArea) {
            m_lineNumberArea->updateWidth();
            updateLineNumberAreaWidth();
        }
    }

    LOG_DEBUG("ExpressionEditor: Emitting contentChanged signal");
    emit contentChanged();
}

void ExpressionEditor::onCursorPositionChanged()
{
    if (m_lineNumberArea) {
        m_lineNumberArea->update();
    }

    // Update current line highlighting including LN reference backgrounds
    highlightCurrentLine();
}

void ExpressionEditor::updateViewport()
{
    if (m_lineNumberArea) {
        updateLineNumberArea(viewport()->rect(), 0);
    }
}

void ExpressionEditor::jumpToPreviousNumberOrLN()
{
    QTextCursor cursor = textCursor();
    QString text = toPlainText();
    int currentPos = cursor.position();

    // Get current line only (don't cross line boundaries)
    QTextBlock currentBlock = cursor.block();
    QString lineText = currentBlock.text();
    int lineStart = currentBlock.position();
    int posInLine = currentPos - lineStart;

    // Regular expressions for numbers and LN references
    QRegularExpression numberRegex(R"(-?\d+\.?\d*)");
    QRegularExpression lnRegex(R"(\bLN\d+\b)", QRegularExpression::CaseInsensitiveOption);

    // Find all matches in the current line
    QList<QPair<int, int>> matches; // position, length pairs

    // Find numbers
    QRegularExpressionMatchIterator numberIterator = numberRegex.globalMatch(lineText);
    while (numberIterator.hasNext()) {
        QRegularExpressionMatch match = numberIterator.next();
        matches.append(QPair<int, int>(match.capturedStart(), match.capturedLength()));
    }

    // Find LN references
    QRegularExpressionMatchIterator lnIterator = lnRegex.globalMatch(lineText);
    while (lnIterator.hasNext()) {
        QRegularExpressionMatch match = lnIterator.next();
        matches.append(QPair<int, int>(match.capturedStart(), match.capturedLength()));
    }

    // Sort matches by position
    std::sort(matches.begin(), matches.end(), [](const QPair<int, int> &a, const QPair<int, int> &b) {
        return a.first < b.first;
    });

    // Find the previous match
    int targetStart = -1, targetLength = 0;

    // First check if cursor is inside any match - if so, select that match first
    for (int i = 0; i < matches.size(); ++i) {
        int matchStart = matches[i].first;
        int matchEnd = matchStart + matches[i].second;

        if (posInLine >= matchStart && posInLine <= matchEnd) {
            // Cursor is inside this match - check if it's already fully selected
            QTextCursor currentCursor = textCursor();
            if (currentCursor.hasSelection() &&
                currentCursor.selectionStart() == lineStart + matchStart &&
                currentCursor.selectionEnd() == lineStart + matchEnd) {
                // Already selected, move to previous match
                if (i > 0) {
                    targetStart = matches[i-1].first;
                    targetLength = matches[i-1].second;
                }
            } else {
                // Not selected, select current match
                targetStart = matchStart;
                targetLength = matches[i].second;
            }
            break;
        }
    }

    // If no match found above, look for previous match
    if (targetStart == -1) {
        for (int i = matches.size() - 1; i >= 0; --i) {
            int matchStart = matches[i].first;
            int matchEnd = matchStart + matches[i].second;

            // If cursor is after this match, select it
            if (posInLine > matchEnd) {
                targetStart = matchStart;
                targetLength = matches[i].second;
                break;
            }
        }
    }

    // If we found a target, select it
    if (targetStart >= 0) {
        cursor.setPosition(lineStart + targetStart);
        cursor.setPosition(lineStart + targetStart + targetLength, QTextCursor::KeepAnchor);
        setTextCursor(cursor);
    }
}

void ExpressionEditor::jumpToNextNumberOrLN()
{
    QTextCursor cursor = textCursor();
    QString text = toPlainText();
    int currentPos = cursor.position();

    // Get current line only (don't cross line boundaries)
    QTextBlock currentBlock = cursor.block();
    QString lineText = currentBlock.text();
    int lineStart = currentBlock.position();
    int posInLine = currentPos - lineStart;

    // Regular expressions for numbers and LN references
    QRegularExpression numberRegex(R"(-?\d+\.?\d*)");
    QRegularExpression lnRegex(R"(\bLN\d+\b)", QRegularExpression::CaseInsensitiveOption);

    // Find all matches in the current line
    QList<QPair<int, int>> matches; // position, length pairs

    // Find numbers
    QRegularExpressionMatchIterator numberIterator = numberRegex.globalMatch(lineText);
    while (numberIterator.hasNext()) {
        QRegularExpressionMatch match = numberIterator.next();
        matches.append(QPair<int, int>(match.capturedStart(), match.capturedLength()));
    }

    // Find LN references
    QRegularExpressionMatchIterator lnIterator = lnRegex.globalMatch(lineText);
    while (lnIterator.hasNext()) {
        QRegularExpressionMatch match = lnIterator.next();
        matches.append(QPair<int, int>(match.capturedStart(), match.capturedLength()));
    }

    // Sort matches by position
    std::sort(matches.begin(), matches.end(), [](const QPair<int, int> &a, const QPair<int, int> &b) {
        return a.first < b.first;
    });

    // Find the next match
    int targetStart = -1, targetLength = 0;

    // First check if cursor is inside any match - if so, select that match first
    for (int i = 0; i < matches.size(); ++i) {
        int matchStart = matches[i].first;
        int matchEnd = matchStart + matches[i].second;

        if (posInLine >= matchStart && posInLine <= matchEnd) {
            // Cursor is inside this match - check if it's already fully selected
            QTextCursor currentCursor = textCursor();
            if (currentCursor.hasSelection() &&
                currentCursor.selectionStart() == lineStart + matchStart &&
                currentCursor.selectionEnd() == lineStart + matchEnd) {
                // Already selected, move to next match
                if (i < matches.size() - 1) {
                    targetStart = matches[i+1].first;
                    targetLength = matches[i+1].second;
                }
            } else {
                // Not selected, select current match
                targetStart = matchStart;
                targetLength = matches[i].second;
            }
            break;
        }
    }

    // If no match found above, look for next match
    if (targetStart == -1) {
        for (int i = 0; i < matches.size(); ++i) {
            int matchStart = matches[i].first;
            int matchEnd = matchStart + matches[i].second;

            // If cursor is before this match, select it
            if (posInLine < matchStart) {
                targetStart = matchStart;
                targetLength = matches[i].second;
                break;
            }
        }
    }

    // If we found a target, select it
    if (targetStart >= 0) {
        cursor.setPosition(lineStart + targetStart);
        cursor.setPosition(lineStart + targetStart + targetLength, QTextCursor::KeepAnchor);
        setTextCursor(cursor);
    }
}

void ExpressionEditor::handleCopyShortcut()
{
    QTextCursor cursor = textCursor();

    // If there's a selection, use default copy behavior
    if (cursor.hasSelection()) {
        copy(); // Use QTextEdit's built-in copy functionality
        return;
    }

    // No selection - copy the result from the current line
    int currentLineNumber = cursor.blockNumber() + 1; // Convert to 1-based line number

    // Get the parent WorksheetWidget to access results
    QWidget *parent = parentWidget();
    while (parent && !parent->inherits("WorksheetWidget")) {
        parent = parent->parentWidget();
    }

    if (!parent) {
        logEditorDebug("Could not find WorksheetWidget parent for copy operation");
        return;
    }

    // Try to get the results display from the WorksheetWidget
    QObject *resultsDisplay = parent->findChild<QObject*>("ResultsDisplay");
    if (!resultsDisplay) {
        // Try alternative approach - look for any QTextEdit that might be the results
        QList<QTextEdit*> textEdits = parent->findChildren<QTextEdit*>();
        for (QTextEdit *edit : textEdits) {
            if (edit != this) { // Not the expression editor
                resultsDisplay = edit;
                break;
            }
        }
    }

    if (resultsDisplay) {
        // Cast to QTextEdit to access the document
        QTextEdit *resultsEdit = qobject_cast<QTextEdit*>(resultsDisplay);
        if (resultsEdit) {
            QTextDocument *resultsDoc = resultsEdit->document();
            QTextBlock resultBlock = resultsDoc->findBlockByNumber(currentLineNumber - 1); // Convert back to 0-based

            if (resultBlock.isValid()) {
                QString resultText = resultBlock.text().trimmed();
                if (!resultText.isEmpty()) {
                    // Copy the result to clipboard
                    QClipboard *clipboard = QApplication::clipboard();
                    clipboard->setText(resultText);

                    logEditorDebug(QString("Copied result from line %1: '%2'").arg(currentLineNumber).arg(resultText));
                    return;
                }
            }
        }
    }

    logEditorDebug(QString("No result found to copy for line %1").arg(currentLineNumber));
}

void ExpressionEditor::selectCurrentLine()
{
    logEditorDebug("selectCurrentLine() called");

    QTextCursor cursor = textCursor();

    // Move to beginning of current line
    cursor.movePosition(QTextCursor::StartOfLine);

    // Select to end of line
    cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);

    // Set the selection
    setTextCursor(cursor);

    logEditorDebug("Selected current line");
}

void ExpressionEditor::smartParenthesesSelection()
{
    logEditorDebug("smartParenthesesSelection() called");

    QTextCursor cursor = textCursor();
    QString text = toPlainText();
    int cursorPos = cursor.position();

    // Get current line boundaries
    QTextCursor lineCursor = cursor;
    lineCursor.movePosition(QTextCursor::StartOfLine);
    int lineStart = lineCursor.position();
    lineCursor.movePosition(QTextCursor::EndOfLine);
    int lineEnd = lineCursor.position();

    // If we already have a selection, try to expand it
    if (cursor.hasSelection()) {
        int selStart = cursor.selectionStart();
        int selEnd = cursor.selectionEnd();

        // Check if current selection is inside parentheses that we can expand
        if (selStart > lineStart && selEnd < lineEnd &&
            text[selStart - 1] == '(' && text[selEnd] == ')') {
            // Current selection is content inside parentheses, expand to include the parentheses
            cursor.setPosition(selStart - 1);
            cursor.setPosition(selEnd + 1, QTextCursor::KeepAnchor);
            setTextCursor(cursor);
            logEditorDebug("Expanded selection to include parentheses");
            return;
        }

        // Try to find next outer level of parentheses within the current line
        int outerStart = selStart - 1;
        int outerEnd = selEnd + 1;

        // Find opening parenthesis before current selection (but not before line start)
        while (outerStart >= lineStart && text[outerStart] != '(') {
            outerStart--;
        }

        // Find closing parenthesis after current selection (but not after line end)
        while (outerEnd <= lineEnd && text[outerEnd] != ')') {
            outerEnd++;
        }

        if (outerStart >= lineStart && outerEnd <= lineEnd &&
            text[outerStart] == '(' && text[outerEnd] == ')') {
            // Select content inside the outer parentheses
            cursor.setPosition(outerStart + 1);
            cursor.setPosition(outerEnd, QTextCursor::KeepAnchor);
            setTextCursor(cursor);
            logEditorDebug("Expanded to next outer parentheses level within current line");
            return;
        }
    }

    // No selection or can't expand - find innermost parentheses around cursor within current line
    int openPos = -1;
    int closePos = -1;

    // Find the innermost opening parenthesis before cursor (but not before line start)
    for (int i = cursorPos - 1; i >= lineStart; i--) {
        if (text[i] == ')') {
            // Found closing paren before cursor, skip this level
            int depth = 1;
            i--;
            while (i >= lineStart && depth > 0) {
                if (text[i] == ')') depth++;
                else if (text[i] == '(') depth--;
                i--;
            }
        } else if (text[i] == '(') {
            openPos = i;
            break;
        }
    }

    // Find the matching closing parenthesis after cursor (but not after line end)
    if (openPos >= 0) {
        int depth = 1;
        for (int i = openPos + 1; i <= lineEnd; i++) {
            if (text[i] == '(') {
                depth++;
            } else if (text[i] == ')') {
                depth--;
                if (depth == 0) {
                    closePos = i;
                    break;
                }
            }
        }
    }

    if (openPos >= 0 && closePos >= 0 && openPos >= lineStart && closePos <= lineEnd) {
        // Select content inside parentheses (not including the parentheses themselves)
        cursor.setPosition(openPos + 1);
        cursor.setPosition(closePos, QTextCursor::KeepAnchor);
        setTextCursor(cursor);
        logEditorDebug(QString("Selected content inside parentheses within current line: positions %1 to %2").arg(openPos + 1).arg(closePos));
    } else {
        logEditorDebug("No parentheses found around cursor within current line");
    }
}

// Syntax highlighting methods
void ExpressionEditor::setSyntaxHighlightingEnabled(bool enabled)
{
    m_syntaxHighlightingEnabled = enabled;

    if (m_syntaxHighlighter) {
        if (enabled) {
            // Re-enable highlighting by triggering a rehighlight
            m_syntaxHighlighter->rehighlight();
        } else {
            // Disable highlighting by setting a null highlighter
            // Note: We keep the object but disconnect it from the document
            m_syntaxHighlighter->setDocument(nullptr);
        }
    }
}

bool ExpressionEditor::isSyntaxHighlightingEnabled() const
{
    return m_syntaxHighlightingEnabled;
}

void ExpressionEditor::setColorBlindMode(bool enabled)
{
    if (m_syntaxHighlighter) {
        m_syntaxHighlighter->setColorBlindMode(enabled);
    }
}

bool ExpressionEditor::isColorBlindMode() const
{
    if (m_syntaxHighlighter) {
        return m_syntaxHighlighter->isColorBlindMode();
    }
    return false;
}

// Current line highlighting methods
void ExpressionEditor::highlightCurrentLine()
{
    if (!m_currentLineHighlightingEnabled) {
        // Clear any existing extra selections if highlighting is disabled
        setExtraSelections({});
        return;
    }

    QList<QTextEdit::ExtraSelection> extraSelections;

    // Add cross-sheet highlighting if present (before current line highlighting)
    const int currentLineNumber = getCurrentLineNumber();
    if (m_crossSheetHighlightedLine > 0 && m_crossSheetHighlightedLine != currentLineNumber) {
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

    // Use C++ strengths: const correctness and efficient algorithms
    const QString currentLineText = textCursor().block().text();

    // Static regex for performance (compiled once)
    static const QRegularExpression lnPattern(R"(\bLN(\d+)\b)",
                                              QRegularExpression::CaseInsensitiveOption);

    // Use STL-style container for efficient LN number collection
    std::vector<int> referencedLNs;
    referencedLNs.reserve(4); // Reserve space for typical number of references

    // Collect all LN references efficiently
    QRegularExpressionMatchIterator iterator = lnPattern.globalMatch(currentLineText);
    while (iterator.hasNext()) {
        const QRegularExpressionMatch match = iterator.next();
        const int lnNumber = match.captured(1).toInt();

        // Check if this LN is part of a cross-sheet reference (S.Sheet.LN1)
        // Look backwards from the match position to see if there's "S.something."
        bool isPartOfCrossSheet = false;
        int startPos = match.capturedStart();
        if (startPos >= 2) {
            QString beforeMatch = currentLineText.left(startPos);
            QRegularExpression crossSheetCheck(R"(S\.[^.]+\.$)", QRegularExpression::CaseInsensitiveOption);
            if (crossSheetCheck.match(beforeMatch).hasMatch()) {
                isPartOfCrossSheet = true;
            }
        }

        if (!isPartOfCrossSheet) {
            referencedLNs.push_back(lnNumber);
        }
    }

    // Process LN references if any found
    if (!referencedLNs.empty()) {

        // Pre-allocate selections for better performance
        extraSelections.reserve(extraSelections.size() + referencedLNs.size() + 1);

        // Use range-based for loop (C++11 strength)
        for (const int lnNumber : referencedLNs) {
            // Find the line with this LN number (1-based line numbers)
            const QTextBlock targetBlock = document()->findBlockByNumber(lnNumber - 1);
            if (!targetBlock.isValid()) {
                continue; // Skip invalid blocks
            }

            // Get the LN color efficiently
            QColor lnColor;
            if (m_syntaxHighlighter) {
                lnColor = m_syntaxHighlighter->getLNColor(lnNumber);
            } else {
                // Fallback with static const array for performance
                static const std::array<QColor, 17> fallbackColors = {{
                    QColor("#FF6B6B"), QColor("#4ECDC4"), QColor("#45B7D1"), QColor("#96CEB4"),
                    QColor("#FFEAA7"), QColor("#DDA0DD"), QColor("#98D8C8"), QColor("#F7DC6F"),
                    QColor("#BB8FCE"), QColor("#85C1E9"), QColor("#F8C471"), QColor("#82E0AA"),
                    QColor("#F1948A"), QColor("#85CDFD"), QColor("#D7BDE2"), QColor("#A9DFBF"),
                    QColor("#F9E79F")
                }};
                const int colorIndex = (lnNumber - 1) % fallbackColors.size();
                lnColor = fallbackColors[colorIndex];
            }

            // Create darker background color using efficient color manipulation
            QColor bgColor = lnColor.darker(250); // Make it darker
            bgColor.setAlpha(80); // Make it semi-transparent

            // Create selection with move semantics for efficiency
            QTextEdit::ExtraSelection lnSelection;
            lnSelection.format.setBackground(bgColor);
            lnSelection.format.setProperty(QTextCharFormat::FullWidthSelection, true);
            lnSelection.cursor = QTextCursor(targetBlock);
            extraSelections.append(std::move(lnSelection));
        }
    }

    // Add current line highlight (medium priority)
    QTextEdit::ExtraSelection currentLineSelection;
    currentLineSelection.format.setBackground(s_currentLineBackgroundColor);
    currentLineSelection.format.setProperty(QTextCharFormat::FullWidthSelection, true);
    currentLineSelection.cursor = textCursor();

    // Clear selection to highlight the entire line
    if (!currentLineSelection.cursor.hasSelection()) {
        currentLineSelection.cursor.clearSelection();
    }

    extraSelections.append(currentLineSelection);

    // Add operator highlighting (highest priority - most specific)
    if (m_operatorHighlightingEnabled && m_currentOperatorAbsolutePosition != -1 &&
        m_currentOperatorAbsoluteBounds.first != -1 && m_currentOperatorAbsoluteBounds.second != -1) {

        // Create cursor for the operator expression bounds using absolute positions
        QTextCursor operatorCursor = textCursor();
        operatorCursor.setPosition(m_currentOperatorAbsoluteBounds.first);
        operatorCursor.setPosition(m_currentOperatorAbsoluteBounds.second + 1, QTextCursor::KeepAnchor);

        // Determine if the operator is on the current line for color selection
        const int currentLineNumber = getCurrentLineNumber();
        QTextBlock operatorBlock = document()->findBlock(m_currentOperatorAbsolutePosition);
        bool isOnCurrentLine = (operatorBlock.blockNumber() + 1) == currentLineNumber;

        // Create operator highlighting selection
        QTextEdit::ExtraSelection operatorSelection;
        QColor operatorColor = getOperatorHighlightColor(m_currentOperatorChar, isOnCurrentLine);
        operatorSelection.format.setBackground(operatorColor);
        operatorSelection.cursor = operatorCursor;

        extraSelections.append(operatorSelection);
    }

    setExtraSelections(extraSelections);
}

void ExpressionEditor::highlightSpecificLine(int lineNumber, const QColor &lnColor)
{
    // Simple method to highlight a specific line (for cross-sheet highlighting)
    if (lineNumber < 1) {
        return;
    }

    // Store the cross-sheet highlighted line and color for persistence
    m_crossSheetHighlightedLine = lineNumber;
    m_crossSheetHighlightColor = lnColor;

    // Get current extra selections and add cross-sheet highlighting
    QList<QTextEdit::ExtraSelection> extraSelections = this->extraSelections();

    // Find the block for the specified line number (1-based to 0-based conversion)
    QTextBlock targetBlock = document()->findBlockByNumber(lineNumber - 1);
    if (targetBlock.isValid()) {
        QTextEdit::ExtraSelection selection;

        // Use the LN color with reduced alpha for cross-sheet highlighting
        QColor crossSheetColor = lnColor.isValid() ? lnColor : s_currentLineBackgroundColor;
        crossSheetColor.setAlpha(64); // Semi-transparent background highlighting
        selection.format.setBackground(crossSheetColor);
        selection.format.setProperty(QTextCharFormat::FullWidthSelection, true);
        selection.cursor = QTextCursor(targetBlock);

        extraSelections.append(selection);
    }

    setExtraSelections(extraSelections);
}

void ExpressionEditor::clearCrossSheetHighlighting()
{
    // Clear cross-sheet highlighting and refresh current highlighting
    m_crossSheetHighlightedLine = -1;
    m_crossSheetHighlightColor = QColor(); // Clear the stored color

    // Refresh current highlighting to remove cross-sheet highlights
    if (m_currentLineHighlightingEnabled) {
        highlightCurrentLine();
    }
}

void ExpressionEditor::setCurrentLineHighlightingEnabled(bool enabled)
{
    if (m_currentLineHighlightingEnabled != enabled) {
        m_currentLineHighlightingEnabled = enabled;

        // Update highlighting immediately
        highlightCurrentLine();
    }
}

bool ExpressionEditor::isCurrentLineHighlightingEnabled() const noexcept
{
    return m_currentLineHighlightingEnabled;
}

void ExpressionEditor::handleCrossSheetNavigation()
{
    LOG_DEBUG("=== ExpressionEditor::handleCrossSheetNavigation ===");

    // Get current cursor position and line text
    QTextCursor cursor = textCursor();
    int cursorPosition = cursor.positionInBlock();
    QString lineText = cursor.block().text();
    int lineNumber = getCurrentLineNumber();

    LOG_DEBUG(QString("  Current line %1: '%2'").arg(lineNumber).arg(lineText));
    LOG_DEBUG(QString("  Cursor position in line: %1").arg(cursorPosition));

    // Find cross-sheet references in the current line
    QRegularExpression crossSheetPattern(R"(\bS\.([^.]+)\.LN(\d+)\b)", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatchIterator iterator = crossSheetPattern.globalMatch(lineText);

    // Find the cross-sheet reference at or near the cursor position
    QString targetSheetName;
    int targetLineNumber = -1;
    int referenceStart = -1;

    while (iterator.hasNext()) {
        QRegularExpressionMatch match = iterator.next();
        int matchStart = match.capturedStart();
        int matchEnd = match.capturedEnd();

        LOG_DEBUG(QString("  Found cross-sheet reference: '%1' at positions %2-%3")
                  .arg(match.captured(0)).arg(matchStart).arg(matchEnd));

        // Check if cursor is within this cross-sheet reference
        if (cursorPosition >= matchStart && cursorPosition <= matchEnd) {
            targetSheetName = match.captured(1);
            targetLineNumber = match.captured(2).toInt();
            referenceStart = matchStart;

            LOG_DEBUG(QString("  Cursor is within cross-sheet reference: S.%1.LN%2")
                      .arg(targetSheetName).arg(targetLineNumber));
            break;
        }
    }

    if (targetSheetName.isEmpty() || targetLineNumber <= 0) {
        LOG_DEBUG("  No cross-sheet reference found at cursor position");
        return;
    }

    // Save current position for return navigation
    // Get the current sheet name from the parent WorksheetWidget
    QWidget *parent = parentWidget();
    while (parent && !qobject_cast<class WorksheetWidget*>(parent)) {
        parent = parent->parentWidget();
    }

    if (!parent) {
        LOG_DEBUG("  Could not find parent WorksheetWidget");
        return;
    }

    // Get the current sheet name from MainWindow
    QWidget *mainWindowWidget = parent;
    while (mainWindowWidget && !qobject_cast<QMainWindow*>(mainWindowWidget)) {
        mainWindowWidget = mainWindowWidget->parentWidget();
    }

    class MainWindow *mainWindow = qobject_cast<class MainWindow*>(mainWindowWidget);
    if (!mainWindow) {
        LOG_DEBUG("  Could not find MainWindow");
        return;
    }

    // Get current sheet name (we'll need to add a method to get this)
    QString currentSheetName = getCurrentSheetName(mainWindow);

    // Save navigation history in MainWindow (global)
    mainWindow->saveNavigationHistory(currentSheetName, lineNumber, referenceStart);

    LOG_DEBUG(QString("  Saved navigation history in MainWindow: sheet='%1', line=%2, position=%3")
              .arg(currentSheetName).arg(lineNumber).arg(referenceStart));

    // Navigate to target sheet and line
    navigateToSheet(mainWindow, targetSheetName, targetLineNumber);

    LOG_DEBUG("=== END ExpressionEditor::handleCrossSheetNavigation ===");
}

void ExpressionEditor::handleCrossSheetReturn()
{
    LOG_DEBUG("=== ExpressionEditor::handleCrossSheetReturn ===");

    // Get MainWindow
    QWidget *parent = parentWidget();
    while (parent && !qobject_cast<QMainWindow*>(parent)) {
        parent = parent->parentWidget();
    }

    class MainWindow *mainWindow = qobject_cast<class MainWindow*>(parent);
    if (!mainWindow) {
        LOG_DEBUG("  Could not find MainWindow");
        return;
    }

    // Check if MainWindow has navigation history
    if (!mainWindow->hasNavigationHistory()) {
        LOG_DEBUG("  No navigation history available in MainWindow");
        return;
    }

    // Use MainWindow's return method
    mainWindow->returnToPreviousLocation();

    LOG_DEBUG("=== END ExpressionEditor::handleCrossSheetReturn ===");
}

QString ExpressionEditor::getCurrentSheetName(class MainWindow *mainWindow) const
{
    if (mainWindow) {
        return mainWindow->getCurrentSheetName();
    }
    return QString();
}

void ExpressionEditor::navigateToSheet(class MainWindow *mainWindow, const QString &sheetName, int lineNumber, int cursorPosition)
{
    if (mainWindow) {
        mainWindow->navigateToSheet(sheetName, lineNumber, cursorPosition);
    }
}

// Tooltip functionality methods
void ExpressionEditor::setTooltipsEnabled(bool enabled)
{
    m_tooltipsEnabled = enabled;
    if (!enabled) {
        QToolTip::hideText();
    }
}

bool ExpressionEditor::areTooltipsEnabled() const
{
    return m_tooltipsEnabled;
}

void ExpressionEditor::setAutoCompleteEnabled(bool enabled)
{
    m_autoCompleteEnabled = enabled;
    if (!enabled && m_autoCompleteManager) {
        m_autoCompleteManager->hideAutocomplete();
    }
}

bool ExpressionEditor::isAutoCompleteEnabled() const
{
    return m_autoCompleteEnabled;
}

void ExpressionEditor::showTooltipForPosition(const QPoint &globalPos, int textPosition)
{
    // Get the current line text
    QTextCursor cursor = textCursor();
    cursor.setPosition(textPosition);
    QTextBlock block = cursor.block();
    QString lineText = block.text();
    int positionInLine = textPosition - block.position();

    // Check for LN variable tooltip first
    QString lnTooltip = getLNVariableTooltip(lineText, positionInLine);
    if (!lnTooltip.isEmpty()) {
        QToolTip::showText(globalPos, lnTooltip, this);
        return;
    }

    // Check for operator tooltip
    QString operatorTooltip = getOperatorTooltip(lineText, positionInLine);
    if (!operatorTooltip.isEmpty()) {
        QToolTip::showText(globalPos, operatorTooltip, this);
        return;
    }

    // No tooltip to show, hide any existing tooltip
    QToolTip::hideText();
}

QString ExpressionEditor::getLNVariableTooltip(const QString &text, int position)
{
    // Check for cross-sheet references first: S.SheetName.LN2
    QRegularExpression crossSheetRegex(R"(\bS\.([^.]+)\.LN(\d+)\b)", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatchIterator crossSheetIterator = crossSheetRegex.globalMatch(text);

    while (crossSheetIterator.hasNext()) {
        QRegularExpressionMatch match = crossSheetIterator.next();
        int start = match.capturedStart();
        int end = match.capturedEnd();

        if (position >= start && position < end) {
            QString sheetName = match.captured(1);
            int lineNumber = match.captured(2).toInt();

            // Get the value from the cross-sheet reference
            // We need to access the MainWindow to get cross-sheet values
            QWidget *mainWindowWidget = this;
            while (mainWindowWidget && !qobject_cast<class MainWindow*>(mainWindowWidget)) {
                mainWindowWidget = mainWindowWidget->parentWidget();
            }

            if (mainWindowWidget) {
                class MainWindow *mainWindow = qobject_cast<class MainWindow*>(mainWindowWidget);
                if (mainWindow) {
                    double value = mainWindow->getCrossSheetValue(sheetName, lineNumber);
                    if (!std::isnan(value)) {
                        return QString::number(value);
                    } else {
                        return QString("not found");
                    }
                }
            }

            return QString("?");
        }
    }

    // Check for regular LN references: LN1, LN2, etc.
    QRegularExpression lnRegex(R"(\bLN(\d+)\b)", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatchIterator lnIterator = lnRegex.globalMatch(text);

    while (lnIterator.hasNext()) {
        QRegularExpressionMatch match = lnIterator.next();
        int start = match.capturedStart();
        int end = match.capturedEnd();

        if (position >= start && position < end) {
            int lineNumber = match.captured(1).toInt();

            // Get the value from the calculation engine
            // We need to access the WorksheetWidget to get the calculation engine
            QWidget *worksheetWidget = this;
            while (worksheetWidget && !qobject_cast<class WorksheetWidget*>(worksheetWidget)) {
                worksheetWidget = worksheetWidget->parentWidget();
            }

            if (worksheetWidget) {
                class WorksheetWidget *worksheet = qobject_cast<class WorksheetWidget*>(worksheetWidget);
                if (worksheet) {
                    double value = worksheet->getLineValue(lineNumber);
                    if (!std::isnan(value)) {
                        return QString::number(value);
                    } else {
                        return QString("not found");
                    }
                }
            }

            return QString("?");
        }
    }

    return QString(); // No LN variable found at this position
}

QString ExpressionEditor::getOperatorTooltip(const QString &text, int position)
{
    // Check if we're hovering over an operator
    if (position >= text.length()) {
        return QString();
    }

    QChar ch = text[position];
    if (ch != '+' && ch != '-' && ch != '*' && ch != '/' && ch != '^') {
        return QString(); // Not an operator
    }

    // Find the sub-expression around this operator
    QString subExprResult = evaluateSubExpression(text, position);
    if (!subExprResult.isEmpty()) {
        return subExprResult;
    }

    return QString();
}

QString ExpressionEditor::evaluateSubExpression(const QString &expression, int operatorPos)
{
    // For complex expressions like "20-(5*(10-8))", we want to show the result of the specific operation
    // the user is hovering over. This requires a more sophisticated approach.

    QChar op = expression[operatorPos];

    // Find the immediate left and right operands for this specific operator
    QString leftOperand = extractLeftOperand(expression, operatorPos);
    QString rightOperand = extractRightOperand(expression, operatorPos);

    if (leftOperand.isEmpty() || rightOperand.isEmpty()) {
        return QString(); // Could not determine operands
    }

    // Create a simple expression with just these operands and the operator
    QString simpleExpr = QString("%1%2%3").arg(leftOperand).arg(op).arg(rightOperand);

    // Get the calculation engine to evaluate the simple expression
    QWidget *worksheetWidget = this;
    while (worksheetWidget && !qobject_cast<class WorksheetWidget*>(worksheetWidget)) {
        worksheetWidget = worksheetWidget->parentWidget();
    }

    if (worksheetWidget) {
        class WorksheetWidget *worksheet = qobject_cast<class WorksheetWidget*>(worksheetWidget);
        if (worksheet) {
            // Use the calculation engine to evaluate the simple expression
            QString result = worksheet->evaluateExpression(simpleExpr);

            // Extract numeric value from result
            bool ok;
            double numericValue = result.toDouble(&ok);
            if (ok) {
                return QString::number(numericValue);
            } else {
                // Try to extract number from result string (e.g., "123 miles" -> "123")
                QRegularExpression numberRegex(R"([-+]?\d*\.?\d+)");
                QRegularExpressionMatch match = numberRegex.match(result);
                if (match.hasMatch()) {
                    return match.captured(0);
                }
            }
        }
    }

    return QString();
}

QString ExpressionEditor::extractLeftOperand(const QString &expression, int operatorPos)
{
    int pos = operatorPos - 1;

    // Skip whitespace
    while (pos >= 0 && expression[pos].isSpace()) {
        pos--;
    }

    if (pos < 0) return QString();

    // Check if we have a closing parenthesis - need to find matching opening parenthesis
    if (expression[pos] == ')') {
        int parenLevel = 1;
        int endPos = pos;
        pos--;

        while (pos >= 0 && parenLevel > 0) {
            if (expression[pos] == ')') {
                parenLevel++;
            } else if (expression[pos] == '(') {
                parenLevel--;
            }
            pos--;
        }

        if (parenLevel == 0) {
            return expression.mid(pos + 1, endPos - pos);
        } else {
            return QString(); // Unmatched parentheses
        }
    }

    // Extract a simple number or identifier
    int endPos = pos;
    while (pos >= 0 && (expression[pos].isDigit() || expression[pos] == '.' ||
                        expression[pos].isLetter() || expression[pos] == '_')) {
        pos--;
    }

    return expression.mid(pos + 1, endPos - pos);
}

QString ExpressionEditor::extractRightOperand(const QString &expression, int operatorPos)
{
    int pos = operatorPos + 1;

    // Skip whitespace
    while (pos < expression.length() && expression[pos].isSpace()) {
        pos++;
    }

    if (pos >= expression.length()) return QString();

    int startPos = pos;

    // Check if we have an opening parenthesis - need to find matching closing parenthesis
    if (expression[pos] == '(') {
        int parenLevel = 1;
        pos++;

        while (pos < expression.length() && parenLevel > 0) {
            if (expression[pos] == '(') {
                parenLevel++;
            } else if (expression[pos] == ')') {
                parenLevel--;
            }
            pos++;
        }

        if (parenLevel == 0) {
            return expression.mid(startPos, pos - startPos);
        } else {
            return QString(); // Unmatched parentheses
        }
    }

    // Extract a simple number or identifier
    while (pos < expression.length() && (expression[pos].isDigit() || expression[pos] == '.' ||
                                         expression[pos].isLetter() || expression[pos] == '_')) {
        pos++;
    }

    return expression.mid(startPos, pos - startPos);
}

QPair<int, int> ExpressionEditor::findSubExpressionBounds(const QString &text, int operatorPos)
{
    // This is a simplified approach - find the immediate operands around the operator
    // For a more sophisticated approach, we would need to parse the full expression tree

    int leftStart = operatorPos - 1;
    int rightEnd = operatorPos + 1;

    // Find the left operand (scan backwards)
    while (leftStart >= 0) {
        QChar ch = text[leftStart];
        if (ch.isDigit() || ch == '.' || ch == ')') {
            leftStart--;
        } else if (ch == '(') {
            // Found opening parenthesis, this is our left bound
            break;
        } else if (ch.isSpace()) {
            leftStart--;
        } else {
            // Found another operator or invalid character, stop here
            leftStart++;
            break;
        }
    }

    if (leftStart < 0) leftStart = 0;

    // Find the right operand (scan forwards)
    while (rightEnd < text.length()) {
        QChar ch = text[rightEnd];
        if (ch.isDigit() || ch == '.' || ch == '(') {
            rightEnd++;
        } else if (ch == ')') {
            // Found closing parenthesis, include it and stop
            break;
        } else if (ch.isSpace()) {
            rightEnd++;
        } else {
            // Found another operator or invalid character, stop here
            rightEnd--;
            break;
        }
    }

    if (rightEnd >= text.length()) rightEnd = text.length() - 1;

    // Handle parentheses properly
    int parenCount = 0;
    int adjustedLeftStart = leftStart;
    int adjustedRightEnd = rightEnd;

    // Scan for balanced parentheses around our operator
    for (int i = leftStart; i <= rightEnd; i++) {
        if (text[i] == '(') {
            parenCount++;
            if (i < operatorPos) adjustedLeftStart = i;
        } else if (text[i] == ')') {
            parenCount--;
            if (i > operatorPos) adjustedRightEnd = i;
        }
    }

    // For complex expressions like "20-(5*(10-8))", we want to find the immediate operation
    // around the operator we're hovering over

    // Simple case: just find the immediate operands
    int simpleLeft = operatorPos - 1;
    int simpleRight = operatorPos + 1;

    // Skip whitespace
    while (simpleLeft >= 0 && text[simpleLeft].isSpace()) simpleLeft--;
    while (simpleRight < text.length() && text[simpleRight].isSpace()) simpleRight++;

    // Expand to include full numbers/expressions
    while (simpleLeft >= 0 && (text[simpleLeft].isDigit() || text[simpleLeft] == '.')) simpleLeft--;
    while (simpleRight < text.length() && (text[simpleRight].isDigit() || text[simpleRight] == '.')) simpleRight++;

    // Handle parentheses
    if (simpleLeft >= 0 && text[simpleLeft] == ')') {
        // Find matching opening parenthesis
        int parenLevel = 1;
        simpleLeft--;
        while (simpleLeft >= 0 && parenLevel > 0) {
            if (text[simpleLeft] == ')') parenLevel++;
            else if (text[simpleLeft] == '(') parenLevel--;
            simpleLeft--;
        }
        simpleLeft++; // Include the opening parenthesis
    }

    if (simpleRight < text.length() && text[simpleRight] == '(') {
        // Find matching closing parenthesis
        int parenLevel = 1;
        simpleRight++;
        while (simpleRight < text.length() && parenLevel > 0) {
            if (text[simpleRight] == '(') parenLevel++;
            else if (text[simpleRight] == ')') parenLevel--;
            simpleRight++;
        }
        simpleRight--; // Include the closing parenthesis
    }

    simpleLeft++; // Adjust to include the first character
    simpleRight--; // Adjust to include the last character

    return QPair<int, int>(simpleLeft, simpleRight);
}

// Operator highlighting functionality
void ExpressionEditor::setOperatorHighlightingEnabled(bool enabled)
{
    if (m_operatorHighlightingEnabled != enabled) {
        m_operatorHighlightingEnabled = enabled;

        if (!enabled) {
            // Clear any existing operator highlighting
            clearOperatorHighlighting();
        }
    }
}

bool ExpressionEditor::isOperatorHighlightingEnabled() const
{
    return m_operatorHighlightingEnabled;
}

void ExpressionEditor::updateOperatorHighlighting(const QString &text, int positionInLine, int absolutePosition, int blockStart)
{
    // Check if we're hovering over an operator
    if (positionInLine >= text.length() || positionInLine < 0) {
        clearOperatorHighlighting();
        return;
    }

    QChar ch = text[positionInLine];
    if (ch != '+' && ch != '-' && ch != '*' && ch != '/' && ch != '^') {
        clearOperatorHighlighting();
        return;
    }

    // Check if this is the same operator position we're already highlighting
    if (m_currentOperatorAbsolutePosition == absolutePosition) {
        return; // No need to update
    }

    // Get the expression bounds for this operator (relative to line)
    QPair<int, int> relativeBounds = getOperatorExpressionBounds(text, positionInLine);
    if (relativeBounds.first == -1 || relativeBounds.second == -1) {
        clearOperatorHighlighting();
        return;
    }

    // Convert relative bounds to absolute bounds
    QPair<int, int> absoluteBounds;
    absoluteBounds.first = blockStart + relativeBounds.first;
    absoluteBounds.second = blockStart + relativeBounds.second;

    // Update current operator tracking
    m_currentOperatorAbsolutePosition = absolutePosition;
    m_currentOperatorAbsoluteBounds = absoluteBounds;
    m_currentOperatorChar = ch;

    // Refresh highlighting to include the new operator highlight
    highlightCurrentLine();
}

void ExpressionEditor::clearOperatorHighlighting()
{
    if (m_currentOperatorAbsolutePosition != -1) {
        m_currentOperatorAbsolutePosition = -1;
        m_currentOperatorAbsoluteBounds = QPair<int, int>(-1, -1);
        m_currentOperatorChar = QChar();

        // Refresh highlighting to remove operator highlight
        highlightCurrentLine();
    }
}

QPair<int, int> ExpressionEditor::getOperatorExpressionBounds(const QString &text, int operatorPos)
{
    // Find the immediate left and right operands for this specific operator
    QString leftOperand = extractLeftOperand(text, operatorPos);
    QString rightOperand = extractRightOperand(text, operatorPos);

    if (leftOperand.isEmpty() || rightOperand.isEmpty()) {
        return QPair<int, int>(-1, -1); // Could not determine operands
    }

    // Calculate the start position (beginning of left operand)
    int leftStart = operatorPos - 1;

    // Skip whitespace before operator
    while (leftStart >= 0 && text[leftStart].isSpace()) {
        leftStart--;
    }

    // Move to start of left operand
    leftStart = leftStart - leftOperand.length() + 1;

    // Calculate the end position (end of right operand)
    int rightStart = operatorPos + 1;

    // Skip whitespace after operator
    while (rightStart < text.length() && text[rightStart].isSpace()) {
        rightStart++;
    }

    // Move to end of right operand
    int rightEnd = rightStart + rightOperand.length() - 1;

    return QPair<int, int>(leftStart, rightEnd);
}

QColor ExpressionEditor::getOperatorHighlightColor(QChar operatorChar, bool isCurrentLineHighlighted) const
{
    QColor baseColor;

    // Choose base color based on operator type
    switch (operatorChar.toLatin1()) {
        case '+':
        case '-':
            baseColor = QColor(100, 149, 237); // Cornflower blue
            break;
        case '*':
        case '/':
            baseColor = QColor(255, 140, 0); // Dark orange
            break;
        case '^':
            baseColor = QColor(220, 20, 60); // Crimson red for exponentiation
            break;
        default:
            baseColor = QColor(147, 112, 219); // Medium slate blue
            break;
    }

    // Adjust alpha based on whether current line is highlighted
    if (isCurrentLineHighlighted) {
        baseColor.setAlpha(140); // More opaque to stand out against line highlighting
    } else {
        baseColor.setAlpha(100); // Semi-transparent for non-highlighted lines
    }

    return baseColor;
}
