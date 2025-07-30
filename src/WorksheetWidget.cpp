#include "WorksheetWidget.h"
#include "ExpressionEditor.h"
#include "ResultsDisplay.h"
#include "LineNumberArea.h"
#include "CalculationEngine.h"
#include "DependencyTracker.h"
#include "LNReferenceAutoUpdater.h"
#include "LineChangeDetector.h"
#include "SyntaxHighlighter.h"
#include "CustomTabWidget.h"
#include "Logger.h"
// Phase 3.2: Business logic separation
#include "WorksheetModel.h"
#include "CalculationService.h"

// Phase 4.1: Event system
#include "EventBus.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QScrollBar>
#include <QSplitterHandle>
#include <QTimer>
#include <QMouseEvent>
#include <QPainter>
#include <QEnterEvent>
#include <QShowEvent>
#include <QTimer>
#include <QMainWindow>
#include <QTabWidget>

// Custom splitter handle that properly responds to hover events
class CustomSplitterHandle : public QSplitterHandle
{
public:
    CustomSplitterHandle(Qt::Orientation orientation, QSplitter *parent)
        : QSplitterHandle(orientation, parent), m_isHovered(false)
    {
        setMouseTracking(true);
        setAttribute(Qt::WA_Hover, true);
    }

protected:
    void enterEvent(QEnterEvent *event) override
    {
        Q_UNUSED(event);
        m_isHovered = true;
        update();
    }

    void leaveEvent(QEvent *event) override
    {
        Q_UNUSED(event);
        m_isHovered = false;
        update();
    }

    void paintEvent(QPaintEvent *event) override
    {
        Q_UNUSED(event);
        QPainter painter(this);

        QColor backgroundColor;
        QColor borderColor;

        if (m_isHovered) {
            backgroundColor = QColor("#0c7ff2");  // Blue on hover
            borderColor = QColor("#0969DA");
        } else {
            backgroundColor = QColor("#6B7280");  // Default gray
            borderColor = QColor("#484F58");
        }

        // Calculate position to match edge handles (centered relative to main window)
        const int handleHeight = 150;

        // Get the main window to calculate proper centering
        QWidget *mainWindow = this;
        while (mainWindow->parentWidget()) {
            mainWindow = mainWindow->parentWidget();
        }

        // Calculate where the handle should be positioned to match edge handles
        // Edge handles are centered relative to main window height
        const int mainWindowHeight = mainWindow->height();
        const int edgeHandleCenterY = (mainWindowHeight - handleHeight) / 2;

        // Calculate our widget's position relative to main window
        QPoint globalPos = mapToGlobal(QPoint(0, 0));
        QPoint mainWindowPos = mainWindow->mapFromGlobal(globalPos);
        const int ourOffsetFromTop = mainWindowPos.y();

        // Calculate where to position our handle to align with edge handles
        const int targetY = edgeHandleCenterY - ourOffsetFromTop;
        const int clampedY = qMax(0, qMin(targetY, rect().height() - handleHeight));

        // Create the handle rectangle at the calculated position
        QRect handleRect(0, clampedY, rect().width(), handleHeight);

        // Draw the handle with proper colors
        painter.fillRect(handleRect.adjusted(0, 2, 0, -2), backgroundColor);
        painter.setPen(borderColor);
        painter.drawRect(handleRect.adjusted(0, 2, -1, -3));
    }

private:
    bool m_isHovered;
};

// Custom splitter that uses our custom handle
class CustomSplitter : public QSplitter
{
public:
    CustomSplitter(Qt::Orientation orientation, QWidget *parent = nullptr)
        : QSplitter(orientation, parent)
    {
        setMouseTracking(true);
        setAttribute(Qt::WA_Hover, true);
    }

protected:
    QSplitterHandle *createHandle() override
    {
        return new CustomSplitterHandle(orientation(), this);
    }
};

WorksheetWidget::WorksheetWidget(QWidget *parent)
    : QWidget(parent)
    , m_layout(nullptr)
    , m_editor(nullptr)
    , m_results(nullptr)
    , m_lineNumberArea(nullptr)
    , m_resultsLineNumberArea(nullptr)
    , m_columnsSplitter(nullptr)
    , m_evaluationTimer(nullptr)
    , m_isModified(false)
    , m_splitterStateRestored(false)
    , m_pendingSplitterState()
    , m_hasCrossSheetRefs(false)
    , m_settings(nullptr)
    // Phase 3.2: Business logic separation
    , m_model(std::make_unique<WorksheetModel>(this))
    , m_calculationService(std::make_unique<CalculationService>(this))
    // Phase 4.1: Event system
    , m_eventBus(nullptr)
    // Legacy support
    , m_calculationEngine(nullptr)
    , m_dependencyTracker(std::make_unique<DependencyTracker>())
    , m_referenceAutoUpdater(std::make_unique<LNReferenceAutoUpdater>(this))
    , m_isLoadingContent(false)
{
    // Initialize settings
    m_settings = new QSettings("CalcForge", "CalcForge", this);

    // Initialize calculation engine
    m_calculationEngine = new CalculationEngine();

    // Set the worksheet widget reference for content access
    m_calculationEngine->setWorksheetWidget(this);

    // Test the calculation engine
    QString testResult = m_calculationEngine->evaluateExpression("2 + 2", 1);
    LOG_INFO(QString("Calculation engine initialized - test: 2 + 2 = %1").arg(testResult));

    // Phase 3.2: Initialize business logic separation
    setupBusinessLogic();

    // Setup UI
    setupUI();
    setupConnections();
    
    // Initialize evaluation timer
    m_evaluationTimer = new QTimer(this);
    m_evaluationTimer->setSingleShot(true);
    m_evaluationTimer->setInterval(150); // 150ms delay for evaluation (faster response)
    connect(m_evaluationTimer, &QTimer::timeout, this, &WorksheetWidget::evaluateAndHighlight);
}

