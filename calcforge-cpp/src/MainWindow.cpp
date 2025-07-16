#include "MainWindow.h"
#include "WorksheetWidget.h"
#include "ExpressionEditor.h"
#include "ResultsDisplay.h"
#include "CalculationEngine.h"
#include "SyntaxHighlighter.h"
#include "LineNumberArea.h"
#include "CurrencyConverter.h"
#include "LineChangeDetector.h"
#include "Logger.h"
#include <QApplication>
#include <limits>
#include <QMessageBox>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QStandardPaths>
#include <QDir>
#include <QIcon>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QTextBlock>
#include <QInputDialog>
#include <QLineEdit>
#include <QTabBar>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QCoreApplication>
#include <QDebug>
#include <QTimer>
#include <QFile>
#include <QTextStream>
#include <QDateTime>

// Simple logging function for debugging
void logDebug(const QString &message) {
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
               << " - " << message << "\n";
        logFile.close();
    }
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_centralWidget(nullptr)
    , m_mainLayout(nullptr)
    , m_topLayout(nullptr)
    , m_tabWidget(nullptr)
    , m_addButton(nullptr)
    , m_helpButton(nullptr)
    , m_bottomLeftCorner(nullptr)
    , m_bottomRightCorner(nullptr)
    , m_leftEdgeIndicator(nullptr)
    , m_rightEdgeIndicator(nullptr)
    , m_bottomEdgeIndicator(nullptr)
    , m_menuBar(nullptr)
    , m_toolBar(nullptr)
    , m_statusBar(nullptr)
    , m_settings(nullptr)
    , m_increaseFontShortcut(nullptr)
    , m_decreaseFontShortcut(nullptr)
    , m_resetFontShortcut(nullptr)
    , m_isModified(false)
    , m_dragging(false)
    , m_resizing(false)
    , m_resizeEdges(Qt::Edges())
    , m_hasNavigationHistory(false)
{
    // Initialize settings
    m_settings = new QSettings("CalcForge", "CalcForge", this);

    // Initialize global font size (default pixel size)
    m_globalFontSize = 17; // Default font size (15 base + 2 for pixel adjustment)
    
    // Set window properties for custom styling
    setWindowTitle("CalcForge v4.0");
    setMinimumSize(800, 600);

    // Enable frameless window for custom title bar but allow resizing
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);

    // Enable mouse tracking for resize functionality
    setMouseTracking(true);
    setAttribute(Qt::WA_Hover, true);

    // Ensure the window can be resized
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // Set application icon
    setWindowIcon(QIcon(":/icons/calcforge.ico"));
    
    // Setup UI
    setupUI();
    // Remove menu bar, toolbar, and status bar for frameless design
    // setupMenuBar();
    // setupToolBar();
    // setupStatusBar();

    // Setup keyboard shortcuts
    setupShortcuts();

    // Install global event filter to catch font size shortcuts
    qApp->installEventFilter(this);
    
    // Restore window state
    restoreWindowState();

    // Load existing worksheets or create initial tab if none exist
    loadWorksheets();

    // Ensure we have at least one tab
    if (m_tabWidget->count() == 0) {
        createInitialTab();
    }
}

MainWindow::~MainWindow()
{
    saveWindowState();
    saveWorksheets();
}

void MainWindow::setupUI()
{
    // Set main window styling to match GitHub dark theme exactly
    setStyleSheet(
        "QMainWindow {"
        "  background-color: #0D1117;"
        "  color: #ffffff;"
        "  border: 1px solid #30363D;"
        "}"
        "QWidget {"
        "  background-color: #0D1117;"
        "  color: #ffffff;"
        "  font-family: Inter, 'Noto Sans', sans-serif;"
        "}"
    );

    // Create central widget and main layout
    m_centralWidget = new QWidget(this);
    setCentralWidget(m_centralWidget);

    // Install event filter to handle resize events properly
    m_centralWidget->installEventFilter(this);

    m_mainLayout = new QVBoxLayout(m_centralWidget);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    // Create custom header bar with integrated tabs (matching HTML mockup)
    QWidget *headerWidget = new QWidget(this);
    headerWidget->setStyleSheet(
        "QWidget {"
        "  background-color: #161B22;"
        "  border-bottom: 1px solid #30363D;"
        "}"
    );
    headerWidget->setFixedHeight(45); // Even more compact height

    QVBoxLayout *headerMainLayout = new QVBoxLayout(headerWidget);
    headerMainLayout->setContentsMargins(16, 2, 16, 1);
    headerMainLayout->setSpacing(1);

    // Top row: Title and control buttons
    QHBoxLayout *headerTopLayout = new QHBoxLayout();
    headerTopLayout->setSpacing(8);

    // Title label (left side)
    QLabel *titleLabel = new QLabel("CalcForge v4.0", this);
    titleLabel->setStyleSheet(
        "QLabel {"
        "  color: #ffffff;"
        "  font-weight: 600;"
        "  font-size: 14px;"
        "  background-color: transparent;"
        "}"
    );

    // Control buttons layout (right side)
    m_topLayout = new QHBoxLayout();
    m_topLayout->setSpacing(8);

    // Add button (styled like HTML mockup)
    m_addButton = new QPushButton(this);
    m_addButton->setFixedSize(28, 28);
    m_addButton->setToolTip("New Tab");
    m_addButton->setStyleSheet(
        "QPushButton {"
        "  background-color: #21262D;"
        "  color: #0c7ff2;"
        "  border: none;"
        "  border-radius: 6px;"
        "  font-size: 16px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background-color: #30363D;"
        "}"
        "QPushButton:focus {"
        "  outline: 2px solid #0c7ff2;"
        "}"
    );
    m_addButton->setText("+");
    connect(m_addButton, &QPushButton::clicked, this, &MainWindow::addTab);

    // Currency update button (styled like HTML mockup)
    m_currencyButton = new QPushButton("$", this);
    m_currencyButton->setFixedSize(28, 28);
    m_currencyButton->setToolTip("Update Exchange Rates");
    m_currencyButton->setStyleSheet(
        "QPushButton {"
        "  background-color: #21262D;"
        "  color: #00d084;"  // Green color for currency
        "  border: none;"
        "  border-radius: 6px;"
        "  font-size: 16px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background-color: #30363D;"
        "}"
        "QPushButton:focus {"
        "  outline: 2px solid #00d084;"
        "}"
    );
    connect(m_currencyButton, &QPushButton::clicked, this, &MainWindow::updateCurrencyRates);

    // Help button (styled like HTML mockup)
    m_helpButton = new QPushButton("?", this);
    m_helpButton->setFixedSize(28, 28);
    m_helpButton->setToolTip("Help");
    m_helpButton->setStyleSheet(
        "QPushButton {"
        "  background-color: #21262D;"
        "  color: #ffffff;"
        "  border: none;"
        "  border-radius: 6px;"
        "  font-size: 14px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background-color: #30363D;"
        "}"
        "QPushButton:focus {"
        "  outline: 2px solid #0c7ff2;"
        "}"
    );
    connect(m_helpButton, &QPushButton::clicked, this, &MainWindow::showHelp);

    // Close button (styled like HTML mockup)
    QPushButton *closeButton = new QPushButton("×", this);
    closeButton->setFixedSize(28, 28);
    closeButton->setToolTip("Close");
    closeButton->setStyleSheet(
        "QPushButton {"
        "  background-color: #21262D;"
        "  color: #ffffff;"
        "  border: none;"
        "  border-radius: 6px;"
        "  font-size: 16px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background-color: #da3633;"
        "  color: #ffffff;"
        "}"
        "QPushButton:focus {"
        "  outline: 2px solid #da3633;"
        "}"
    );
    connect(closeButton, &QPushButton::clicked, this, &QWidget::close);

    // Add control buttons to header
    m_topLayout->addWidget(m_addButton);
    m_topLayout->addWidget(m_currencyButton);
    m_topLayout->addWidget(m_helpButton);
    m_topLayout->addWidget(closeButton);

    // Assemble top row of header
    headerTopLayout->addWidget(titleLabel);
    headerTopLayout->addStretch();
    headerTopLayout->addLayout(m_topLayout);

    // Bottom row: Tab navigation (integrated into header)
    QHBoxLayout *tabRowLayout = new QHBoxLayout();
    tabRowLayout->setContentsMargins(0, 0, 0, 0);
    tabRowLayout->setSpacing(0);

    // Create tab widget with custom styling (no separate tab bar widget)
    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setTabsClosable(false); // We'll create custom close buttons
    m_tabWidget->setMovable(true);

    // Style the tab widget to match HTML mockup exactly
    m_tabWidget->setStyleSheet(
        "QTabWidget {"
        "  background-color: #0D1117;"
        "  border: none;"
        "}"
        "QTabWidget::pane {"
        "  border: none;"
        "  background-color: #0D1117;"
        "  margin-top: 0px;"
        "}"
        "QTabWidget::tab-bar {"
        "  alignment: left;"
        "  background-color: transparent;"
        "  left: 15px;"  // Move tab bar 15px from left edge
        "}"
        "QTabBar {"
        "  background-color: transparent;"
        "  border: none;"
        "}"
        "QTabBar::tab {"
        "  background-color: #21262D;"
        "  color: #9CA3AF;"
        "  border: none;"
        "  border-radius: 6px;"
        "  padding: 6px 4px 6px 12px;"  // Minimal right padding to bring button closer to text
        "  margin-right: 8px;"
        "  margin-top: 4px;"
        "  margin-bottom: 4px;"
        "  font-family: Inter, 'Segoe UI', Arial, sans-serif;"  // Match line numbers font
        "  font-size: 14px;"  // Larger size for better readability
        "  font-weight: 500;"
        "  min-width: 60px;"  // Back to original min-width
        "}"
        "QTabBar::tab:selected {"
        "  background-color: #21262D;"
        "  color: #ffffff;"
        "  border: 2px solid #0c7ff2;"
        "}"
        "QTabBar::tab:hover {"
        "  background-color: #30363D;"
        "  color: #ffffff;"
        "}"

    );

    // Create a custom close button icon
    QTabBar *tabBar = m_tabWidget->tabBar();
    if (tabBar) {
        // Force close buttons to be visible and styled
        tabBar->setTabsClosable(true);
    }

    connect(m_tabWidget, &QTabWidget::tabBarDoubleClicked, this, &MainWindow::renameTab);
    connect(m_tabWidget, &QTabWidget::currentChanged, this, &MainWindow::onTabChanged);

    // Create a container for the tab bar
    QWidget *tabBarContainer = new QWidget(this);
    tabBarContainer->setStyleSheet("QWidget { background-color: transparent; }");
    QHBoxLayout *tabBarLayout = new QHBoxLayout(tabBarContainer);
    tabBarLayout->setContentsMargins(0, 0, 0, 0);
    tabBarLayout->setSpacing(0);

    // Add tab bar container to the tab row in header
    tabRowLayout->addWidget(tabBarContainer);
    tabRowLayout->addStretch();

    // Assemble header layout
    headerMainLayout->addLayout(headerTopLayout);
    headerMainLayout->addLayout(tabRowLayout);

    // Add components to main layout
    m_mainLayout->addWidget(headerWidget);
    m_mainLayout->addWidget(m_tabWidget, 1); // Give tab widget all remaining space

    // Add corner resize indicators for better UX
    createResizeCorners();

    // Add edge resize indicators for single-axis resizing
    createResizeEdges();
}

