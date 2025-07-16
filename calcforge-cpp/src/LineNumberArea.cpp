#include "LineNumberArea.h"
#include "ExpressionEditor.h"
#include "ResultsDisplay.h"
#include <QPainter>
#include <QTextBlock>
#include <QScrollBar>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QApplication>

LineNumberArea::LineNumberArea(ExpressionEditor *editor)
    : QWidget(editor)
    , m_editor(editor)
    , m_textEdit(nullptr)
    , m_width(0)
    , m_digitWidth(0)
    , m_backgroundColor(QColor(13, 17, 23))  // #0D1117
    , m_textColor(QColor(107, 114, 128))     // #6B7280 (gray-500)
    , m_currentLineColor(QColor(156, 163, 175)) // #9CA3AF (gray-400)
    , m_commentLineColor(QColor(0, 255, 0))  // #00FF00 (bright green for comments)
{
    setupWidget();
}

LineNumberArea::LineNumberArea(QTextEdit *textEdit)
    : QWidget(textEdit)
    , m_editor(nullptr)
    , m_textEdit(textEdit)
    , m_width(0)
    , m_digitWidth(0)
    , m_backgroundColor(QColor(13, 17, 23))  // #0D1117
    , m_textColor(QColor(107, 114, 128))     // #6B7280 (gray-500)
    , m_currentLineColor(QColor(156, 163, 175)) // #9CA3AF (gray-400)
    , m_commentLineColor(QColor(0, 255, 0))  // #00FF00 (bright green for comments)
{
    setupWidget();
}

LineNumberArea::~LineNumberArea()
{
}

void LineNumberArea::setupWidget()
{
    // Set font to match newdesign.html (Inter sans-serif, not monospace!)
    // Using 10px for better visual balance
    m_font = QFont("Inter", 10);
    if (!m_font.exactMatch()) {
        m_font = QFont("Segoe UI", 10);  // Windows system sans-serif
        if (!m_font.exactMatch()) {
            m_font = QFont("Arial", 10);  // Fallback sans-serif
        }
    }
    
    // Calculate initial width
    updateWidth();
    
    // Set widget properties
    setAttribute(Qt::WA_OpaquePaintEvent);
    setMouseTracking(false);
}

QSize LineNumberArea::sizeHint() const
{
    return QSize(getWidth(), 0);
}

int LineNumberArea::getWidth() const
{
    return m_width;
}

void LineNumberArea::updateWidth()
{
    m_width = calculateWidth();
    setFixedWidth(m_width);

    if (m_editor) {
        m_editor->updateLineNumberAreaWidth();
    } else if (m_textEdit) {
        // For generic QPlainTextEdit (like ResultsDisplay), call updateLineNumberAreaWidth if it exists
        ResultsDisplay *resultsDisplay = qobject_cast<ResultsDisplay*>(m_textEdit);
        if (resultsDisplay) {
            resultsDisplay->updateLineNumberAreaWidth();
        }
    }
}

int LineNumberArea::calculateWidth() const
{
    if (!m_editor && !m_textEdit) {
        return 50; // Default width
    }

    // Calculate width based on number of lines
    int lineCount;
    if (m_editor) {
        lineCount = m_editor->getLineCount();
    } else {
        lineCount = m_textEdit->document()->blockCount();
    }
    int digits = QString::number(lineCount).length();
    
    // Ensure minimum of 3 digits
    if (digits < 3) {
        digits = 3;
    }
    
    // Calculate digit width
    QFontMetrics metrics(m_font);
    int digitWidth = metrics.horizontalAdvance('9');
    
    // Calculate total width with padding
    int width = digits * digitWidth + 20; // 10px padding on each side
    
    return width;
}

void LineNumberArea::updateLineNumbers()
{
    update();
}

void LineNumberArea::setFont(const QFont &font)
{
    m_font = font;
    updateWidth();
    update();
}

void LineNumberArea::paintEvent(QPaintEvent *event)
{
    if (!m_editor && !m_textEdit) {
        return;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);

    // Fill background
    painter.fillRect(event->rect(), m_backgroundColor);

    // Paint line numbers
    paintLineNumbers(painter, event->rect());
}