WorksheetWidget::WorksheetWidget(PluginManager* pluginManager, QWidget *parent)
    : QWidget(parent)
    , m_editor(nullptr)
    , m_results(nullptr)
    , m_columnsSplitter(nullptr)
    , m_calculationEngine(nullptr)
    , m_settings(nullptr)
    , m_dependencyTracker(std::make_unique<DependencyTracker>())
    , m_referenceAutoUpdater(std::make_unique<LNReferenceAutoUpdater>(this))
    , m_isLoadingContent(false)
{
    // Initialize settings
    m_settings = new QSettings("CalcForge", "CalcForge", this);

    // Phase 4.2: Initialize calculation engine with plugin manager
    if (pluginManager) {
        m_calculationEngine = new CalculationEngine(pluginManager);
        LOG_DEBUG("WorksheetWidget: Initialized with plugin-aware calculation engine");
    } else {
        m_calculationEngine = new CalculationEngine();
        LOG_DEBUG("WorksheetWidget: Initialized with standard calculation engine");
    }

    // Set the worksheet widget reference for content access
    m_calculationEngine->setWorksheetWidget(this);

    // Test the calculation engine
    QString testResult = m_calculationEngine->evaluateExpression("2 + 2", 1);
    LOG_INFO(QString("Calculation engine initialized - test: 2 + 2 = %1").arg(testResult));

    // Phase 3.2: Initialize business logic separation
    setupBusinessLogic();

    // Setup UI
    setupUI();
    setupConnections();

    // Initialize evaluation timer
    m_evaluationTimer = new QTimer(this);
    m_evaluationTimer->setSingleShot(true);
    m_evaluationTimer->setInterval(150); // 150ms delay for evaluation (faster response)
    connect(m_evaluationTimer, &QTimer::timeout, this, &WorksheetWidget::evaluateAndHighlight);
}

WorksheetWidget::~WorksheetWidget()
{
    delete m_calculationEngine;
}

void WorksheetWidget::setupBusinessLogic()
{
    // Phase 3.2: Set up business logic separation
    LOG_DEBUG("WorksheetWidget: Setting up business logic separation");

    // Connect model to calculation service
    m_calculationService->setModel(m_model.get());

    // Connect model signals to UI updates
    connect(m_model.get(), &WorksheetModel::contentChanged, this, &WorksheetWidget::contentChanged);
    connect(m_model.get(), &WorksheetModel::modificationStateChanged, this, [this](bool modified) {
        m_isModified = modified;
    });

    // Connect calculation service signals
    connect(m_calculationService.get(), &CalculationService::calculationCompleted, this, &WorksheetWidget::evaluateAndHighlight);
    connect(m_calculationService.get(), &CalculationService::lineValueChanged, this, [this](int lineNumber, double value) {
        // Update results display when line values change
        // This will be implemented when we refactor the results display
    });

    LOG_DEBUG("WorksheetWidget: Business logic setup completed");
}

void WorksheetWidget::setupUI()
{
    // Set widget styling to match GitHub dark theme
    setStyleSheet(
        "QWidget {"
        "  background-color: #0D1117;"
        "}"
    );

    // Create main layout with proper spacing like HTML design
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(0, 16, 16, 16); // Remove left margin
    m_layout->setSpacing(16);

    // Create the 4-column layout container (matching column_example.png)
    QWidget *columnsContainer = new QWidget(this);
    columnsContainer->setStyleSheet(
        "QWidget {"
        "  background-color: #161B22;"
        "  border-radius: 8px;"
        "  border: 1px solid #30363D;"
        "}"
    );

    // Use CustomSplitter instead of QSplitter to allow resizing with proper hover effects
    m_columnsSplitter = new CustomSplitter(Qt::Horizontal, columnsContainer);

    // Set splitter properties for better visibility
    m_columnsSplitter->setHandleWidth(6);  // Match edge handle thickness (6px)
    m_columnsSplitter->setChildrenCollapsible(false);  // Prevent collapsing
    m_columnsSplitter->setMouseTracking(true);  // Enable mouse tracking for hover effects
    m_columnsSplitter->setAttribute(Qt::WA_Hover, true);  // Enable hover events

    // Custom splitter handles its own styling programmatically
    m_columnsSplitter->setStyleSheet("QSplitter { background-color: transparent; }");

    QHBoxLayout *columnsLayout = new QHBoxLayout(columnsContainer);
    columnsLayout->setContentsMargins(0, 0, 0, 0);
    columnsLayout->setSpacing(0);
    columnsLayout->addWidget(m_columnsSplitter);

    // Create expression editor container with line numbers
    QWidget *expressionContainer = new QWidget(this);
    expressionContainer->setStyleSheet("QWidget { background-color: #161B22; }");
    QHBoxLayout *expressionLayout = new QHBoxLayout(expressionContainer);
    expressionLayout->setContentsMargins(0, 0, 0, 0);
    expressionLayout->setSpacing(0);

    // Expression editor
    m_editor = new ExpressionEditor(this);
    // Note: ExpressionEditor has its own Material Design styling - don't override it here

    // Create line number area for expressions
    m_lineNumberArea = new LineNumberArea(m_editor);
    m_editor->setLineNumberArea(m_lineNumberArea);

    // Add to expression container
    expressionLayout->addWidget(m_lineNumberArea);
    expressionLayout->addWidget(m_editor);

    // New worksheets start blank

    // Create results container WITH line numbers (matching expression side)
    QWidget *resultsContainer = new QWidget(this);
    resultsContainer->setStyleSheet("QWidget { background-color: #161B22; }");
    QHBoxLayout *resultsLayout = new QHBoxLayout(resultsContainer);
    resultsLayout->setContentsMargins(0, 0, 0, 0);
    resultsLayout->setSpacing(0);

    // Results display with line numbers
    m_results = new ResultsDisplay(this);
    // Note: ResultsDisplay has its own Material Design styling - don't override it here

    // Create line number area for results (matching expression side)
    m_resultsLineNumberArea = new LineNumberArea(m_results);
    m_results->setLineNumberArea(m_resultsLineNumberArea);

    // Add line number area and results display to container
    resultsLayout->addWidget(m_resultsLineNumberArea);
    resultsLayout->addWidget(m_results);

    // Results will be populated when expressions are evaluated

    // Line numbers are now handled automatically by LineNumberArea

    // Add both containers to the splitter (instead of layout for resizing)
    m_columnsSplitter->addWidget(expressionContainer);
    m_columnsSplitter->addWidget(resultsContainer);

    // Don't set default sizes here - let MainWindow handle it via setSplitterState()
    // This prevents overriding restored splitter state


    // Add the columns container to main layout
    m_layout->addWidget(columnsContainer);
}