void MainWindow::setupMenuBar()
{
    m_menuBar = menuBar();
    
    // File menu
    QMenu *fileMenu = m_menuBar->addMenu("&File");
    
    QAction *newAction = fileMenu->addAction("&New", this, &MainWindow::newFile);
    newAction->setShortcut(QKeySequence::New);
    
    QAction *openAction = fileMenu->addAction("&Open...", this, &MainWindow::openFile);
    openAction->setShortcut(QKeySequence::Open);
    
    fileMenu->addSeparator();
    
    QAction *saveAction = fileMenu->addAction("&Save", this, &MainWindow::saveFile);
    saveAction->setShortcut(QKeySequence::Save);
    
    QAction *saveAsAction = fileMenu->addAction("Save &As...", this, &MainWindow::saveAsFile);
    saveAsAction->setShortcut(QKeySequence::SaveAs);
    
    fileMenu->addSeparator();
    
    QAction *exitAction = fileMenu->addAction("E&xit", this, &MainWindow::exitApplication);
    exitAction->setShortcut(QKeySequence::Quit);
    
    // Help menu
    QMenu *helpMenu = m_menuBar->addMenu("&Help");
    helpMenu->addAction("&About CalcForge", this, &MainWindow::showAbout);
    helpMenu->addAction("&Help", this, &MainWindow::showHelp);
}

void MainWindow::setupToolBar()
{
    m_toolBar = addToolBar("Main");
    m_toolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    
    // Add common actions to toolbar
    m_toolBar->addAction("New", this, &MainWindow::addTab);
    m_toolBar->addSeparator();
    m_toolBar->addAction("Help", this, &MainWindow::showHelp);
}

void MainWindow::setupStatusBar()
{
    m_statusBar = statusBar();
    m_statusBar->showMessage("Ready", 2000);
}

void MainWindow::setupShortcuts()
{
    logDebug("Setting up font size shortcuts...");

    // Create font size shortcuts - only the three specified
    m_increaseFontShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Period), this); // Ctrl+.
    m_decreaseFontShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Comma), this); // Ctrl+,
    m_resetFontShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_0), this); // Ctrl+0

    logDebug(QString("Font increase shortcut (Ctrl+.): %1").arg(m_increaseFontShortcut->key().toString()));
    logDebug(QString("Font decrease shortcut (Ctrl+,): %1").arg(m_decreaseFontShortcut->key().toString()));
    logDebug(QString("Reset font shortcut (Ctrl+0): %1").arg(m_resetFontShortcut->key().toString()));

    // Log the key values for debugging
    logDebug(QString("Qt::Key_Period value: %1").arg(Qt::Key_Period));
    logDebug(QString("Qt::Key_Comma value: %1").arg(Qt::Key_Comma));

    // Connect shortcuts to methods
    connect(m_increaseFontShortcut, &QShortcut::activated, this, &MainWindow::increaseFontSize);
    connect(m_decreaseFontShortcut, &QShortcut::activated, this, &MainWindow::decreaseFontSize);
    connect(m_resetFontShortcut, &QShortcut::activated, this, &MainWindow::resetFontSize);

    // Add tab navigation shortcuts (Ctrl+PageUp/PageDown)
    QShortcut *previousTabShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_PageUp), this);
    QShortcut *nextTabShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_PageDown), this);

    connect(previousTabShortcut, &QShortcut::activated, this, &MainWindow::previousTab);
    connect(nextTabShortcut, &QShortcut::activated, this, &MainWindow::nextTab);

    // Add text selection shortcuts
    QShortcut *selectLineShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Down), this);
    QShortcut *smartParenthesesShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Up), this);

    connect(selectLineShortcut, &QShortcut::activated, this, &MainWindow::selectCurrentLine);
    connect(smartParenthesesShortcut, &QShortcut::activated, this, &MainWindow::smartParenthesesSelection);

    logDebug("Font size, tab navigation, and text selection shortcuts setup complete");
}

void MainWindow::createInitialTab()
{
    if (m_tabWidget->count() == 0) {
        addTab();
    }
}

void MainWindow::addTab()
{
    WorksheetWidget *worksheet = new WorksheetWidget(this);
    int index = m_tabWidget->addTab(worksheet, QString("Sheet %1").arg(m_tabWidget->count() + 1));
    m_tabWidget->setCurrentIndex(index);

    // Apply current global font size to new tab
    QFont editorFont = worksheet->getEditor()->font();
    QFont resultsFont = worksheet->getResults()->font();
    editorFont.setPixelSize(m_globalFontSize);
    resultsFont.setPixelSize(m_globalFontSize);
    worksheet->getEditor()->setFont(editorFont);
    worksheet->getResults()->setFont(resultsFont);

    // Connect font size signals from the expression editor
    connect(worksheet->getEditor(), &ExpressionEditor::fontSizeIncreaseRequested,
            this, &MainWindow::increaseFontSize);
    connect(worksheet->getEditor(), &ExpressionEditor::fontSizeDecreaseRequested,
            this, &MainWindow::decreaseFontSize);
    connect(worksheet->getEditor(), &ExpressionEditor::fontSizeResetRequested,
            this, &MainWindow::resetFontSize);

    // Create custom close button for this tab
    QTabBar *tabBar = m_tabWidget->tabBar();
    if (tabBar) {
        // Create a container widget to control positioning
        QWidget *buttonContainer = new QWidget(this);
        buttonContainer->setFixedSize(26, 20); // Even wider container for more right-side space

        QPushButton *closeButton = new QPushButton("×", buttonContainer);
        closeButton->setFixedSize(14, 14);  // Back to readable size
        closeButton->move(0, 3); // Position flush left for close spacing to text
        closeButton->setStyleSheet(
            "QPushButton {"
            "  background-color: #6B7280;"
            "  border: 1px solid #6B7280;"
            "  color: #ffffff;"
            "  font-family: 'Arial', sans-serif;"
            "  font-size: 9px;"
            "  font-weight: bold;"
            "  border-radius: 2px;"
            "  padding: 0px;"
            "  margin: 0px;"
            "}"
            "QPushButton:hover {"
            "  background-color: #da3633;"
            "  border-color: #da3633;"
            "}"
        );
        closeButton->setToolTip("Close tab");

        // Connect the button to close this specific tab
        connect(closeButton, &QPushButton::clicked, [this, buttonContainer]() {
            // Find which tab this close button belongs to
            QTabBar *tabBar = m_tabWidget->tabBar();
            for (int i = 0; i < tabBar->count(); ++i) {
                if (tabBar->tabButton(i, QTabBar::RightSide) == buttonContainer) {
                    closeTab(i);
                    break;
                }
            }
        });

        // Set the container as the tab button
        tabBar->setTabButton(index, QTabBar::RightSide, buttonContainer);
    }

    // Set up cross-sheet reference support
    setupCrossSheetSupport(worksheet);

    // Always set splitter state (either restored or default)
    LOG_DEBUG(QString("Setting splitter state for new tab - state size: %1 bytes").arg(m_splitterState.size()));
    worksheet->setSplitterState(m_splitterState);

    // Connect splitter changes to update global state and sync all tabs
    connect(worksheet, &WorksheetWidget::splitterMoved, this, &MainWindow::onSplitterMoved);

    // Connect line numbering changes for cross-sheet LN auto-updates
    connect(worksheet, &WorksheetWidget::lineNumberingChanged, this, &MainWindow::onLineNumberingChanged);

    // Connect value changes for cross-sheet recalculation
    connect(worksheet, &WorksheetWidget::valuesChanged, this, &MainWindow::onValuesChanged);

    // Focus on the new worksheet
    worksheet->getEditor()->setFocus();
}