void LineNumberArea::paintLineNumbers(QPainter &painter, const QRect &rect)
{
    // Set font
    painter.setFont(m_font);

    // Get the first visible block and other properties
    QTextBlock block;
    int currentLineNumber;
    QPointF contentOffset;

    if (m_editor) {
        block = m_editor->getFirstVisibleBlock();
        currentLineNumber = m_editor->getCurrentLineNumber() - 1;
        contentOffset = m_editor->getContentOffset();
    } else if (m_textEdit) {
        // Use the wrapper methods from ResultsDisplay
        ResultsDisplay *resultsDisplay = qobject_cast<ResultsDisplay*>(m_textEdit);
        if (resultsDisplay) {
            block = resultsDisplay->getFirstVisibleBlock();
            currentLineNumber = resultsDisplay->getCurrentLineNumber() - 1;
            contentOffset = resultsDisplay->getContentOffset();
        } else {
            return; // Not a ResultsDisplay
        }
    } else {
        return; // No text editor available
    }

    int blockNumber = block.blockNumber();
    QRectF blockGeometry;
    QRectF blockRect;

    if (m_editor) {
        blockGeometry = m_editor->getBlockBoundingGeometry(block);
        blockRect = m_editor->getBlockBoundingRect(block);
    } else {
        ResultsDisplay *resultsDisplay = qobject_cast<ResultsDisplay*>(m_textEdit);
        if (resultsDisplay) {
            blockGeometry = resultsDisplay->getBlockBoundingGeometry(block);
            blockRect = resultsDisplay->getBlockBoundingRect(block);
        }
    }

    int top = qRound(blockGeometry.translated(contentOffset).top());
    int bottom = top + qRound(blockRect.height());
    
    // Paint line numbers
    while (block.isValid() && top <= rect.bottom()) {
        if (block.isVisible() && bottom >= rect.top()) {
            QString number = QString::number(blockNumber + 1);
            
            // Set color based on line type and current line status
            bool isComment = false;

            // Check if this is a comment line
            if (m_editor) {
                // For expression editor, check if line starts with ":::"
                QString lineText = block.text().trimmed();
                isComment = lineText.startsWith(":::");
            } else if (m_textEdit) {
                // For results display, check if it's a comment line
                ResultsDisplay* resultsDisplay = qobject_cast<ResultsDisplay*>(m_textEdit);
                if (resultsDisplay) {
                    isComment = resultsDisplay->isCommentLine(blockNumber + 1);
                }
            }

            // Set color based on line type and current line status
            if (isComment) {
                painter.setPen(m_commentLineColor);  // Green for comment lines
            } else if (blockNumber == currentLineNumber) {
                painter.setPen(m_currentLineColor);
            } else {
                painter.setPen(m_textColor);
            }
            
            // Draw the line number
            int fontHeight;
            if (m_editor) {
                fontHeight = m_editor->fontMetrics().height();
            } else {
                fontHeight = m_textEdit->fontMetrics().height();
            }

            painter.drawText(0, top, width() - 10, fontHeight,
                           Qt::AlignRight | Qt::AlignVCenter, number);
        }

        block = block.next();
        top = bottom;

        if (m_editor) {
            bottom = top + qRound(m_editor->getBlockBoundingRect(block).height());
        } else {
            ResultsDisplay *resultsDisplay = qobject_cast<ResultsDisplay*>(m_textEdit);
            if (resultsDisplay) {
                bottom = top + qRound(resultsDisplay->getBlockBoundingRect(block).height());
            }
        }
        ++blockNumber;
    }
}

void LineNumberArea::mousePressEvent(QMouseEvent *event)
{
    if (!m_editor && !m_textEdit) {
        return;
    }

    // Calculate which line was clicked
    QTextBlock block;
    QPointF contentOffset;

    if (m_editor) {
        block = m_editor->getFirstVisibleBlock();
        contentOffset = m_editor->getContentOffset();
    } else {
        ResultsDisplay *resultsDisplay = qobject_cast<ResultsDisplay*>(m_textEdit);
        if (resultsDisplay) {
            block = resultsDisplay->getFirstVisibleBlock();
            contentOffset = resultsDisplay->getContentOffset();
        } else {
            return; // Not a ResultsDisplay
        }
    }

    QRectF blockGeometry;
    QRectF blockRect;

    if (m_editor) {
        blockGeometry = m_editor->getBlockBoundingGeometry(block);
        blockRect = m_editor->getBlockBoundingRect(block);
    } else {
        ResultsDisplay *resultsDisplay = qobject_cast<ResultsDisplay*>(m_textEdit);
        if (resultsDisplay) {
            blockGeometry = resultsDisplay->getBlockBoundingGeometry(block);
            blockRect = resultsDisplay->getBlockBoundingRect(block);
        }
    }

    int top = qRound(blockGeometry.translated(contentOffset).top());
    int bottom = top + qRound(blockRect.height());

    while (block.isValid()) {
        if (event->position().y() >= top && event->position().y() <= bottom) {
            // Move cursor to the clicked line
            QTextCursor cursor(block);
            if (m_editor) {
                m_editor->setTextCursor(cursor);
            } else {
                m_textEdit->setTextCursor(cursor);
            }
            break;
        }

        block = block.next();
        top = bottom;

        if (m_editor) {
            bottom = top + qRound(m_editor->getBlockBoundingRect(block).height());
        } else {
            ResultsDisplay *resultsDisplay = qobject_cast<ResultsDisplay*>(m_textEdit);
            if (resultsDisplay) {
                bottom = top + qRound(resultsDisplay->getBlockBoundingRect(block).height());
            }
        }
    }
    
    QWidget::mousePressEvent(event);
}

void LineNumberArea::wheelEvent(QWheelEvent *event)
{
    // Forward wheel events to the editor
    if (m_editor) {
        QApplication::sendEvent(m_editor, event);
    }
}