void WorksheetWidget::setupConnections()
{
    // Connect editor text changes
    connect(m_editor, &ExpressionEditor::contentChanged, this, &WorksheetWidget::onTextChanged);

    // Connect scrollbar synchronization
    connect(m_editor->verticalScrollBar(), &QScrollBar::valueChanged,
            this, &WorksheetWidget::syncEditorToResults);
    connect(m_results->verticalScrollBar(), &QScrollBar::valueChanged,
            this, &WorksheetWidget::syncResultsToEditor);

    // Connect line count changes
    connect(m_editor, &ExpressionEditor::lineCountChanged,
            [this]() {
                int newLineCount = m_editor->getLineCount();
                int oldResultsLineCount = m_results->document()->blockCount();
                LOG_DEBUG(QString("=== LineCountChanged: Editor now has %1 lines, Results had %2 lines ===")
                          .arg(newLineCount).arg(oldResultsLineCount));
                m_results->updateLineCount(newLineCount);
                int newResultsLineCount = m_results->document()->blockCount();
                LOG_DEBUG(QString("=== LineCountChanged: Results now has %1 lines ===").arg(newResultsLineCount));
            });

    // Connect cursor position changes for current line highlighting synchronization
    connect(m_editor, &QTextEdit::cursorPositionChanged, this, [this]() {
        // Simple and reliable approach: only highlight if this editor has focus
        // This ensures only the currently active worksheet gets highlighting updates
        bool shouldHighlight = m_editor->hasFocus();

        LOG_DEBUG(QString("=== WorksheetWidget::cursorPositionChanged ==="));
        LOG_DEBUG(QString("  Editor has focus: %1").arg(shouldHighlight));

        int currentLine = m_editor->getCurrentLineNumber();
        QString currentLineText = m_editor->textCursor().block().text();
        int editorLineCount = m_editor->getLineCount();
        int resultsLineCount = m_results->document()->blockCount();

        LOG_DEBUG(QString("  Editor current line: %1").arg(currentLine));
        LOG_DEBUG(QString("  Editor line count: %1").arg(editorLineCount));
        LOG_DEBUG(QString("  Results line count: %1").arg(resultsLineCount));
        LOG_DEBUG(QString("  Current line text: '%1'").arg(currentLineText));

        // Only highlight if this editor has focus (i.e., this is the active worksheet)
        if (shouldHighlight) {
            LOG_DEBUG("  PROCEEDING: Editor has focus, updating highlighting");

            // Instead of passing a potentially stale line number, make ResultsDisplay
            // get the current line directly from the ExpressionEditor's cursor
            // This ensures it always highlights the correct line, even after document changes
            m_results->highlightCurrentLineFromEditor(m_editor);

            // NOTE: Cross-sheet background highlighting is now disabled
            // It's only triggered by Shift+Enter navigation, not automatic cursor movement
            // handleCrossSheetBackgroundHighlighting(currentLineText);

            // Also update the results line number area to show current line styling
            if (m_results->getLineNumberArea()) {
                m_results->getLineNumberArea()->update();
            }
        } else {
            LOG_DEBUG("  SKIPPING: Editor does not have focus");
        }

        LOG_DEBUG(QString("=== END WorksheetWidget::cursorPositionChanged ==="));
    });

    // Connect splitter changes to emit signal for global synchronization
    connect(m_columnsSplitter, &QSplitter::splitterMoved, [this]() {
        if (m_splitterStateRestored) { // Only emit after initial restoration
            QByteArray newState = getSplitterState();
            if (!newState.isEmpty()) {
                emit splitterMoved(newState);
                LOG_DEBUG(QString("Splitter moved - emitting new state: '%1'").arg(QString::fromUtf8(newState)));
            }
        }
    });
}

QString WorksheetWidget::getContent() const
{
    // Phase 3.2: Get content from model if available, otherwise from editor
    if (m_model) {
        return m_model->getContent();
    }
    return m_editor->toPlainText();
}