void MainWindow::closeTab(int index)
{
    if (m_tabWidget->count() <= 1) {
        // Don't close the last tab, just clear it
        WorksheetWidget *worksheet = qobject_cast<WorksheetWidget*>(m_tabWidget->widget(index));
        if (worksheet) {
            worksheet->setContent("");
        }
        return;
    }
    
    WorksheetWidget *worksheet = qobject_cast<WorksheetWidget*>(m_tabWidget->widget(index));
    if (worksheet && worksheet->isModified()) {
        int ret = QMessageBox::question(this, "Close Tab",
                                      "The worksheet has unsaved changes. Close anyway?",
                                      QMessageBox::Yes | QMessageBox::No,
                                      QMessageBox::No);
        if (ret == QMessageBox::No) {
            return;
        }
    }
    
    m_tabWidget->removeTab(index);
}

void MainWindow::renameTab(int index)
{
    if (index < 0 || index >= m_tabWidget->count()) {
        return;
    }

    QString currentName = m_tabWidget->tabText(index);

    bool ok;
    QString newName = QInputDialog::getText(this,
                                          "Rename Sheet",
                                          "New name:",
                                          QLineEdit::Normal,
                                          currentName,
                                          &ok);

    if (ok && !newName.isEmpty() && newName != currentName) {
        m_tabWidget->setTabText(index, newName);

        // Mark as modified if needed
        WorksheetWidget *worksheet = qobject_cast<WorksheetWidget*>(m_tabWidget->widget(index));
        if (worksheet) {
            worksheet->setModified(true);
        }
    }
}

void MainWindow::onTabChanged(int index)
{
    Q_UNUSED(index)
    // Apply the global splitter state to the newly selected tab
    WorksheetWidget *worksheet = qobject_cast<WorksheetWidget*>(m_tabWidget->currentWidget());
    if (worksheet) {
        // Apply the current global splitter state to this tab
        worksheet->setSplitterState(m_splitterState);
        LOG_DEBUG(QString("onTabChanged: Applied global splitter state '%1' to new tab")
                  .arg(QString::fromUtf8(m_splitterState)));

        // Reset horizontal scroll position to leftmost when switching tabs
        // Use QTimer::singleShot to ensure this happens after the tab is fully activated
        QTimer::singleShot(0, [worksheet]() {
            if (worksheet) {  // Check if worksheet is still valid
                worksheet->getEditor()->horizontalScrollBar()->setValue(0);
                worksheet->getResults()->horizontalScrollBar()->setValue(0);

                // Trigger initial highlighting for the current tab
                int currentLine = worksheet->getEditor()->getCurrentLineNumber();
                QString currentLineText = worksheet->getEditor()->textCursor().block().text();

                // Highlight local LN references on the current tab
                worksheet->getResults()->highlightCurrentLineWithLNReferences(currentLine, currentLineText);

                // NOTE: Cross-sheet background highlighting is now disabled for automatic tab changes
                // It's only triggered by Shift+Enter navigation, not automatic cursor movement
                // worksheet->handleCrossSheetBackgroundHighlighting(currentLineText);

                LOG_DEBUG(QString("onTabChanged: Skipped automatic cross-sheet highlighting for line %1: '%2'")
                          .arg(currentLine).arg(currentLineText));
            }
        });

        // NOTE: Incoming cross-sheet highlighting is now disabled for automatic tab changes
        // It's only triggered by Shift+Enter navigation, not automatic cursor movement
        // QTimer::singleShot(10, [this, worksheet]() {
        //     if (worksheet) {
        //         highlightIncomingCrossSheetReferences(worksheet);
        //         LOG_DEBUG("onTabChanged: Triggered incoming cross-sheet highlighting");
        //     }
        // });
    }
}

void MainWindow::onSplitterMoved(const QByteArray &newState)
{
    // Update the global splitter state
    m_splitterState = newState;
    LOG_DEBUG(QString("Global splitter state updated to: '%1'").arg(QString::fromUtf8(newState)));

    // Apply the new state to all other tabs (except the one that triggered this)
    WorksheetWidget *senderWorksheet = qobject_cast<WorksheetWidget*>(sender());
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        WorksheetWidget *worksheet = qobject_cast<WorksheetWidget*>(m_tabWidget->widget(i));
        if (worksheet && worksheet != senderWorksheet) {
            worksheet->setSplitterState(newState);
            LOG_DEBUG(QString("Applied new splitter state to tab %1").arg(i));
        }
    }
}

void MainWindow::showHelp()
{
    QMessageBox::information(this, "CalcForge Help",
                           "CalcForge v4.0 - Advanced Calculator\n\n"
                           "Features:\n"
                           "• Mathematical expressions\n"
                           "• Unit conversions\n"
                           "• Currency conversions\n"
                           "• Cross-sheet references\n"
                           "• Line number variables (LN1, LN2, etc.)\n\n"
                           "Keyboard Shortcuts:\n"
                           "• Ctrl+Plus: Increase font size\n"
                           "• Ctrl+Minus: Decrease font size\n"
                           "• Ctrl+N: New file\n"
                           "• Ctrl+O: Open file\n"
                           "• Ctrl+S: Save file\n\n"
                           "Currency Conversions:\n"
                           "• Use format: '100 dollars to euros'\n"
                           "• Click $ button to update exchange rates");
}

void MainWindow::updateCurrencyRates()
{
    // Create a temporary currency converter to update rates
    CurrencyConverter converter;

    // Disable the button during update
    m_currencyButton->setEnabled(false);
    m_currencyButton->setText("...");

    // Update rates from API
    bool success = converter.updateExchangeRatesFromAPI();

    // Re-enable the button
    m_currencyButton->setEnabled(true);
    m_currencyButton->setText("$");

    // Show result message
    if (success) {
        QMessageBox::information(this, "Exchange Rates Updated",
                               "Exchange rates have been successfully updated from the latest API data!");

        // Trigger recalculation of all worksheets to use new rates
        for (int i = 0; i < m_tabWidget->count(); ++i) {
            WorksheetWidget *worksheet = qobject_cast<WorksheetWidget*>(m_tabWidget->widget(i));
            if (worksheet) {
                worksheet->evaluateAndHighlight();
            }
        }
    } else {
        QMessageBox::warning(this, "Update Failed",
                           "Failed to update exchange rates. Please check your internet connection and try again.\n\n"
                           "Currency conversions will continue using the existing rates.");
    }
}

void MainWindow::toggleStayOnTop(bool enabled)
{
    Qt::WindowFlags flags = windowFlags();
    if (enabled) {
        flags |= Qt::WindowStaysOnTopHint;
    } else {
        flags &= ~Qt::WindowStaysOnTopHint;
    }
    setWindowFlags(flags);
    show(); // Need to show again after changing flags
}

void MainWindow::newFile()
{
    addTab();
}

void MainWindow::openFile()
{
    // TODO: Implement file opening
    QMessageBox::information(this, "Open File", "File opening not yet implemented");
}

void MainWindow::saveFile()
{
    // TODO: Implement file saving
    QMessageBox::information(this, "Save File", "File saving not yet implemented");
}

void MainWindow::saveAsFile()
{
    // TODO: Implement save as
    QMessageBox::information(this, "Save As", "Save As not yet implemented");
}

void MainWindow::exitApplication()
{
    close();
}

void MainWindow::showAbout()
{
    QMessageBox::about(this, "About CalcForge",
                      "CalcForge v4.0\n\n"
                      "Advanced Calculator with Mathematical Operations and Unit Conversion\n\n"
                      "Built with Qt 6 and C++ for maximum performance\n"
                      "Copyright © 2024 CalcForge Project");
}

