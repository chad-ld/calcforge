#include "ExpressionEditor.h"
#include "LineNumberArea.h"
#include "SyntaxHighlighter.h"
#include "Logger.h"
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
{
    setupEditor();
    setupConnections();
}

ExpressionEditor::~ExpressionEditor()
{
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

    // TEMPORARILY DISABLED: Let MainWindow QShortcut system handle font size shortcuts
    // Handle special key combinations first
    if (event->modifiers() & Qt::ControlModifier) {
        // Handle smart navigation shortcuts
        if (event->key() == Qt::Key_Left) {
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

    // Handle other key events
    QTextEdit::keyPressEvent(event);
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

void ExpressionEditor::onTextChanged()
{
    if (m_isUpdating) {
        return;
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

    // Add current line highlight (always on top)
    QTextEdit::ExtraSelection currentLineSelection;
    currentLineSelection.format.setBackground(s_currentLineBackgroundColor);
    currentLineSelection.format.setProperty(QTextCharFormat::FullWidthSelection, true);
    currentLineSelection.cursor = textCursor();

    // Clear selection to highlight the entire line
    if (!currentLineSelection.cursor.hasSelection()) {
        currentLineSelection.cursor.clearSelection();
    }

    extraSelections.append(currentLineSelection);
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