void WorksheetWidget::setContent(const QString &content)
{
    // Set flag to disable auto-updates during content loading
    m_isLoadingContent = true;
    LOG_DEBUG("WorksheetWidget::setContent - Set m_isLoadingContent = true");

    // Phase 3.2: Update model first
    if (m_model) {
        m_model->setContent(content);
        m_model->setModified(false);
    }

    // Clear stored line values to prevent stale references
    if (m_calculationEngine) {
        m_calculationEngine->clearLineValues();
    }

    m_editor->setPlainText(content);
    m_isModified = false;

    // Position cursor at start (top of sheet)
    m_editor->positionCursorAtStart();

    // Reset horizontal scroll position to leftmost for both editor and results
    // Use QTimer::singleShot to ensure this happens after the content is fully rendered
    QTimer::singleShot(0, [this]() {
        m_editor->horizontalScrollBar()->setValue(0);
        m_results->horizontalScrollBar()->setValue(0);
    });

    // Update our tracking variables to prevent auto-updates on first edit
    m_lastContent = content;

    // Clear the loading flag
    m_isLoadingContent = false;
    LOG_DEBUG("WorksheetWidget::setContent - Set m_isLoadingContent = false");

    // Trigger evaluation
    startEvaluationTimer();
}

bool WorksheetWidget::isModified() const
{
    return m_isModified;
}

void WorksheetWidget::setModified(bool modified)
{
    m_isModified = modified;
}

QByteArray WorksheetWidget::getSplitterState() const
{
    if (m_columnsSplitter && m_splitterStateRestored) {
        // Only save if we've already restored the initial state
        QList<int> sizes = m_columnsSplitter->sizes();
        QString sizeString = QString("%1,%2").arg(sizes.value(0)).arg(sizes.value(1));
        QByteArray customState = sizeString.toUtf8();

        LOG_DEBUG(QString("Saving custom splitter state - current sizes: %1, custom state: '%2'")
                  .arg(sizeString).arg(sizeString));
        return customState;
    } else {
        LOG_DEBUG("Not saving splitter state - either no splitter or not yet restored");
        return QByteArray();
    }
}

void WorksheetWidget::setSplitterState(const QByteArray &state)
{
    // Store the state to apply later when the widget is shown
    m_pendingSplitterState = state;
    LOG_DEBUG(QString("Stored pending custom splitter state - size: %1 bytes, content: '%2'")
              .arg(state.size()).arg(QString::fromUtf8(state)));
}

void WorksheetWidget::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);

    // Apply pending splitter state now that the widget is shown and has proper size
    if (m_columnsSplitter && !m_splitterStateRestored) {
        QStringList beforeSizes;
        for (int size : m_columnsSplitter->sizes()) {
            beforeSizes << QString::number(size);
        }
        LOG_DEBUG(QString("Before restore (in showEvent) - splitter sizes: %1").arg(beforeSizes.join(", ")));

        if (!m_pendingSplitterState.isEmpty()) {
            // Parse our custom state format: "leftSize,rightSize"
            QString stateString = QString::fromUtf8(m_pendingSplitterState);
            QStringList sizeParts = stateString.split(',');

            if (sizeParts.size() == 2) {
                bool ok1, ok2;
                int leftSize = sizeParts[0].toInt(&ok1);
                int rightSize = sizeParts[1].toInt(&ok2);

                if (ok1 && ok2 && leftSize > 0 && rightSize > 0) {
                    // Apply the saved sizes directly
                    m_columnsSplitter->setSizes({leftSize, rightSize});
                    LOG_DEBUG(QString("Restored custom splitter state: %1, %2").arg(leftSize).arg(rightSize));
                } else {
                    // Invalid format, use defaults
                    m_columnsSplitter->setSizes({400, 400});
                    LOG_DEBUG("Invalid custom state format, using defaults");
                }
            } else {
                // Invalid format, use defaults
                m_columnsSplitter->setSizes({400, 400});
                LOG_DEBUG("Invalid custom state format (wrong parts count), using defaults");
            }
        } else {
            // No saved state - use defaults
            m_columnsSplitter->setSizes({400, 400});
            LOG_DEBUG("No saved splitter state - using default equal sizes (400, 400)");
        }

        QStringList afterSizes;
        for (int size : m_columnsSplitter->sizes()) {
            afterSizes << QString::number(size);
        }
        LOG_DEBUG(QString("After restore (in showEvent) - sizes: %1").arg(afterSizes.join(", ")));

        m_splitterStateRestored = true;

        // Clear the pending state
        m_pendingSplitterState.clear();
    }
}