void MainWindow::loadWorksheets()
{
    // Get the path to worksheets.json in the same directory as the executable
    QString appDir = QCoreApplication::applicationDirPath();
    QString worksheetsPath = QDir(appDir).absoluteFilePath("worksheets.json");

    QFile file(worksheetsPath);
    if (!file.exists()) {
        // No saved worksheets, keep the default tab
        return;
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open worksheets.json for reading:" << file.errorString();
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "Failed to parse worksheets.json:" << parseError.errorString();
        return;
    }

    if (!doc.isObject()) {
        qWarning() << "Invalid worksheets.json format: not an object";
        return;
    }

    QJsonObject root = doc.object();
    if (root.isEmpty()) {
        // Empty file, keep default tab
        return;
    }

    // Clear existing tabs
    while (m_tabWidget->count() > 0) {
        m_tabWidget->removeTab(0);
    }

    // Check if this is the new format (version 2.0 with tabs array)
    if (root.contains("version") && root.contains("tabs") && root["tabs"].isArray()) {
        // New format: load tabs in order from array
        QJsonArray tabsArray = root["tabs"].toArray();

        for (const QJsonValue &tabValue : tabsArray) {
            if (!tabValue.isObject()) {
                continue; // Skip invalid entries
            }

            QJsonObject tabObject = tabValue.toObject();
            if (!tabObject.contains("name") || !tabObject.contains("content")) {
                continue; // Skip incomplete entries
            }

            QString tabName = tabObject["name"].toString();
            QString content = tabObject["content"].toString();

            loadSingleWorksheet(tabName, content);
        }
    } else {
        // Legacy format: load tabs from object keys (order not preserved)
        for (auto it = root.begin(); it != root.end(); ++it) {
            QString tabName = it.key();
            QString content = it.value().toString();

            loadSingleWorksheet(tabName, content);
        }
    }

    // After all worksheets are loaded, trigger coordinated cross-sheet calculation
    if (m_tabWidget->count() > 0) {
        recalculateAllWorksheets();
    }

    // Set focus to first tab if any tabs were loaded
    if (m_tabWidget->count() > 0) {
        m_tabWidget->setCurrentIndex(0);
        WorksheetWidget *firstWorksheet = qobject_cast<WorksheetWidget*>(m_tabWidget->widget(0));
        if (firstWorksheet) {
            firstWorksheet->getEditor()->setFocus();

            // Ensure horizontal scroll positions start at leftmost for initial load
            QTimer::singleShot(10, [firstWorksheet]() {
                if (firstWorksheet) {  // Check if worksheet is still valid
                    firstWorksheet->getEditor()->horizontalScrollBar()->setValue(0);
                    firstWorksheet->getResults()->horizontalScrollBar()->setValue(0);

                    // Trigger initial cross-sheet highlighting for current cursor position on app startup
                    // This ensures cross-sheet references are highlighted immediately when app loads
                    int currentLine = firstWorksheet->getEditor()->getCurrentLineNumber();
                    QString currentLineText = firstWorksheet->getEditor()->textCursor().block().text();

                    // Trigger the same highlighting logic as cursor position changes
                    firstWorksheet->getResults()->highlightCurrentLineWithLNReferences(currentLine, currentLineText);
                    // NOTE: Cross-sheet background highlighting is now disabled for automatic startup
                    // firstWorksheet->handleCrossSheetBackgroundHighlighting(currentLineText);

                    LOG_DEBUG(QString("App startup: Triggered initial highlighting for line %1: '%2' (cross-sheet disabled)")
                              .arg(currentLine).arg(currentLineText));
                }
            });
        }
    }
}

void MainWindow::loadSingleWorksheet(const QString &tabName, const QString &content)
{
    // Create new worksheet
    WorksheetWidget *worksheet = new WorksheetWidget(this);
    worksheet->setContent(content);

    // Apply current global font size to loaded tab
    QFont editorFont = worksheet->getEditor()->font();
    QFont resultsFont = worksheet->getResults()->font();
    editorFont.setPixelSize(m_globalFontSize);
    resultsFont.setPixelSize(m_globalFontSize);
    worksheet->getEditor()->setFont(editorFont);
    worksheet->getResults()->setFont(resultsFont);

    int index = m_tabWidget->addTab(worksheet, tabName);

    // Connect font size signals from the expression editor
    connect(worksheet->getEditor(), &ExpressionEditor::fontSizeIncreaseRequested,
            this, &MainWindow::increaseFontSize);
    connect(worksheet->getEditor(), &ExpressionEditor::fontSizeDecreaseRequested,
            this, &MainWindow::decreaseFontSize);
    connect(worksheet->getEditor(), &ExpressionEditor::fontSizeResetRequested,
            this, &MainWindow::resetFontSize);

    // Create custom close button for this tab (same as addTab method)
    QTabBar *tabBar = m_tabWidget->tabBar();
    if (tabBar) {
        QWidget *buttonContainer = new QWidget(this);
        buttonContainer->setFixedSize(26, 20);

        QHBoxLayout *buttonLayout = new QHBoxLayout(buttonContainer);
        buttonLayout->setContentsMargins(0, 0, 8, 0);
        buttonLayout->setSpacing(0);
        buttonLayout->addStretch();

        QPushButton *closeButton = new QPushButton("×", buttonContainer);
        closeButton->setFixedSize(12, 12);
        closeButton->setStyleSheet(
            "QPushButton {"
            "  background-color: transparent;"
            "  color: #9CA3AF;"
            "  border: none;"
            "  font-size: 10px;"
            "  font-weight: bold;"
            "  border-radius: 2px;"
            "}"
            "QPushButton:hover {"
            "  background-color: #da3633;"
            "  color: #ffffff;"
            "}"
        );
        closeButton->setToolTip("Close tab");

        connect(closeButton, &QPushButton::clicked, [this, buttonContainer]() {
            QTabBar *tabBar = m_tabWidget->tabBar();
            for (int i = 0; i < tabBar->count(); ++i) {
                if (tabBar->tabButton(i, QTabBar::RightSide) == buttonContainer) {
                    closeTab(i);
                    break;
                }
            }
        });

        buttonLayout->addWidget(closeButton);
        tabBar->setTabButton(index, QTabBar::RightSide, buttonContainer);
    }

    // Set up cross-sheet reference support
    setupCrossSheetSupport(worksheet);

    // Always set splitter state (either restored or default)
    LOG_DEBUG(QString("Setting splitter state for loaded worksheet - state size: %1 bytes, content: '%2'")
              .arg(m_splitterState.size()).arg(QString::fromUtf8(m_splitterState)));
    worksheet->setSplitterState(m_splitterState);

    // Connect splitter changes to update global state and sync all tabs
    connect(worksheet, &WorksheetWidget::splitterMoved, this, &MainWindow::onSplitterMoved);

    // Connect line numbering changes for cross-sheet LN auto-updates
    connect(worksheet, &WorksheetWidget::lineNumberingChanged, this, &MainWindow::onLineNumberingChanged);

    // Connect value changes for cross-sheet recalculation
    connect(worksheet, &WorksheetWidget::valuesChanged, this, &MainWindow::onValuesChanged);
}

void MainWindow::saveWorksheets()
{
    // Get the path to worksheets.json in the same directory as the executable
    QString appDir = QCoreApplication::applicationDirPath();
    QString worksheetsPath = QDir(appDir).absoluteFilePath("worksheets.json");

    // Create new format with version and ordered tabs array
    QJsonObject root;
    root["version"] = "2.0";

    QJsonArray tabsArray;

    // Collect data from all tabs in their current order
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        QString tabName = m_tabWidget->tabText(i);
        WorksheetWidget *worksheet = qobject_cast<WorksheetWidget*>(m_tabWidget->widget(i));

        if (worksheet) {
            QString content = worksheet->getContent();

            // Create tab object with name and content
            QJsonObject tabObject;
            tabObject["name"] = tabName;
            tabObject["content"] = content;

            tabsArray.append(tabObject);
        }
    }

    root["tabs"] = tabsArray;

    // Create JSON document
    QJsonDocument doc(root);

    // Write to file
    QFile file(worksheetsPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Failed to open worksheets.json for writing:" << file.errorString();
        return;
    }

    QByteArray jsonData = doc.toJson(QJsonDocument::Indented);
    qint64 bytesWritten = file.write(jsonData);
    file.close();

    if (bytesWritten == -1) {
        qWarning() << "Failed to write worksheets.json:" << file.errorString();
    }
}

void MainWindow::restoreWindowState()
{
    if (m_settings->contains("geometry")) {
        restoreGeometry(m_settings->value("geometry").toByteArray());
    }

    if (m_settings->contains("splitterState")) {
        m_splitterState = m_settings->value("splitterState").toByteArray();
        LOG_DEBUG(QString("Restored custom splitter state from settings - size: %1 bytes, content: '%2'")
                  .arg(m_splitterState.size()).arg(QString::fromUtf8(m_splitterState)));
    } else {
        LOG_DEBUG("No saved splitter state found in settings");
    }

    // Stay on top functionality removed to match HTML design
}

void MainWindow::saveWindowState()
{
    m_settings->setValue("geometry", saveGeometry());
    m_settings->setValue("splitterState", m_splitterState);
    LOG_DEBUG(QString("Saved splitter state to settings - size: %1 bytes").arg(m_splitterState.size()));
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    // Capture current tab's splitter state before saving
    WorksheetWidget *currentWorksheet = qobject_cast<WorksheetWidget*>(m_tabWidget->currentWidget());
    if (currentWorksheet) {
        m_splitterState = currentWorksheet->getSplitterState();
        LOG_DEBUG(QString("Captured custom splitter state from current worksheet on close - size: %1 bytes, content: '%2'")
                  .arg(m_splitterState.size()).arg(QString::fromUtf8(m_splitterState)));
    } else {
        LOG_DEBUG("No current worksheet found during close event");
    }

    saveWindowState();
    saveWorksheets();
    QMainWindow::closeEvent(event);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    updateCornerPositions();
    updateEdgePositions();
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_resizeEdges = getResizeEdges(event->pos());
        if (m_resizeEdges != Qt::Edges()) {
            m_resizing = true;
        } else {
            m_dragging = true;
            m_dragPosition = event->globalPosition().toPoint() - frameGeometry().topLeft();
        }
        event->accept();
    }
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        if (m_resizing) {
            // Handle window resizing
            QRect geometry = this->geometry();
            QPoint globalPos = event->globalPosition().toPoint();

            if (m_resizeEdges & Qt::LeftEdge) {
                int newWidth = geometry.right() - globalPos.x();
                if (newWidth >= minimumWidth()) {
                    geometry.setLeft(globalPos.x());
                }
            }
            if (m_resizeEdges & Qt::RightEdge) {
                geometry.setRight(globalPos.x());
            }
            if (m_resizeEdges & Qt::TopEdge) {
                int newHeight = geometry.bottom() - globalPos.y();
                if (newHeight >= minimumHeight()) {
                    geometry.setTop(globalPos.y());
                }
            }
            if (m_resizeEdges & Qt::BottomEdge) {
                geometry.setBottom(globalPos.y());
            }

            setGeometry(geometry);
        } else if (m_dragging) {
            // Handle window dragging
            move(event->globalPosition().toPoint() - m_dragPosition);
        }
        event->accept();
    } else {
        // Update cursor based on position when not dragging
        Qt::Edges edges = getResizeEdges(event->pos());
        if (edges & (Qt::LeftEdge | Qt::RightEdge) && edges & (Qt::TopEdge | Qt::BottomEdge)) {
            if ((edges & Qt::LeftEdge && edges & Qt::TopEdge) || (edges & Qt::RightEdge && edges & Qt::BottomEdge)) {
                setCursor(Qt::SizeFDiagCursor);
            } else {
                setCursor(Qt::SizeBDiagCursor);
            }
        } else if (edges & (Qt::LeftEdge | Qt::RightEdge)) {
            setCursor(Qt::SizeHorCursor);
        } else if (edges & (Qt::TopEdge | Qt::BottomEdge)) {
            setCursor(Qt::SizeVerCursor);
        } else {
            setCursor(Qt::ArrowCursor);
        }
    }
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
        m_resizing = false;
        m_resizeEdges = Qt::Edges();
        setCursor(Qt::ArrowCursor);
        event->accept();
    }
}

Qt::Edges MainWindow::getResizeEdges(const QPoint &pos) const
{
    Qt::Edges edges;
    const int margin = 8; // Resize margin in pixels

    if (pos.x() <= margin) {
        edges |= Qt::LeftEdge;
    }
    if (pos.x() >= width() - margin) {
        edges |= Qt::RightEdge;
    }
    if (pos.y() <= margin) {
        edges |= Qt::TopEdge;
    }
    if (pos.y() >= height() - margin) {
        edges |= Qt::BottomEdge;
    }

    return edges;
}



bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    // Handle global keyboard shortcuts for font size - ONLY for Ctrl key combinations
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);

        // ONLY handle Ctrl key combinations to avoid interfering with normal text input
        if (keyEvent->modifiers() & Qt::ControlModifier) {
            logDebug(QString("CTRL KEY DETECTED: Ctrl+%1 (key code: %2, text: '%3')")
                    .arg(QKeySequence(keyEvent->key()).toString())
                    .arg(keyEvent->key())
                    .arg(keyEvent->text()));

            if (keyEvent->key() == Qt::Key_Period || keyEvent->text() == ".") {
                logDebug("Triggering increaseFontSize from global event filter");
                increaseFontSize();
                return true; // Event handled
            } else if (keyEvent->key() == Qt::Key_Comma || keyEvent->text() == ",") {
                logDebug(QString("Triggering decreaseFontSize from global event filter (key: %1, text: '%2')").arg(keyEvent->key()).arg(keyEvent->text()));
                decreaseFontSize();
                return true; // Event handled
            } else if (keyEvent->key() == Qt::Key_0) {
                logDebug("Triggering resetFontSize from global event filter");
                resetFontSize();
                return true; // Event handled
            } else if (keyEvent->key() == Qt::Key_PageUp) {
                logDebug("Triggering previous tab from Ctrl+PageUp");
                previousTab();
                return true; // Event handled
            } else if (keyEvent->key() == Qt::Key_PageDown) {
                logDebug("Triggering next tab from Ctrl+PageDown");
                nextTab();
                return true; // Event handled
            }
        }

        // Don't log or interfere with non-Ctrl keys to avoid input issues
    }

    // Handle mouse events on corner resize indicators
    if (obj == m_bottomLeftCorner || obj == m_bottomRightCorner) {
        if (event->type() == QEvent::MouseButtonPress ||
            event->type() == QEvent::MouseMove ||
            event->type() == QEvent::MouseButtonRelease) {

            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
            QPoint globalPos = mouseEvent->globalPosition().toPoint();
            QPoint localPos = mapFromGlobal(globalPos);

            // Force diagonal resize edges based on which corner was clicked
            Qt::Edges forcedEdges;
            if (obj == m_bottomLeftCorner) {
                forcedEdges = Qt::LeftEdge | Qt::BottomEdge;  // Bottom-left diagonal
            } else {
                forcedEdges = Qt::RightEdge | Qt::BottomEdge; // Bottom-right diagonal
            }

            // Create a new mouse event with forced diagonal resize edges
            QMouseEvent newEvent(mouseEvent->type(), localPos, globalPos,
                               mouseEvent->button(), mouseEvent->buttons(),
                               mouseEvent->modifiers());

            // Handle the event in the main window with forced edges
            if (event->type() == QEvent::MouseButtonPress) {
                m_resizeEdges = forcedEdges;  // Force diagonal resize
                if (mouseEvent->button() == Qt::LeftButton) {
                    m_resizing = true;
                }
            } else if (event->type() == QEvent::MouseMove) {
                if (m_resizing) {
                    mouseMoveEvent(&newEvent);
                }
            } else if (event->type() == QEvent::MouseButtonRelease) {
                mouseReleaseEvent(&newEvent);
            }
            return true; // Event handled
        }
    }

    // Handle mouse events on edge resize indicators
    if (obj == m_leftEdgeIndicator || obj == m_rightEdgeIndicator || obj == m_bottomEdgeIndicator) {
        if (event->type() == QEvent::MouseButtonPress ||
            event->type() == QEvent::MouseMove ||
            event->type() == QEvent::MouseButtonRelease) {

            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
            QPoint globalPos = mouseEvent->globalPosition().toPoint();
            QPoint localPos = mapFromGlobal(globalPos);

            // Force single-axis resize edges based on which edge was clicked
            Qt::Edges forcedEdges;
            if (obj == m_leftEdgeIndicator) {
                forcedEdges = Qt::LeftEdge;  // Left edge only
            } else if (obj == m_rightEdgeIndicator) {
                forcedEdges = Qt::RightEdge; // Right edge only
            } else if (obj == m_bottomEdgeIndicator) {
                forcedEdges = Qt::BottomEdge; // Bottom edge only
            }

            // Create a new mouse event with forced single-axis resize edges
            QMouseEvent newEvent(mouseEvent->type(), localPos, globalPos,
                               mouseEvent->button(), mouseEvent->buttons(),
                               mouseEvent->modifiers());

            // Handle the event in the main window with forced edges
            if (event->type() == QEvent::MouseButtonPress) {
                m_resizeEdges = forcedEdges;  // Force single-axis resize
                if (mouseEvent->button() == Qt::LeftButton) {
                    m_resizing = true;
                }
            } else if (event->type() == QEvent::MouseMove) {
                if (m_resizing) {
                    mouseMoveEvent(&newEvent);
                }
            } else if (event->type() == QEvent::MouseButtonRelease) {
                mouseReleaseEvent(&newEvent);
            }
            return true; // Event handled
        }
    }

    // Handle mouse events on central widget for window resizing
    if (obj == m_centralWidget) {
        if (event->type() == QEvent::MouseButtonPress ||
            event->type() == QEvent::MouseMove ||
            event->type() == QEvent::MouseButtonRelease) {

            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

            // Check if we're near the window edges for resizing
            QPoint globalPos = mouseEvent->globalPosition().toPoint();
            QPoint localPos = mapFromGlobal(globalPos);
            Qt::Edges edges = getResizeEdges(localPos);

            // If near edges, let the main window handle the event
            if (edges != Qt::Edges()) {
                // Create a new mouse event with coordinates relative to main window
                QMouseEvent newEvent(mouseEvent->type(), localPos, globalPos,
                                   mouseEvent->button(), mouseEvent->buttons(),
                                   mouseEvent->modifiers());

                // Handle the event in the main window
                if (event->type() == QEvent::MouseButtonPress) {
                    mousePressEvent(&newEvent);
                } else if (event->type() == QEvent::MouseMove) {
                    mouseMoveEvent(&newEvent);
                } else if (event->type() == QEvent::MouseButtonRelease) {
                    mouseReleaseEvent(&newEvent);
                }
                return true; // Event handled
            }
        }
    }

    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::createResizeCorners()
{
    // Create bottom-left corner resize indicator
    m_bottomLeftCorner = new QWidget(this);
    m_bottomLeftCorner->setFixedSize(12, 12);
    m_bottomLeftCorner->setStyleSheet(
        "QWidget {"
        "  background-color: #484F58;"
        "  border: 1px solid #6B7280;"
        "  border-radius: 2px;"
        "}"
        "QWidget:hover {"
        "  background-color: #0c7ff2;"
        "  border: 1px solid #0969DA;"
        "}"
    );
    m_bottomLeftCorner->setCursor(Qt::SizeBDiagCursor);
    m_bottomLeftCorner->setToolTip("Drag to resize window");
    m_bottomLeftCorner->installEventFilter(this);  // Install event filter to handle resize

    // Create bottom-right corner resize indicator
    m_bottomRightCorner = new QWidget(this);
    m_bottomRightCorner->setFixedSize(12, 12);
    m_bottomRightCorner->setStyleSheet(
        "QWidget {"
        "  background-color: #484F58;"
        "  border: 1px solid #6B7280;"
        "  border-radius: 2px;"
        "}"
        "QWidget:hover {"
        "  background-color: #0c7ff2;"
        "  border: 1px solid #0969DA;"
        "}"
    );
    m_bottomRightCorner->setCursor(Qt::SizeFDiagCursor);
    m_bottomRightCorner->setToolTip("Drag to resize window");
    m_bottomRightCorner->installEventFilter(this);  // Install event filter to handle resize

    // Position corners (will be updated in resizeEvent)
    updateCornerPositions();
}