void WorksheetWidget::onTextChanged()
{
    m_isModified = true;

    // Check if content actually changed (avoid unnecessary evaluations)
    QString currentContent = m_editor->toPlainText();

    LOG_DEBUG(QString("=== onTextChanged called ==="));
    LOG_DEBUG(QString("Current content length: %1").arg(currentContent.length()));
    LOG_DEBUG(QString("Last content length: %1").arg(m_lastContent.length()));
    LOG_DEBUG(QString("Is loading content: %1").arg(m_isLoadingContent));

    if (currentContent == m_lastContent) {
        LOG_DEBUG("Content unchanged, returning early");
        return;
    }

    LOG_DEBUG("Content changed, proceeding with processing");

    // Skip auto-updates during content loading to prevent incorrect reference shifts
    if (!m_isLoadingContent) {
        // Process LN reference auto-updates before evaluation
        QStringList oldLines = m_lastContent.split('\n');
        QStringList newLines = currentContent.split('\n');

        // Apply preprocessing to both old and new lines to avoid false change detection
        // This prevents issues where zero-padded numbers (010 -> 10) trigger incorrect auto-updates
        QStringList preprocessedOldLines, preprocessedNewLines;
        for (const QString &line : oldLines) {
            preprocessedOldLines.append(m_calculationEngine->preprocessExpression(line));
        }
        for (const QString &line : newLines) {
            preprocessedNewLines.append(m_calculationEngine->preprocessExpression(line));
        }

        // Detect if this is likely a paste operation (large content change)
        bool isPasteOperation = false;
        int oldLineCount = preprocessedOldLines.size();
        int newLineCount = preprocessedNewLines.size();
        int lineDifference = abs(newLineCount - oldLineCount);

        // Consider it a paste if:
        // 1. Line count changes dramatically (more than 5 lines difference)
        // 2. OR more than 50% of content changed at once
        if (lineDifference > 5 || (oldLineCount > 0 && lineDifference > oldLineCount * 0.5)) {
            isPasteOperation = true;
            LOG_INFO(QString("Detected paste operation: %1 -> %2 lines, disabling auto-updates").arg(oldLineCount).arg(newLineCount));

            // Clear stored line values during paste operations to prevent stale references
            if (m_calculationEngine) {
                m_calculationEngine->clearLineValues();
                LOG_DEBUG("Cleared line values due to paste operation");
            }
        }

        // Check for LN reference updates using preprocessed content to avoid false positives
        // Skip auto-updates during paste operations to prevent incorrect reference replacement
        LOG_DEBUG(QString("LN Auto-Update: About to call processTextChanges, isPasteOperation=%1").arg(isPasteOperation));
        bool hasLNUpdates = false;
        if (!isPasteOperation && m_referenceAutoUpdater) {
            hasLNUpdates = m_referenceAutoUpdater->processTextChanges(preprocessedOldLines, preprocessedNewLines, m_dependencyTracker.get());
            LOG_DEBUG(QString("LN Auto-Update: processTextChanges returned %1").arg(hasLNUpdates));
        }

        if (hasLNUpdates) {
            // Update stored line values in calculation engine to match new line positions
            updateCalculationEngineLineValues(preprocessedOldLines, preprocessedNewLines);

            // References were updated, update the editor content
            QString updatedContent = preprocessedNewLines.join('\n');

            // Compare with the original preprocessed content (before LN updates)
            // to see if LN references actually changed
            QStringList originalPreprocessedLines;
            QStringList newLines = currentContent.split('\n');
            for (const QString &line : newLines) {
                originalPreprocessedLines.append(m_calculationEngine->preprocessExpression(line));
            }

            // Compare line by line instead of as a single string
            bool contentDiffers = false;
            if (preprocessedNewLines.size() != originalPreprocessedLines.size()) {
                contentDiffers = true;
                LOG_DEBUG(QString("LN Auto-Update: Line count differs: %1 vs %2").arg(originalPreprocessedLines.size()).arg(preprocessedNewLines.size()));
            } else {
                for (int i = 0; i < preprocessedNewLines.size(); ++i) {
                    if (preprocessedNewLines[i] != originalPreprocessedLines[i]) {
                        contentDiffers = true;
                        LOG_DEBUG(QString("LN Auto-Update: Line %1 differs: '%2' -> '%3'").arg(i+1).arg(originalPreprocessedLines[i]).arg(preprocessedNewLines[i]));
                    }
                }
            }

            if (contentDiffers) {
                LOG_INFO(QString("LN Auto-Update: Content differs, updating editor"));

                // Save cursor position before updating content
                QTextCursor cursor = m_editor->textCursor();
                int cursorPosition = cursor.position();

                // Temporarily disable text change signals to avoid recursion
                m_editor->blockSignals(true);

                // Update the editor with the new content, line by line
                m_editor->clear();
                QTextCursor editorCursor = m_editor->textCursor();
                for (int i = 0; i < preprocessedNewLines.size(); ++i) {
                    if (i > 0) {
                        editorCursor.insertText("\n");
                    }
                    editorCursor.insertText(preprocessedNewLines[i]);
                }

                // Restore cursor position (or as close as possible)
                cursor = m_editor->textCursor();
                cursor.setPosition(qMin(cursorPosition, m_editor->toPlainText().length()));
                m_editor->setTextCursor(cursor);

                m_editor->blockSignals(false);

                // Force current line highlighting update after document rebuild
                // This ensures highlighting is properly restored on both expression and results sides
                if (m_editor->hasFocus()) {
                    // Use QTimer::singleShot to ensure the highlighting update happens after all document changes are complete
                    QTimer::singleShot(0, [this]() {
                        m_editor->highlightCurrentLine();
                        if (m_results) {
                            m_results->highlightCurrentLineFromEditor(m_editor);
                        }
                        LOG_DEBUG("Forced current line highlighting update after LN reference auto-update");
                    });
                }

                // Update our tracking variables
                currentContent = m_editor->toPlainText();
                LOG_INFO("LN references auto-updated in worksheet");
            } else {
                LOG_WARNING("LN Auto-Update: Content is identical, no editor update needed");
            }
        }

        // Check for line insertions/deletions that affect cross-sheet references
        // Only emit signal if there are actual line number changes (not just modifications)
        LOG_DEBUG(QString("Checking for cross-sheet line changes. isPasteOperation: %1").arg(isPasteOperation));
        if (!isPasteOperation) {
            LineChangeDetector changeDetector;
            QList<LineChange> changes = changeDetector.detectChanges(preprocessedOldLines, preprocessedNewLines);
            LOG_DEBUG(QString("LineChangeDetector found %1 total changes").arg(changes.size()));

            // Check if any changes affect line numbering
            bool hasLineNumberChanges = false;
            for (const LineChange &change : changes) {
                LOG_DEBUG(QString("Change type: %1 at line %2 (count: %3)").arg(change.type).arg(change.startLine).arg(change.count));
                if (change.type == LineChange::Insertion || change.type == LineChange::Deletion) {
                    hasLineNumberChanges = true;
                    break;
                }
            }

            LOG_DEBUG(QString("Has line number changes: %1").arg(hasLineNumberChanges));
            if (hasLineNumberChanges) {
                // Get the current sheet name to include in the signal
                QString currentSheetName = getCurrentSheetName();
                LOG_DEBUG(QString("Emitting lineNumberingChanged signal for sheet '%1' with %2 changes")
                         .arg(currentSheetName).arg(changes.size()));
                emit lineNumberingChanged(currentSheetName, changes);
            } else {
                LOG_DEBUG("No line number changes detected - only modifications");
            }
        } else {
            LOG_DEBUG("Skipping cross-sheet line change detection due to paste operation");
        }
    }

    m_lastContent = currentContent;

    // Phase 4.1: Sync model with editor content to ensure save works correctly
    if (m_model && m_model->getContent() != currentContent) {
        m_model->setContent(currentContent);
        LOG_DEBUG("WorksheetWidget: Synced model content with editor");
    }

    // Emit contentChanged signal to notify MainWindow of modifications
    emit contentChanged();

    // Start evaluation timer
    startEvaluationTimer();
}