void MainWindow::updateCornerPositions()
{
    if (m_bottomLeftCorner && m_bottomRightCorner) {
        // Position bottom-left corner
        m_bottomLeftCorner->move(4, height() - 16);

        // Position bottom-right corner
        m_bottomRightCorner->move(width() - 16, height() - 16);
    }
}

void MainWindow::createResizeEdges()
{
    // Create left edge resize indicator
    m_leftEdgeIndicator = new QWidget(this);
    m_leftEdgeIndicator->setFixedSize(6, 150); // Thicker vertical line
    m_leftEdgeIndicator->setStyleSheet(
        "QWidget {"
        "  background-color: #484F58;"
        "  border: none;"
        "  border-radius: 1px;"
        "}"
        "QWidget:hover {"
        "  background-color: #0c7ff2;"
        "}"
    );
    m_leftEdgeIndicator->setCursor(Qt::SizeHorCursor);
    m_leftEdgeIndicator->setToolTip("Drag to resize window width");
    m_leftEdgeIndicator->installEventFilter(this);

    // Create right edge resize indicator
    m_rightEdgeIndicator = new QWidget(this);
    m_rightEdgeIndicator->setFixedSize(6, 150); // Thicker vertical line
    m_rightEdgeIndicator->setStyleSheet(
        "QWidget {"
        "  background-color: #484F58;"
        "  border: none;"
        "  border-radius: 1px;"
        "}"
        "QWidget:hover {"
        "  background-color: #0c7ff2;"
        "}"
    );
    m_rightEdgeIndicator->setCursor(Qt::SizeHorCursor);
    m_rightEdgeIndicator->setToolTip("Drag to resize window width");
    m_rightEdgeIndicator->installEventFilter(this);

    // Create bottom edge resize indicator
    m_bottomEdgeIndicator = new QWidget(this);
    m_bottomEdgeIndicator->setFixedSize(150, 6); // Thicker horizontal line
    m_bottomEdgeIndicator->setStyleSheet(
        "QWidget {"
        "  background-color: #484F58;"
        "  border: none;"
        "  border-radius: 1px;"
        "}"
        "QWidget:hover {"
        "  background-color: #0c7ff2;"
        "}"
    );
    m_bottomEdgeIndicator->setCursor(Qt::SizeVerCursor);
    m_bottomEdgeIndicator->setToolTip("Drag to resize window height");
    m_bottomEdgeIndicator->installEventFilter(this);

    // Position edges (will be updated in resizeEvent)
    updateEdgePositions();
}

void MainWindow::updateEdgePositions()
{
    if (m_leftEdgeIndicator && m_rightEdgeIndicator && m_bottomEdgeIndicator) {
        // Position left edge indicator (centered vertically on left edge)
        int leftY = (height() - m_leftEdgeIndicator->height()) / 2;
        m_leftEdgeIndicator->move(1, leftY);

        // Position right edge indicator (centered vertically on right edge)
        int rightY = (height() - m_rightEdgeIndicator->height()) / 2;
        m_rightEdgeIndicator->move(width() - 7, rightY);

        // Position bottom edge indicator (centered horizontally on bottom edge)
        int bottomX = (width() - m_bottomEdgeIndicator->width()) / 2;
        m_bottomEdgeIndicator->move(bottomX, height() - 7);
    }
}

void MainWindow::applyGlobalFontSize()
{
    // Apply global font size to all tabs
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        WorksheetWidget *worksheet = qobject_cast<WorksheetWidget*>(m_tabWidget->widget(i));
        if (worksheet) {
            // Set font size directly using pixel size
            QFont editorFont = worksheet->getEditor()->font();
            QFont resultsFont = worksheet->getResults()->font();

            editorFont.setPixelSize(m_globalFontSize);
            resultsFont.setPixelSize(m_globalFontSize);

            worksheet->getEditor()->setFont(editorFont);
            worksheet->getResults()->setFont(resultsFont);

            // Update line number areas by calling the existing methods
            worksheet->getEditor()->updateLineNumberAreaWidth();
            worksheet->getResults()->updateLineNumberAreaWidth();
        }
    }
}

void MainWindow::increaseFontSize()
{
    logDebug("MainWindow::increaseFontSize() called");

    // Increase global font size
    if (m_globalFontSize < 32) { // Maximum pixel size
        m_globalFontSize++;
        logDebug(QString("Global font size increased to %1").arg(m_globalFontSize));
        applyGlobalFontSize();
    }
}

void MainWindow::decreaseFontSize()
{
    logDebug("MainWindow::decreaseFontSize() called");

    // Decrease global font size
    if (m_globalFontSize > 8) { // Minimum pixel size
        m_globalFontSize--;
        logDebug(QString("Global font size decreased to %1").arg(m_globalFontSize));
        applyGlobalFontSize();
    }
}

void MainWindow::resetFontSize()
{
    logDebug("MainWindow::resetFontSize() called");

    // Reset global font size to default
    m_globalFontSize = 17; // Default font size (15 base + 2 for pixel adjustment)
    logDebug(QString("Global font size reset to %1").arg(m_globalFontSize));
    applyGlobalFontSize();
}

void MainWindow::selectCurrentLine()
{
    logDebug("MainWindow::selectCurrentLine() called");
    WorksheetWidget *worksheet = qobject_cast<WorksheetWidget*>(m_tabWidget->currentWidget());
    if (worksheet) {
        ExpressionEditor *editor = worksheet->getEditor();
        if (editor) {
            editor->selectCurrentLine();
        }
    }
}

void MainWindow::smartParenthesesSelection()
{
    logDebug("MainWindow::smartParenthesesSelection() called");
    WorksheetWidget *worksheet = qobject_cast<WorksheetWidget*>(m_tabWidget->currentWidget());
    if (worksheet) {
        ExpressionEditor *editor = worksheet->getEditor();
        if (editor) {
            editor->smartParenthesesSelection();
        }
    }
}

void MainWindow::previousTab()
{
    logDebug("MainWindow::previousTab() called");

    int currentIndex = m_tabWidget->currentIndex();
    int tabCount = m_tabWidget->count();

    if (tabCount <= 1) {
        logDebug("Only one tab or no tabs, cannot switch");
        return;
    }

    int newIndex = (currentIndex - 1 + tabCount) % tabCount; // Wrap around to last tab if at first
    m_tabWidget->setCurrentIndex(newIndex);

    logDebug(QString("Switched from tab %1 to tab %2").arg(currentIndex).arg(newIndex));
}

void MainWindow::nextTab()
{
    logDebug("MainWindow::nextTab() called");

    int currentIndex = m_tabWidget->currentIndex();
    int tabCount = m_tabWidget->count();

    if (tabCount <= 1) {
        logDebug("Only one tab or no tabs, cannot switch");
        return;
    }

    int newIndex = (currentIndex + 1) % tabCount; // Wrap around to first tab if at last
    m_tabWidget->setCurrentIndex(newIndex);

    logDebug(QString("Switched from tab %1 to tab %2").arg(currentIndex).arg(newIndex));
}

WorksheetWidget* MainWindow::getSheetByName(const QString &sheetName) const
{
    // Search through all tabs for a matching sheet name (case-insensitive)
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        QString tabName = m_tabWidget->tabText(i);
        if (tabName.compare(sheetName, Qt::CaseInsensitive) == 0) {
            return qobject_cast<WorksheetWidget*>(m_tabWidget->widget(i));
        }
    }
    return nullptr; // Sheet not found
}

QString MainWindow::getCurrentSheetName() const
{
    int currentIndex = m_tabWidget->currentIndex();
    if (currentIndex >= 0 && currentIndex < m_tabWidget->count()) {
        return m_tabWidget->tabText(currentIndex);
    }
    return QString(); // No current sheet
}

double MainWindow::getCrossSheetValue(const QString &sheetName, int lineNumber) const
{
    WorksheetWidget *targetSheet = getSheetByName(sheetName);
    if (targetSheet) {
        return targetSheet->getLineValue(lineNumber);
    }
    return std::numeric_limits<double>::quiet_NaN(); // Sheet not found
}

int MainWindow::getTabCount() const
{
    return m_tabWidget ? m_tabWidget->count() : 0;
}

QString MainWindow::getTabName(int index) const
{
    if (m_tabWidget && index >= 0 && index < m_tabWidget->count()) {
        return m_tabWidget->tabText(index);
    }
    return QString();
}