void WorksheetWidget::startEvaluationTimer()
{
    // Stop any existing timer
    m_evaluationTimer->stop();

    // Start new timer
    m_evaluationTimer->start();
}

void WorksheetWidget::evaluateAndHighlight()
{
    if (!m_calculationEngine || !m_dependencyTracker) {
        LOG_ERROR("Calculation engine or dependency tracker not initialized");
        return;
    }

    QString content = m_editor->toPlainText();
    QStringList currentLines = content.split('\n');

    // Detect which lines have changed
    QSet<int> changedLines = detectChangedLines(currentLines);

    // Always evaluate on first load or if we have changes
    bool isFirstLoad = m_lastLines.isEmpty();
    if (changedLines.isEmpty() && !isFirstLoad) {
        // No changes detected and not first load, skip evaluation
        LOG_DEBUG("No changes detected, skipping evaluation");
        return;
    }

    // Call the extracted evaluation method
    evaluateLines(currentLines, changedLines);
}

QSet<int> WorksheetWidget::detectChangedLines(const QStringList &currentLines)
{
    QSet<int> changedLines;

    // If this is the first evaluation, mark all non-empty lines as changed
    if (m_lastLines.isEmpty()) {
        for (int i = 0; i < currentLines.size(); ++i) {
            if (!currentLines[i].trimmed().isEmpty()) {
                changedLines.insert(i + 1);  // Convert to 1-based line numbers
            }
        }
        return changedLines;
    }

    // Compare with previous lines
    int maxLines = std::max(currentLines.size(), m_lastLines.size());

    for (int i = 0; i < maxLines; ++i) {
        QString currentLine = (i < currentLines.size()) ? currentLines[i] : "";
        QString lastLine = (i < m_lastLines.size()) ? m_lastLines[i] : "";

        if (currentLine != lastLine) {
            changedLines.insert(i + 1);  // Convert to 1-based line numbers
        }
    }

    return changedLines;
}

void WorksheetWidget::updateCalculationEngineLineValues(const QStringList &oldLines, const QStringList &newLines)
{
    if (!m_calculationEngine) {
        return;
    }

    // Simple detection of line insertions/deletions
    int oldCount = oldLines.size();
    int newCount = newLines.size();

    if (oldCount == newCount) {
        // No line count change, just modifications
        return;
    }

    LOG_DEBUG(QString("Line count changed from %1 to %2, clearing all line values for clean recalculation").arg(oldCount).arg(newCount));

    // When line count changes (insertions/deletions), clear all line values
    // This ensures that statistical functions like count(below) work correctly
    // and prevents stale values from persisting at incorrect line numbers
    m_calculationEngine->clearLineValues();

    // The lines will be re-evaluated in the correct order by the dependency tracker
}

void WorksheetWidget::syncEditorToResults(int value)
{
    // Prevent infinite recursion
    static bool syncing = false;
    if (syncing) return;
    syncing = true;

    // Get scrollbars
    QScrollBar *editorScrollBar = m_editor->verticalScrollBar();
    QScrollBar *resultsScrollBar = m_results->verticalScrollBar();

    // Calculate proportional position to handle different content heights
    int editorMax = qMax(1, editorScrollBar->maximum());
    int resultsMax = qMax(1, resultsScrollBar->maximum());

    if (editorMax > 0 && resultsMax > 0) {
        // Calculate ratio and apply to results
        double ratio = static_cast<double>(value) / editorMax;
        int targetValue = static_cast<int>(ratio * resultsMax);

        // Only update if different to prevent unnecessary updates
        if (resultsScrollBar->value() != targetValue) {
            resultsScrollBar->setValue(targetValue);
        }
    } else {
        // Fallback to direct value if no scaling needed
        if (resultsScrollBar->value() != value) {
            resultsScrollBar->setValue(value);
        }
    }

    syncing = false;
    // Line number area sync is now handled automatically by LineNumberArea
}

void WorksheetWidget::syncResultsToEditor(int value)
{
    // Prevent infinite recursion
    static bool syncing = false;
    if (syncing) return;
    syncing = true;

    // Get scrollbars
    QScrollBar *editorScrollBar = m_editor->verticalScrollBar();
    QScrollBar *resultsScrollBar = m_results->verticalScrollBar();

    // Calculate proportional position to handle different content heights
    int editorMax = qMax(1, editorScrollBar->maximum());
    int resultsMax = qMax(1, resultsScrollBar->maximum());

    if (resultsMax > 0 && editorMax > 0) {
        // Calculate ratio and apply to editor
        double ratio = static_cast<double>(value) / resultsMax;
        int targetValue = static_cast<int>(ratio * editorMax);

        // Only update if different to prevent unnecessary updates
        if (editorScrollBar->value() != targetValue) {
            editorScrollBar->setValue(targetValue);
        }
    } else {
        // Fallback to direct value if no scaling needed
        if (editorScrollBar->value() != value) {
            editorScrollBar->setValue(value);
        }
    }

    syncing = false;
    // Line number area sync is now handled automatically by LineNumberArea
}