void MainWindow::navigateToSheet(const QString &sheetName, int lineNumber, int cursorPosition)
{
    LOG_DEBUG(QString("=== MainWindow::navigateToSheet ==="));
    LOG_DEBUG(QString("  Target: sheet='%1', line=%2, position=%3")
              .arg(sheetName).arg(lineNumber).arg(cursorPosition));

    // Find the target sheet
    WorksheetWidget *targetSheet = getSheetByName(sheetName);
    if (!targetSheet) {
        LOG_DEBUG(QString("  Sheet '%1' not found").arg(sheetName));
        return;
    }

    // Switch to the target tab
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        if (m_tabWidget->widget(i) == targetSheet) {
            LOG_DEBUG(QString("  Switching to tab %1").arg(i));
            m_tabWidget->setCurrentIndex(i);
            break;
        }
    }

    // Position the cursor at the specified line and position
    class ExpressionEditor *editor = targetSheet->getEditor();
    if (editor) {
        // Move to the specified line
        QTextCursor cursor = editor->textCursor();
        QTextBlock targetBlock = editor->document()->findBlockByNumber(lineNumber - 1); // Convert 1-based to 0-based

        if (targetBlock.isValid()) {
            cursor.setPosition(targetBlock.position());

            // If a specific cursor position is specified, move to that position within the line
            if (cursorPosition >= 0) {
                int blockPosition = targetBlock.position();
                int targetPosition = blockPosition + qMin(cursorPosition, targetBlock.length() - 1);
                cursor.setPosition(targetPosition);
                LOG_DEBUG(QString("  Positioned cursor at line %1, position %2 (absolute position %3)")
                          .arg(lineNumber).arg(cursorPosition).arg(targetPosition));
            } else {
                LOG_DEBUG(QString("  Positioned cursor at line %1, start of line").arg(lineNumber));
            }

            editor->setTextCursor(cursor);
            editor->setFocus(); // Give focus to the editor so highlighting works
        } else {
            LOG_DEBUG(QString("  Line %1 not found in target sheet").arg(lineNumber));
        }
    }

    LOG_DEBUG(QString("=== END MainWindow::navigateToSheet ==="));
}

void MainWindow::saveNavigationHistory(const QString &sheetName, int lineNumber, int cursorPosition)
{
    LOG_DEBUG(QString("=== MainWindow::saveNavigationHistory ==="));
    LOG_DEBUG(QString("  Saving: sheet='%1', line=%2, position=%3")
              .arg(sheetName).arg(lineNumber).arg(cursorPosition));

    m_navigationHistory.sheetName = sheetName;
    m_navigationHistory.lineNumber = lineNumber;
    m_navigationHistory.cursorPosition = cursorPosition;
    m_hasNavigationHistory = true;

    LOG_DEBUG(QString("=== END MainWindow::saveNavigationHistory ==="));
}

bool MainWindow::hasNavigationHistory() const
{
    return m_hasNavigationHistory;
}

void MainWindow::returnToPreviousLocation()
{
    LOG_DEBUG(QString("=== MainWindow::returnToPreviousLocation ==="));

    if (!m_hasNavigationHistory) {
        LOG_DEBUG("  No navigation history available");
        return;
    }

    LOG_DEBUG(QString("  Returning to: sheet='%1', line=%2, position=%3")
              .arg(m_navigationHistory.sheetName)
              .arg(m_navigationHistory.lineNumber)
              .arg(m_navigationHistory.cursorPosition));

    // Navigate back to the saved location
    navigateToSheet(m_navigationHistory.sheetName, m_navigationHistory.lineNumber, m_navigationHistory.cursorPosition);

    // Clear navigation history after use
    m_hasNavigationHistory = false;

    LOG_DEBUG(QString("=== END MainWindow::returnToPreviousLocation ==="));
}

void MainWindow::setupCrossSheetSupport(WorksheetWidget *worksheet)
{
    if (!worksheet) {
        LOG_WARNING("setupCrossSheetSupport: worksheet is null");
        return;
    }

    // Get the calculation engine from the worksheet
    CalculationEngine *engine = worksheet->getCalculationEngine();
    if (!engine) {
        LOG_WARNING("setupCrossSheetSupport: engine is null");
        return;
    }

    // Set up the sheet lookup function using a lambda that captures 'this'
    engine->setSheetLookupFunction([this](const QString &sheetName) -> WorksheetWidget* {
        return this->getSheetByName(sheetName);
    });

    // Set the current sheet name for error reporting
    // Find the tab index for this worksheet to get its name
    QString tabName = "Unknown";
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        if (m_tabWidget->widget(i) == worksheet) {
            tabName = m_tabWidget->tabText(i);
            engine->setCurrentSheetName(tabName);
            break;
        }
    }

    LOG_DEBUG(QString("setupCrossSheetSupport: Set up cross-sheet support for sheet: %1").arg(tabName));
}

void MainWindow::highlightIncomingCrossSheetReferences(WorksheetWidget *targetSheet)
{
    if (!targetSheet) {
        return;
    }

    // Get the name of the target sheet
    QString targetSheetName;
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        if (m_tabWidget->widget(i) == targetSheet) {
            targetSheetName = m_tabWidget->tabText(i);
            break;
        }
    }

    if (targetSheetName.isEmpty()) {
        LOG_DEBUG("highlightIncomingCrossSheetReferences: Could not find target sheet name");
        return;
    }

    LOG_DEBUG(QString("highlightIncomingCrossSheetReferences: Looking for references to sheet '%1'").arg(targetSheetName));

    // Clear any existing cross-sheet highlighting first
    targetSheet->getResults()->clearCrossSheetHighlighting();
    targetSheet->getEditor()->clearCrossSheetHighlighting();

    // Pattern to find cross-sheet references to this target sheet
    QRegularExpression crossSheetPattern(QString(R"(\bS\.%1\.LN(\d+)\b)").arg(QRegularExpression::escape(targetSheetName)),
                                        QRegularExpression::CaseInsensitiveOption);

    // Check all other tabs for references to the target sheet
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        WorksheetWidget *sourceSheet = qobject_cast<WorksheetWidget*>(m_tabWidget->widget(i));
        if (!sourceSheet || sourceSheet == targetSheet) {
            continue; // Skip the target sheet itself
        }

        QString sourceSheetName = m_tabWidget->tabText(i);

        // Get the current line from the source sheet
        int currentLine = sourceSheet->getEditor()->getCurrentLineNumber();
        QString currentLineText = sourceSheet->getEditor()->textCursor().block().text();

        LOG_DEBUG(QString("Checking sheet '%1' line %2: '%3'").arg(sourceSheetName).arg(currentLine).arg(currentLineText));

        // Find cross-sheet references in the current line of the source sheet
        QRegularExpressionMatchIterator iterator = crossSheetPattern.globalMatch(currentLineText);
        while (iterator.hasNext()) {
            QRegularExpressionMatch match = iterator.next();
            int referencedLineNumber = match.captured(1).toInt();

            LOG_DEBUG(QString("Found reference from '%1' line %2 to '%3' line %4")
                      .arg(sourceSheetName).arg(currentLine).arg(targetSheetName).arg(referencedLineNumber));

            // Get the LN color for this line number from the source sheet
            QColor lnColor = sourceSheet->getEditor()->getSyntaxHighlighter()->getLNColor(referencedLineNumber);

            // Highlight the referenced line on the target sheet
            targetSheet->getResults()->highlightSpecificLine(referencedLineNumber, lnColor);
            targetSheet->getEditor()->highlightSpecificLine(referencedLineNumber, lnColor);

            // Update the target sheet's line number area
            if (targetSheet->getResults()->getLineNumberArea()) {
                targetSheet->getResults()->getLineNumberArea()->update();
            }
        }
    }
}

void MainWindow::triggerCrossSheetRecalculation()
{
    // This method is called when any worksheet detects cross-sheet references
    // It ensures all worksheets are recalculated in the proper order
    recalculateAllWorksheets();
}

void MainWindow::recalculateAllWorksheets()
{
    LOG_DEBUG("=== Starting coordinated cross-sheet recalculation ===");

    // Phase A: Calculate all local expressions first (no cross-sheet processing)
    LOG_DEBUG("Phase A: Local calculations (cross-sheet disabled)");
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        WorksheetWidget *worksheet = qobject_cast<WorksheetWidget*>(m_tabWidget->widget(i));
        if (worksheet) {
            QString tabName = m_tabWidget->tabText(i);
            LOG_DEBUG(QString("Phase A: Processing sheet %1").arg(tabName));

            // Temporarily disable cross-sheet processing for local calculations
            CalculationEngine *engine = worksheet->getCalculationEngine();
            if (engine) {
                // Clear the sheet lookup function to prevent cross-sheet resolution
                engine->setSheetLookupFunction(nullptr);
                LOG_DEBUG(QString("Phase A: Disabled cross-sheet lookup for %1").arg(tabName));
            }

            // Force recalculation of this worksheet
            worksheet->evaluateAndHighlight();
        }
    }

    // Phase B: Re-enable cross-sheet processing and recalculate sheets with cross-sheet references
    LOG_DEBUG("Phase B: Cross-sheet calculations (cross-sheet enabled)");
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        WorksheetWidget *worksheet = qobject_cast<WorksheetWidget*>(m_tabWidget->widget(i));
        if (worksheet) {
            QString tabName = m_tabWidget->tabText(i);

            // Re-enable cross-sheet processing
            setupCrossSheetSupport(worksheet);

            // Recalculate only if this sheet has cross-sheet references
            bool hasRefs = worksheet->hasCrossSheetReferences();
            LOG_DEBUG(QString("Phase B: Sheet %1 has cross-sheet refs: %2").arg(tabName).arg(hasRefs ? "true" : "false"));
            if (hasRefs) {
                LOG_DEBUG(QString("Phase B: Force recalculating sheet %1").arg(tabName));
                worksheet->forceRecalculation(); // Force recalculation
            }
        }
    }

    LOG_DEBUG("=== Coordinated cross-sheet recalculation complete ===");
}