double WorksheetWidget::getLineValue(int lineNumber) const
{
    // Phase 3.2: Get from model first, fallback to calculation engine
    if (m_model && m_model->hasLineValue(lineNumber)) {
        return m_model->getLineValue(lineNumber);
    }
    if (m_calculationEngine) {
        return m_calculationEngine->getLineValue(lineNumber);
    }
    return 0.0;
}

bool WorksheetWidget::hasLineValue(int lineNumber) const
{
    // Phase 3.2: Check model first, fallback to calculation engine
    if (m_model && m_model->hasLineValue(lineNumber)) {
        return true;
    }
    if (m_calculationEngine) {
        return m_calculationEngine->hasLineValue(lineNumber);
    }
    return false;
}

QString WorksheetWidget::evaluateExpression(const QString &expression) const
{
    // Phase 3.2: Use calculation service first, fallback to calculation engine
    if (m_calculationService) {
        return m_calculationService->evaluateExpression(expression);
    }
    if (m_calculationEngine) {
        // Use a temporary line number for tooltip evaluation
        return m_calculationEngine->evaluateExpression(expression, 0);
    }
    return QString();
}

CalculationEngine* WorksheetWidget::getCalculationEngine() const
{
    // Phase 3.2: Legacy support - return calculation engine for backward compatibility
    return m_calculationEngine;
}

bool WorksheetWidget::hasCrossSheetReferences() const
{
    if (!m_dependencyTracker) {
        return false;
    }

    return m_dependencyTracker->hasCrossSheetReferences();
}

QString WorksheetWidget::getCurrentSheetName() const
{
    // Find the parent MainWindow and get the tab name for this worksheet
    QWidget *parent = parentWidget();
    while (parent && !qobject_cast<QMainWindow*>(parent)) {
        parent = parent->parentWidget();
    }

    if (parent) {
        // Look for a CustomTabWidget in the parent
        CustomTabWidget *tabWidget = parent->findChild<CustomTabWidget*>();
        if (tabWidget) {
            // Find the index of this worksheet
            for (int i = 0; i < tabWidget->count(); ++i) {
                if (tabWidget->widget(i) == this) {
                    return tabWidget->tabText(i);  // CustomTabWidget returns unescaped text
                }
            }
        }
    }

    return QString("Unknown");
}

void WorksheetWidget::handleCrossSheetBackgroundHighlighting(const QString &currentLineText)
{
    LOG_DEBUG(QString("handleCrossSheetBackgroundHighlighting called with text: '%1'").arg(currentLineText));

    // First, clear any existing cross-sheet highlighting on all sheets
    clearAllCrossSheetHighlighting();

    // Find cross-sheet references in the current line
    QRegularExpression crossSheetPattern(R"(\bS\.([^.]+)\.LN(\d+)\b)", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatchIterator iterator = crossSheetPattern.globalMatch(currentLineText);

    // Get the MainWindow to access other sheets
    QWidget *parent = parentWidget();
    while (parent && !qobject_cast<QMainWindow*>(parent)) {
        parent = parent->parentWidget();
    }

    if (!parent) {
        return; // Can't find MainWindow
    }

    // Look for a QTabWidget in the parent
    QTabWidget *tabWidget = parent->findChild<QTabWidget*>();
    if (!tabWidget) {
        LOG_DEBUG("Could not find QTabWidget in parent");
        return; // Can't find tab widget
    }

    LOG_DEBUG(QString("Found QTabWidget with %1 tabs").arg(tabWidget->count()));

    // Process each cross-sheet reference
    while (iterator.hasNext()) {
        QRegularExpressionMatch match = iterator.next();
        QString sheetName = match.captured(1).trimmed();
        int lineNumber = match.captured(2).toInt();

        LOG_DEBUG(QString("Found cross-sheet reference: S.%1.LN%2").arg(sheetName).arg(lineNumber));

        // Find the target sheet
        WorksheetWidget *targetSheet = nullptr;
        LOG_DEBUG(QString("Looking for sheet '%1' among %2 tabs").arg(sheetName).arg(tabWidget->count()));

        // Log all tab names for debugging
        QStringList allTabNames;
        for (int i = 0; i < tabWidget->count(); ++i) {
            allTabNames << QString("'%1'").arg(tabWidget->tabText(i));
        }
        LOG_DEBUG(QString("All tabs: %1").arg(allTabNames.join(", ")));

        for (int i = 0; i < tabWidget->count(); ++i) {
            QString tabName = tabWidget->tabText(i);  // CustomTabWidget returns unescaped text
            LOG_DEBUG(QString("Tab %1: '%2' (comparing with '%3')").arg(i).arg(tabName).arg(sheetName));
            if (tabName.compare(sheetName, Qt::CaseInsensitive) == 0) {
                targetSheet = qobject_cast<WorksheetWidget*>(tabWidget->widget(i));
                LOG_DEBUG(QString("Found matching sheet: '%1'").arg(tabName));
                break;
            }
        }

        if (targetSheet && targetSheet != this) {
            LOG_DEBUG(QString("Found target sheet '%1', highlighting line %2").arg(sheetName).arg(lineNumber));

            // Get the LN color for this line number
            QColor lnColor = m_editor->getSyntaxHighlighter()->getLNColor(lineNumber);

            // Highlight on both expression and results sides with the correct LN color
            targetSheet->getResults()->highlightSpecificLine(lineNumber, lnColor);
            targetSheet->getEditor()->highlightSpecificLine(lineNumber, lnColor);

            // Also update the target sheet's line number area
            if (targetSheet->getResults()->getLineNumberArea()) {
                targetSheet->getResults()->getLineNumberArea()->update();
            }
        } else {
            LOG_DEBUG(QString("Target sheet '%1' not found or is current sheet").arg(sheetName));
        }
    }
}

void WorksheetWidget::clearAllCrossSheetHighlighting()
{
    // Get the MainWindow to access other sheets
    QWidget *parent = parentWidget();
    while (parent && !qobject_cast<QMainWindow*>(parent)) {
        parent = parent->parentWidget();
    }

    if (!parent) {
        return; // Can't find MainWindow
    }

    // Look for a QTabWidget in the parent
    QTabWidget *tabWidget = parent->findChild<QTabWidget*>();
    if (!tabWidget) {
        return; // Can't find tab widget
    }

    // Clear cross-sheet highlighting on all sheets
    for (int i = 0; i < tabWidget->count(); ++i) {
        WorksheetWidget *worksheet = qobject_cast<WorksheetWidget*>(tabWidget->widget(i));
        if (worksheet) {
            // Clear highlighting on both results and expression sides
            if (worksheet->getResults()) {
                worksheet->getResults()->clearCrossSheetHighlighting();
            }
            if (worksheet->getEditor()) {
                worksheet->getEditor()->clearCrossSheetHighlighting();
            }
        }
    }
}

void WorksheetWidget::evaluateLines(const QStringList &currentLines, const QSet<int> &changedLines)
{
    // Determine if this is first load
    bool isFirstLoad = m_lastLines.isEmpty();

    // Update dependency tracking for ALL lines on first load, or just changed lines
    if (isFirstLoad) {
        // On first load, update dependencies for all lines
        for (int i = 0; i < currentLines.size(); ++i) {
            QString expression = currentLines[i];
            m_dependencyTracker->updateLineDependencies(i + 1, expression, currentLines.size());
            // Also update cross-sheet dependencies
            m_dependencyTracker->updateCrossSheetDependencies(i + 1, expression, "Unknown");
        }
    } else {
        // Update dependency tracking for changed lines only
        for (int lineNum : changedLines) {
            if (lineNum <= currentLines.size()) {
                QString expression = currentLines[lineNum - 1];  // Convert to 0-based index
                m_dependencyTracker->updateLineDependencies(lineNum, expression, currentLines.size());
                // Also update cross-sheet dependencies
                m_dependencyTracker->updateCrossSheetDependencies(lineNum, expression, "Unknown");
            }
        }
    }

    // Get all lines that need recalculation (changed + dependents)
    QSet<int> linesToRecalc = m_dependencyTracker->getLinesToRecalculate(changedLines, currentLines.size());

    // If we have existing results, start with them; otherwise create new results array
    QStringList results;
    if (m_lastLines.size() == currentLines.size()) {
        // Reuse existing results
        results = m_results->getResults();
    } else {
        // Size changed, create new results array
        results = QStringList();
        results.reserve(currentLines.size());
        for (int i = 0; i < currentLines.size(); ++i) {
            results.append("");
        }
        // Mark all lines for recalculation when size changes
        linesToRecalc.clear();
        for (int i = 1; i <= currentLines.size(); ++i) {
            linesToRecalc.insert(i);
        }
    }

    LOG_DEBUG(QString("Selective recalculation: %1 changed lines -> %2 lines to recalculate")
              .arg(changedLines.size()).arg(linesToRecalc.size()));

    // Get lines in dependency-safe evaluation order
    QList<int> evaluationOrder = m_dependencyTracker->getEvaluationOrder(linesToRecalc);

    LOG_DEBUG(QString("Evaluation order: %1 lines sorted by dependencies").arg(evaluationOrder.size()));

    // Evaluate lines in dependency order to ensure LN references work correctly
    for (int lineNum : evaluationOrder) {
        int index = lineNum - 1;  // Convert to 0-based index
        if (index >= 0 && index < currentLines.size()) {
            QString line = currentLines[index];

            if (line.trimmed().isEmpty()) {
                results[index] = "";
            } else if (m_calculationEngine->isCommentLine(line)) {
                results[index] = "";  // Comment lines show blank results
            } else {
                QString result = m_calculationEngine->evaluateExpression(line, lineNum);
                results[index] = result;
            }
        }
    }

    // Update results display
    m_results->setResults(results);

    // Cache current state for next comparison
    m_lastLines = currentLines;

    // Update both line number areas
    m_lineNumberArea->updateLineNumbers();
    m_resultsLineNumberArea->updateLineNumbers();

    // Emit signal that values in this sheet have changed (for cross-sheet recalculation)
    // Only emit if we actually recalculated some lines and we're not loading content
    if (!m_isLoadingContent && !linesToRecalc.isEmpty()) {
        QString currentSheetName = getCurrentSheetName();
        LOG_DEBUG(QString("Emitting valuesChanged signal for sheet '%1' after recalculating %2 lines")
                 .arg(currentSheetName).arg(linesToRecalc.size()));
        emit valuesChanged(currentSheetName);
    }
}

void WorksheetWidget::forceRecalculation()
{
    if (!m_calculationEngine || !m_dependencyTracker) {
        LOG_ERROR("Calculation engine or dependency tracker not initialized");
        return;
    }

    LOG_DEBUG("Force recalculation triggered");

    QString content = m_editor->toPlainText();
    QStringList currentLines = content.split('\n');

    // Force evaluation by treating all lines as changed
    QSet<int> changedLines;
    for (int i = 0; i < currentLines.size(); ++i) {
        changedLines.insert(i + 1);
    }

    // Continue with the normal evaluation process but skip change detection
    evaluateLines(currentLines, changedLines);
}

// Phase 4.1: Event system integration
void WorksheetWidget::setEventBus(EventBus* eventBus)
{
    m_eventBus = eventBus;
    LOG_DEBUG("WorksheetWidget: Event bus set");
}