void MainWindow::onLineNumberingChanged(const QString &sheetName, const QList<LineChange> &changes)
{
    LOG_DEBUG(QString("=== Cross-Sheet LN Auto-Update triggered by sheet '%1' ===").arg(sheetName));

    // Update LN references in all OTHER worksheets that reference the changed sheet
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        WorksheetWidget *worksheet = qobject_cast<WorksheetWidget*>(m_tabWidget->widget(i));
        if (!worksheet) {
            continue;
        }

        QString currentSheetName = m_tabWidget->tabText(i);

        // Skip the sheet that triggered the change
        if (currentSheetName.compare(sheetName, Qt::CaseInsensitive) == 0) {
            continue;
        }

        // Check if this worksheet has cross-sheet references to the changed sheet
        if (updateCrossSheetReferences(worksheet, sheetName, changes)) {
            LOG_INFO(QString("Updated cross-sheet LN references in sheet '%1' due to changes in sheet '%2'")
                    .arg(currentSheetName).arg(sheetName));
        }
    }

    LOG_DEBUG("=== Cross-Sheet LN Auto-Update complete ===");
}

bool MainWindow::updateCrossSheetReferences(WorksheetWidget *worksheet, const QString &changedSheetName, const QList<LineChange> &changes)
{
    if (!worksheet) {
        return false;
    }

    QString content = worksheet->getContent();
    QStringList lines = content.split('\n');
    bool anyUpdated = false;

    // Create a regex pattern to find cross-sheet references to the changed sheet
    QString pattern = QString(R"(\bS\.%1\.LN(\d+)\b)").arg(QRegularExpression::escape(changedSheetName));
    QRegularExpression crossSheetRegex(pattern, QRegularExpression::CaseInsensitiveOption);

    LOG_DEBUG(QString("Checking for cross-sheet references to '%1' using pattern: %2").arg(changedSheetName).arg(pattern));

    // Process each line to update cross-sheet references
    for (int i = 0; i < lines.size(); ++i) {
        QString &line = lines[i];
        QString originalLine = line;

        // Find all cross-sheet references in this line
        QRegularExpressionMatchIterator iterator = crossSheetRegex.globalMatch(line);
        QList<QRegularExpressionMatch> matches;

        // Collect all matches (process from right to left to avoid position shifts)
        while (iterator.hasNext()) {
            matches.prepend(iterator.next());
        }

        // Update each reference
        for (const QRegularExpressionMatch &match : matches) {
            bool ok;
            int originalLineNumber = match.captured(1).toInt(&ok);
            if (!ok) {
                continue;
            }

            // Calculate the new line number after the changes
            int newLineNumber = calculateNewLineNumber(originalLineNumber, changes);

            if (newLineNumber != originalLineNumber) {
                QString oldRef = match.captured(0);
                QString newRef;

                if (newLineNumber == -1) {
                    // Line was deleted - replace with 0
                    newRef = "0";
                    LOG_WARNING(QString("Cross-sheet reference %1 points to deleted line, replacing with 0").arg(oldRef));
                } else {
                    // Update the line number
                    newRef = QString("S.%1.LN%2").arg(changedSheetName).arg(newLineNumber);
                    LOG_DEBUG(QString("Updating cross-sheet reference: %1 -> %2").arg(oldRef).arg(newRef));
                }

                // Replace the reference in the line
                line.replace(match.capturedStart(), match.capturedLength(), newRef);
                anyUpdated = true;
            }
        }

        if (line != originalLine) {
            LOG_DEBUG(QString("Line %1 updated: '%2' -> '%3'").arg(i + 1).arg(originalLine).arg(line));
        }
    }

    // If any references were updated, update the worksheet content
    if (anyUpdated) {
        QString updatedContent = lines.join('\n');
        worksheet->setContent(updatedContent);
        LOG_INFO(QString("Updated cross-sheet references in worksheet due to changes in sheet '%1'").arg(changedSheetName));
        return true;
    }

    return false;
}

int MainWindow::calculateNewLineNumber(int originalLine, const QList<LineChange> &changes) const
{
    int newLineNumber = originalLine;

    // Apply changes in order
    for (const LineChange &change : changes) {
        if (change.type == LineChange::Insertion) {
            // If insertion happens before this line, shift line number up
            if (change.startLine <= originalLine) {
                newLineNumber += change.count;
            }
        } else if (change.type == LineChange::Deletion) {
            // If deletion happens before this line, shift line number down
            if (change.startLine <= originalLine) {
                if (originalLine < change.startLine + change.count) {
                    // This line was deleted
                    return -1;
                } else {
                    // This line comes after the deletion
                    newLineNumber -= change.count;
                }
            }
        }
        // Modifications don't change line numbers
    }

    return newLineNumber;
}

void MainWindow::onValuesChanged(const QString &sheetName)
{
    LOG_DEBUG(QString("=== Cross-Sheet Value Change triggered by sheet '%1' ===").arg(sheetName));

    // Check for circular dependencies before proceeding with recalculation
    if (hasCircularCrossSheetDependencies(sheetName)) {
        LOG_ERROR(QString("Circular cross-sheet dependencies detected involving sheet '%1' - skipping recalculation to prevent infinite loop").arg(sheetName));
        return;
    }

    // Recalculate all worksheets that have cross-sheet references to the changed sheet
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        WorksheetWidget *worksheet = qobject_cast<WorksheetWidget*>(m_tabWidget->widget(i));
        if (!worksheet) {
            continue;
        }

        QString currentSheetName = m_tabWidget->tabText(i);

        // Skip the sheet that triggered the change
        if (currentSheetName.compare(sheetName, Qt::CaseInsensitive) == 0) {
            continue;
        }

        // Check if this worksheet has cross-sheet references to the changed sheet
        if (hasCrossSheetReferencesToSheet(worksheet, sheetName)) {
            LOG_DEBUG(QString("Recalculating sheet '%1' due to value changes in sheet '%2'")
                     .arg(currentSheetName).arg(sheetName));
            worksheet->forceRecalculation();
        }
    }

    LOG_DEBUG("=== Cross-Sheet Value Change recalculation complete ===");
}

bool MainWindow::hasCrossSheetReferencesToSheet(WorksheetWidget *worksheet, const QString &sheetName) const
{
    if (!worksheet) {
        return false;
    }

    QString content = worksheet->getContent();

    // Create a regex pattern to find cross-sheet references to the specified sheet
    QString pattern = QString(R"(\bS\.%1\.LN\d+\b)").arg(QRegularExpression::escape(sheetName));
    QRegularExpression crossSheetRegex(pattern, QRegularExpression::CaseInsensitiveOption);

    // Check if the content contains any references to the specified sheet
    return crossSheetRegex.match(content).hasMatch();
}

bool MainWindow::hasCircularCrossSheetDependencies(const QString &sheetName) const
{
    // Get the worksheet for the specified sheet
    WorksheetWidget *worksheet = getSheetByName(sheetName);
    if (!worksheet) {
        return false;
    }

    // Get the dependency tracker from the worksheet
    CalculationEngine *engine = worksheet->getCalculationEngine();
    if (!engine) {
        return false;
    }

    // Create a lambda function for sheet lookup
    auto sheetLookupFunction = [this](const QString &name) -> WorksheetWidget* {
        return this->getSheetByName(name);
    };

    // Use the dependency tracker to check for circular dependencies
    // Note: We need to access the dependency tracker from the calculation engine
    // For now, we'll implement a simpler approach by checking direct circular references

    return detectSimpleCircularReferences(sheetName);
}

bool MainWindow::detectSimpleCircularReferences(const QString &startSheet) const
{
    QSet<QString> visited;
    QSet<QString> currentPath;

    return detectCircularReferencesRecursive(startSheet, visited, currentPath);
}

bool MainWindow::detectCircularReferencesRecursive(const QString &sheetName,
                                                  QSet<QString> &visited,
                                                  QSet<QString> &currentPath) const
{
    // If we've already visited this sheet in the current path, we have a cycle
    if (currentPath.contains(sheetName.toLower())) {
        LOG_WARNING(QString("Circular cross-sheet dependency detected: %1 -> %2")
                   .arg(QStringList(currentPath.values()).join(" -> ")).arg(sheetName));
        return true;
    }

    // If we've already fully processed this sheet, no cycle from here
    if (visited.contains(sheetName.toLower())) {
        return false;
    }

    // Add to current path
    currentPath.insert(sheetName.toLower());

    // Get all sheets referenced by this sheet
    QSet<QString> referencedSheets = getReferencedSheets(sheetName);

    // Check each referenced sheet for cycles
    for (const QString &referencedSheet : referencedSheets) {
        if (detectCircularReferencesRecursive(referencedSheet, visited, currentPath)) {
            return true; // Cycle detected
        }
    }

    // Remove from current path and mark as visited
    currentPath.remove(sheetName.toLower());
    visited.insert(sheetName.toLower());

    return false;
}

QSet<QString> MainWindow::getReferencedSheets(const QString &sheetName) const
{
    QSet<QString> referencedSheets;

    WorksheetWidget *worksheet = getSheetByName(sheetName);
    if (!worksheet) {
        return referencedSheets;
    }

    QString content = worksheet->getContent();

    // Create a regex pattern to find all cross-sheet references
    QRegularExpression crossSheetRegex(R"(\bS\.([^.]+)\.LN\d+\b)", QRegularExpression::CaseInsensitiveOption);

    QRegularExpressionMatchIterator iterator = crossSheetRegex.globalMatch(content);
    while (iterator.hasNext()) {
        QRegularExpressionMatch match = iterator.next();
        QString referencedSheet = match.captured(1).trimmed().toLower();
        if (!referencedSheet.isEmpty() && referencedSheet != sheetName.toLower()) {
            referencedSheets.insert(referencedSheet);
        }
    }

    return referencedSheets;
}
