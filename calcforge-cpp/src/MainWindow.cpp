#include "MainWindow.h"
#include "WorksheetWidget.h"
#include "ExpressionEditor.h"
#include "ResultsDisplay.h"
#include "CalculationEngine.h"
#include "SyntaxHighlighter.h"
#include "LineNumberArea.h"
#include "CurrencyConverter.h"
#include "LineChangeDetector.h"
#include "HelpDialog.h"
#include "Logger.h"
#include "CustomTabWidget.h"

// Phase 3: Manager classes
#include "FileManager.h"
#include "TabManager.h"
#include "CrossSheetNavigator.h"
#include "WindowManager.h"

// Phase 4.1: Event system
#include "EventBus.h"

#ifdef Q_OS_WIN
#include <windows.h>
#endif

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
#include <QDialog>
#include <QLineEdit>
#include <QTabBar>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QCoreApplication>
#include <QDebug>
#include <QTimer>
#include <QFile>
#include <QDateTime>
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
    , m_loadButton(nullptr)
    , m_loadDropdownButton(nullptr)
    , m_saveButton(nullptr)
    , m_saveDropdownButton(nullptr)
    , m_alwaysOnTopButton(nullptr)
    , m_helpButton(nullptr)
    , m_menuBar(nullptr)
    , m_toolBar(nullptr)
    , m_statusBar(nullptr)
    , m_settings(nullptr)
    , m_increaseFontShortcut(nullptr)
    , m_decreaseFontShortcut(nullptr)
    , m_resetFontShortcut(nullptr)
    , m_fileManager(nullptr)
    , m_tabManager(nullptr)
    , m_crossSheetNavigator(nullptr)
    , m_windowManager(nullptr)
    , m_eventBus(nullptr)
    , m_isAlwaysOnTop(false)
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
    
    // Phase 3: Initialize manager classes with dependency injection
    initializeManagers();

    // Setup keyboard shortcuts
    setupShortcuts();

    // Install global event filter to catch font size shortcuts
    qApp->installEventFilter(this);
    
    // Restore window state
    restoreWindowState();

    // Load existing worksheets or create initial tab if none exist
    if (m_fileManager) {
        m_fileManager->loadWorksheets();
    }

    // Ensure we have at least one tab
    if (m_tabWidget->count() == 0) {
        createInitialTab();
    }

    // Set always on top by default after window is shown
    QTimer::singleShot(100, this, [this]() {
        toggleAlwaysOnTop(true);
    });
}

MainWindow::~MainWindow()
{
    saveWindowState();
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

    // Load button (file folder icon)
    m_loadButton = new QPushButton("📁", this);
    m_loadButton->setFixedSize(28, 28);
    m_loadButton->setToolTip("Load Worksheet");
    m_loadButton->setStyleSheet(
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
    connect(m_loadButton, &QPushButton::clicked, this, &MainWindow::loadWorksheetFile);

    // Load dropdown button (small arrow)
    m_loadDropdownButton = new QPushButton("▼", this);
    m_loadDropdownButton->setFixedSize(16, 28);
    m_loadDropdownButton->setToolTip("Recent Files");
    m_loadDropdownButton->setStyleSheet(
        "QPushButton {"
        "  background-color: #21262D;"
        "  color: #ffffff;"
        "  border: none;"
        "  border-radius: 6px;"
        "  font-size: 10px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background-color: #30363D;"
        "}"
        "QPushButton:focus {"
        "  outline: 2px solid #0c7ff2;"
        "}"
    );
    connect(m_loadDropdownButton, &QPushButton::clicked, this, &MainWindow::showLoadDropdown);

    // Save button (floppy disk icon)
    m_saveButton = new QPushButton("💾", this);
    m_saveButton->setFixedSize(28, 28);
    m_saveButton->setToolTip("Save Worksheet");
    m_saveButton->setStyleSheet(
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
    connect(m_saveButton, &QPushButton::clicked, this, &MainWindow::saveWorksheetFile);

    // Save dropdown button (small arrow)
    m_saveDropdownButton = new QPushButton("▼", this);
    m_saveDropdownButton->setFixedSize(16, 28);
    m_saveDropdownButton->setToolTip("Save As...");
    m_saveDropdownButton->setStyleSheet(
        "QPushButton {"
        "  background-color: #21262D;"
        "  color: #ffffff;"
        "  border: none;"
        "  border-radius: 6px;"
        "  font-size: 10px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background-color: #30363D;"
        "}"
        "QPushButton:focus {"
        "  outline: 2px solid #0c7ff2;"
        "}"
    );
    connect(m_saveDropdownButton, &QPushButton::clicked, this, &MainWindow::showSaveDropdown);

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

    // Always on top toggle button (pin icon)
    m_alwaysOnTopButton = new QPushButton("📌", this);
    m_alwaysOnTopButton->setFixedSize(28, 28);
    m_alwaysOnTopButton->setToolTip("Toggle Always On Top");
    m_alwaysOnTopButton->setCheckable(true); // Makes it a toggle button
    m_alwaysOnTopButton->setChecked(true); // Start checked (always on top by default)

    // Set initial style (unchecked state - grey background)
    m_alwaysOnTopButton->setStyleSheet(
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
        "QPushButton:checked {"
        "  background-color: #0c7ff2;"  // Blue background when checked
        "  color: #ffffff;"
        "}"
        "QPushButton:checked:hover {"
        "  background-color: #1f6feb;"  // Slightly darker blue on hover when checked
        "}"
    );

    connect(m_alwaysOnTopButton, &QPushButton::toggled, this, &MainWindow::toggleAlwaysOnTop);

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

    // Create grouped load button layout (no spacing between main button and dropdown)
    QHBoxLayout *loadGroupLayout = new QHBoxLayout();
    loadGroupLayout->setContentsMargins(0, 0, 0, 0);
    loadGroupLayout->setSpacing(0); // No spacing between load button and dropdown
    loadGroupLayout->addWidget(m_loadButton);
    loadGroupLayout->addWidget(m_loadDropdownButton);

    QWidget *loadGroupWidget = new QWidget();
    loadGroupWidget->setLayout(loadGroupLayout);
    m_topLayout->addWidget(loadGroupWidget);

    // Create grouped save button layout (no spacing between main button and dropdown)
    QHBoxLayout *saveGroupLayout = new QHBoxLayout();
    saveGroupLayout->setContentsMargins(0, 0, 0, 0);
    saveGroupLayout->setSpacing(0); // No spacing between save button and dropdown
    saveGroupLayout->addWidget(m_saveButton);
    saveGroupLayout->addWidget(m_saveDropdownButton);

    QWidget *saveGroupWidget = new QWidget();
    saveGroupWidget->setLayout(saveGroupLayout);
    m_topLayout->addWidget(saveGroupWidget);

    m_topLayout->addWidget(m_currencyButton);
    m_topLayout->addWidget(m_alwaysOnTopButton);
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

    // Create custom tab widget with ampersand handling
    m_tabWidget = new CustomTabWidget(this);
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
        // Configure tab bar for proper & character display
        tabBar->setUsesScrollButtons(false);
        tabBar->setAutoHide(false);
        // Disable tab text eliding and try to preserve & characters
        tabBar->setElideMode(Qt::ElideNone);
        // More aggressive mnemonic disabling
        tabBar->setProperty("_q_no_mnemonic", true);
        tabBar->setAttribute(Qt::WA_KeyboardFocusChange, false);
        tabBar->setFocusPolicy(Qt::NoFocus);
        // Try to disable mnemonics at the widget level
        tabBar->setAttribute(Qt::WA_MacShowFocusRect, false);
        tabBar->setProperty("showShortcutUnderline", false);
    }

    // Note: Tab rename will be connected after TabManager is created in Phase 3
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

    // Add file operation shortcuts
    QShortcut *saveShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_S), this);
    QShortcut *loadShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_L), this);
    QShortcut *alwaysOnTopShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_T), this);

    connect(saveShortcut, &QShortcut::activated, this, &MainWindow::saveWorksheetFile);
    connect(loadShortcut, &QShortcut::activated, this, &MainWindow::loadWorksheetFile);
    connect(alwaysOnTopShortcut, &QShortcut::activated, [this]() {
        m_alwaysOnTopButton->toggle(); // Toggle the button state
    });

    logDebug("Font size, tab navigation, text selection, and file operation shortcuts setup complete");
}

void MainWindow::createInitialTab()
{
    if (m_tabWidget->count() == 0) {
        addTab();
    }
}

void MainWindow::addTab()
{
    // Phase 3: Delegate to TabManager
    if (m_tabManager) {
        auto result = m_tabManager->addTab();
        if (!result.isValid()) {
            LOG_DEBUG("MainWindow: Failed to add tab: " + result.errorMessage());
        }
    }
}

void MainWindow::closeTab(int index)
{
    // Phase 3: Delegate to TabManager
    if (m_tabManager) {
        auto result = m_tabManager->closeTab(index);
        if (!result.isValid()) {
            LOG_DEBUG("MainWindow: Failed to close tab: " + result.errorMessage());
        }
    }
    
    m_tabWidget->removeTab(index);
}

void MainWindow::renameTab(int index)
{
    // Phase 3: Delegate to TabManager for proper ampersand handling
    if (m_tabManager) {
        auto result = m_tabManager->renameTab(index);
        if (!result.isValid()) {
            LOG_DEBUG("MainWindow: Failed to rename tab: " + result.errorMessage());
        }
    } else {
        LOG_DEBUG("MainWindow: TabManager not available for tab rename");
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
    HelpDialog *helpDialog = new HelpDialog(this);
    if (m_fileManager) {
        helpDialog->setCurrentFilePath(m_fileManager->getCurrentFile());
    }
    helpDialog->exec();
    helpDialog->deleteLater();
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
    loadWorksheetFile();
}

void MainWindow::saveFile()
{
    saveWorksheetFile();
}

void MainWindow::saveAsFile()
{
    saveWorksheetFileAs();
}

void MainWindow::loadWorksheetFile()
{
    // Phase 3: Delegate to FileManager
    if (m_fileManager) {
        auto result = m_fileManager->loadWorksheetFile();
        if (!result.isValid()) {
            LOG_DEBUG("MainWindow: Failed to load worksheet: " + result.errorMessage());
        }
    }
}

bool MainWindow::saveWorksheetFile()
{
    // Phase 3: Delegate to FileManager
    if (m_fileManager) {
        auto result = m_fileManager->saveWorksheetFile();
        if (result.isValid()) {
            return result.value();
        } else {
            LOG_DEBUG("MainWindow: Failed to save worksheet: " + result.errorMessage());
            return false;
        }
    }
    return false;
}

bool MainWindow::saveWorksheetFileAs()
{
    // Phase 3: Delegate to FileManager
    if (m_fileManager) {
        auto result = m_fileManager->saveWorksheetFileAs();
        if (result.isValid()) {
            return result.value();
        } else {
            LOG_DEBUG("MainWindow: Failed to save worksheet as: " + result.errorMessage());
            return false;
        }
    }
    return false;
}

void MainWindow::showLoadDropdown()
{
    // Phase 3: Delegate to FileManager
    if (!m_fileManager) {
        return;
    }
    
    QStringList recentFiles = m_fileManager->getRecentFiles();
    if (recentFiles.isEmpty()) {
        QMessageBox::information(this, "Recent Files", "No recent files available");
        return;
    }

    QMenu *menu = new QMenu(this);

    for (const QString &filePath : recentFiles) {
        QFileInfo fileInfo(filePath);
        QString displayName = fileInfo.baseName();
        if (displayName.length() > 30) {
            displayName = displayName.left(27) + "...";
        }

        QAction *action = menu->addAction(displayName);
        action->setToolTip(filePath);
        connect(action, &QAction::triggered, [this, filePath]() {
            m_fileManager->loadRecentFile(filePath);
        });
    }

    // Show menu below the dropdown button
    QPoint pos = m_loadDropdownButton->mapToGlobal(QPoint(0, m_loadDropdownButton->height()));
    menu->exec(pos);

    menu->deleteLater();
}

void MainWindow::showSaveDropdown()
{
    QMenu *menu = new QMenu(this);

    QAction *saveAsAction = menu->addAction("Save As...");
    connect(saveAsAction, &QAction::triggered, this, &MainWindow::saveWorksheetFileAs);

    // Show menu below the dropdown button
    QPoint pos = m_saveDropdownButton->mapToGlobal(QPoint(0, m_saveDropdownButton->height()));
    menu->exec(pos);

    menu->deleteLater();
}

// Method removed - now handled by FileManager

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
    // Phase 3: Delegate to FileManager
    if (m_fileManager) {
        m_fileManager->loadWorksheets();
    }
}

void MainWindow::loadWorksheetFromFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open file for reading:" << filePath << file.errorString();
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "Failed to parse JSON file:" << filePath << parseError.errorString();
        return;
    }

    if (!doc.isObject()) {
        qWarning() << "Invalid JSON format in file:" << filePath;
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

    // Phase 3: FileManager handles its own state internally
    if (m_fileManager) {
        // FileManager manages current file and recent files internally
        QString appDir = QCoreApplication::applicationDirPath();
        QString defaultWorksheetsPath = QDir(appDir).absoluteFilePath("worksheets.json");
        if (filePath != defaultWorksheetsPath) {
            m_fileManager->addToRecentFiles(filePath);
            setWindowTitle(QString("CalcForge v4.0 - %1").arg(QFileInfo(filePath).baseName()));
        } else {
            setWindowTitle("CalcForge v4.0");
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
                    int currentLine = firstWorksheet->getEditor()->getCurrentLineNumber();
                    QString currentLineText = firstWorksheet->getEditor()->textCursor().block().text();

                    // Trigger the same highlighting logic as cursor position changes
                    firstWorksheet->getResults()->highlightCurrentLineWithLNReferences(currentLine, currentLineText);

                    LOG_DEBUG(QString("App startup: Triggered initial highlighting for line %1: '%2'")
                              .arg(currentLine).arg(currentLineText));
                }
            });
        }
    }
}

void MainWindow::loadExampleWorksheetsAndSave()
{
    QString appDir = QCoreApplication::applicationDirPath();
    QString exampleWorksheetsPath = QDir(appDir).absoluteFilePath("example_worksheets.json");
    QString worksheetsPath = QDir(appDir).absoluteFilePath("worksheets.json");

    qDebug() << "Loading example worksheets and saving as worksheets.json";

    // Load the example worksheets content directly (without adding to recent files)
    if (loadWorksheetContentFromFile(exampleWorksheetsPath)) {
        // Save them as worksheets.json to prevent overwriting the example file
        if (saveWorksheetsToFile(worksheetsPath)) {
            // Phase 3: Delegate file operations to FileManager
            if (m_fileManager) {
                // FileManager handles file state internally
                m_fileManager->addToRecentFiles(worksheetsPath);
            }
            setWindowTitle("CalcForge v4.0");
            qDebug() << "Successfully saved example worksheets as worksheets.json";
        } else {
            qWarning() << "Failed to save example worksheets as worksheets.json";
        }
    } else {
        qWarning() << "Failed to load example worksheets from" << exampleWorksheetsPath;
    }
}

bool MainWindow::loadWorksheetContentFromFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.exists()) {
        qWarning() << "File does not exist:" << filePath;
        return false;
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open file:" << filePath << file.errorString();
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "Failed to parse JSON file:" << parseError.errorString();
        return false;
    }

    if (!doc.isObject()) {
        qWarning() << "The file does not contain a valid CalcForge worksheet format.";
        return false;
    }

    // Clear existing tabs
    while (m_tabWidget->count() > 0) {
        m_tabWidget->removeTab(0);
    }

    QJsonObject root = doc.object();

    // Load worksheets using the same logic as loadWorksheetFromFile()
    if (root.contains("version") && root.contains("tabs") && root["tabs"].isArray()) {
        // New format: load tabs in order from array
        QJsonArray tabsArray = root["tabs"].toArray();

        for (const QJsonValue &tabValue : tabsArray) {
            if (!tabValue.isObject()) {
                continue;
            }

            QJsonObject tabObject = tabValue.toObject();
            if (!tabObject.contains("name") || !tabObject.contains("content")) {
                continue;
            }

            QString tabName = tabObject["name"].toString();
            QString content = tabObject["content"].toString();

            loadSingleWorksheet(tabName, content);
        }
    } else {
        // Legacy format: load tabs from object keys
        for (auto it = root.begin(); it != root.end(); ++it) {
            QString tabName = it.key();
            QString content = it.value().toString();

            loadSingleWorksheet(tabName, content);
        }
    }

    // Ensure we have at least one tab
    if (m_tabWidget->count() == 0) {
        createInitialTab();
        return false;
    } else {
        // Trigger coordinated cross-sheet calculation
        recalculateAllWorksheets();

        // Set focus to first tab
        m_tabWidget->setCurrentIndex(0);
        WorksheetWidget *firstWorksheet = qobject_cast<WorksheetWidget*>(m_tabWidget->widget(0));
        if (firstWorksheet) {
            firstWorksheet->getEditor()->setFocus();
        }
    }

    return true;
}

void MainWindow::loadSingleWorksheet(const QString &tabName, const QString &content)
{
    // Create new worksheet
    WorksheetWidget *worksheet = new WorksheetWidget(this);

    // Set up cross-sheet support BEFORE setting content to ensure
    // cross-sheet references work during initial evaluation
    setupCrossSheetSupport(worksheet, tabName);

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

    // Phase 4.1: Connect worksheet to event system instead of direct signals
    QString sheetName = getCurrentSheetName();

    // Set the event bus on the worksheet
    if (m_eventBus) {
        worksheet->setEventBus(m_eventBus);
    }

    // Connect splitter changes to emit events
    connect(worksheet, &WorksheetWidget::splitterMoved,
            this, [this, sheetName](const QByteArray& newState) {
                if (m_eventBus) {
                    m_eventBus->worksheetEvents()->emitSplitterMoved(sheetName, newState);
                }
                onSplitterMoved(newState);
            });

    // Connect line numbering changes to emit events
    connect(worksheet, &WorksheetWidget::lineNumberingChanged,
            this, [this](const QString& sheetName, const QList<LineChange>& changes) {
                if (m_eventBus) {
                    m_eventBus->worksheetEvents()->emitLineNumberingChanged(sheetName, changes);
                }
                onLineNumberingChanged(sheetName, changes);
            });

    // Connect value changes to emit events
    connect(worksheet, &WorksheetWidget::valuesChanged,
            this, [this](const QString& sheetName) {
                if (m_eventBus) {
                    m_eventBus->worksheetEvents()->emitValuesChanged(sheetName);
                }
                onValuesChanged(sheetName);
            });

    // Connect content changes to emit events
    connect(worksheet, &WorksheetWidget::contentChanged,
            this, [this, sheetName]() {
                if (m_eventBus) {
                    m_eventBus->worksheetEvents()->emitContentChanged(sheetName);
                }
            });
}

void MainWindow::loadExampleWorksheets()
{
    // Get the path to example_worksheets.json in the same directory as the executable
    QString appDir = QCoreApplication::applicationDirPath();
    QString exampleWorksheetsPath = QDir(appDir).absoluteFilePath("example_worksheets.json");

    QFile file(exampleWorksheetsPath);
    if (!file.exists()) {
        // No example worksheets, this is fine
        return;
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open example_worksheets.json for reading:" << file.errorString();
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "Failed to parse example_worksheets.json:" << parseError.errorString();
        return;
    }

    if (!doc.isObject()) {
        qWarning() << "Invalid example_worksheets.json format: not an object";
        return;
    }

    QJsonObject root = doc.object();
    if (root.isEmpty()) {
        return;
    }

    // Load example worksheets using the same format as regular worksheets
    if (root.contains("version") && root.contains("tabs") && root["tabs"].isArray()) {
        QJsonArray tabsArray = root["tabs"].toArray();

        for (const QJsonValue &tabValue : tabsArray) {
            if (!tabValue.isObject()) {
                continue;
            }

            QJsonObject tabObject = tabValue.toObject();
            if (!tabObject.contains("name") || !tabObject.contains("content")) {
                continue;
            }

            QString tabName = tabObject["name"].toString();
            QString content = tabObject["content"].toString();

            loadSingleWorksheet(tabName, content);
        }

        // After loading example worksheets, trigger cross-sheet calculation
        if (m_tabWidget->count() > 0) {
            recalculateAllWorksheets();
        }
    }
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
        // Get unescaped tab name
        QString tabName = m_tabWidget->tabText(i);
        tabName.replace("&&", "&");  // Un-escape ampersands for saving
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

bool MainWindow::saveWorksheetsToFile(const QString &filePath)
{
    // Create new format with version and ordered tabs array
    QJsonObject root;
    root["version"] = "2.0";

    QJsonArray tabsArray;

    // Collect data from all tabs in their current order
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        // Get unescaped tab name
        QString tabName = m_tabWidget->tabText(i);
        tabName.replace("&&", "&");  // Un-escape ampersands for saving
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
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Save Error",
            QString("Failed to open file for writing: %1").arg(file.errorString()));
        return false;
    }

    QByteArray jsonData = doc.toJson(QJsonDocument::Indented);
    qint64 bytesWritten = file.write(jsonData);
    file.close();

    if (bytesWritten == -1) {
        QMessageBox::warning(this, "Save Error",
            QString("Failed to write to file: %1").arg(file.errorString()));
        return false;
    }

    qDebug() << "Saved worksheets to:" << filePath;
    return true;
}

// Method removed - now handled by FileManager

// Method removed - now handled by FileManager

// Method removed - now handled by FileManager

// Method removed - now handled by FileManager

// Method removed - now handled by FileManager

// Method removed - now handled by FileManager

// Method removed - now handled by FileManager

// Method removed - now handled by FileManager

void MainWindow::toggleAlwaysOnTop(bool enabled)
{
    // Use Qt's WindowStaysOnTopHint instead of Windows API for better dialog compatibility
    Qt::WindowFlags flags = windowFlags();

    if (enabled) {
        flags |= Qt::WindowStaysOnTopHint;
        qDebug() << "Always on top enabled (Qt)";
    } else {
        flags &= ~Qt::WindowStaysOnTopHint;
        qDebug() << "Always on top disabled (Qt)";
    }

    setWindowFlags(flags);
    show(); // Need to show again after changing flags

    // Store the state for dialog handling
    m_isAlwaysOnTop = enabled;
}

void MainWindow::temporarilyDisableAlwaysOnTop()
{
    // Don't change window flags - this causes flicker
    // Instead, we'll ensure dialogs have the correct parent and flags
    qDebug() << "Dialog about to show - keeping always on top active";
}

void MainWindow::restoreAlwaysOnTop()
{
    // No need to restore since we didn't change anything
    qDebug() << "Dialog closed - always on top still active";
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
    // Phase 3: Check for unsaved changes via FileManager
    bool hasChanges = false;
    if (m_fileManager) {
        hasChanges = m_fileManager->hasUnsavedChanges();
    }
    if (hasChanges) {
        QMessageBox msgBox(this);
        msgBox.setWindowTitle("Unsaved Changes");
        msgBox.setText("You have unsaved changes.");
        msgBox.setInformativeText("Do you want to save your changes before closing?");
        msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Save);
        msgBox.setIcon(QMessageBox::Question);

        int reply = msgBox.exec();

        if (reply == QMessageBox::Save) {
            // User wants to save - call save function
            bool saveSuccessful = saveWorksheetFile();
            if (!saveSuccessful) {
                // Save failed or was cancelled, don't close
                event->ignore();
                return;
            }
            // Save succeeded, continue with close
        } else if (reply == QMessageBox::Cancel) {
            // User cancelled, don't close
            event->ignore();
            return;
        }
        // If reply == QMessageBox::Discard, continue with close without saving
    }

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
    // Note: No longer auto-saving worksheets on close
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
    // Phase 3: Delegate to WindowManager
    if (m_windowManager) {
        m_windowManager->handleMousePress(event);
    }
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    // Phase 3: Delegate to WindowManager
    if (m_windowManager) {
        m_windowManager->handleMouseMove(event);
    }
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    // Phase 3: Delegate to WindowManager
    if (m_windowManager) {
        m_windowManager->handleMouseRelease(event);
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
            if (keyEvent->key() == Qt::Key_Period || keyEvent->text() == ".") {
                // Phase 3: Delegate to TabManager
                if (m_tabManager) {
                    m_tabManager->increaseFontSize();
                }
                return true; // Event handled
            } else if (keyEvent->key() == Qt::Key_Comma || keyEvent->text() == ",") {
                // Phase 3: Delegate to TabManager
                if (m_tabManager) {
                    m_tabManager->decreaseFontSize();
                }
                return true; // Event handled
            } else if (keyEvent->key() == Qt::Key_0) {
                // Phase 3: Delegate to TabManager
                if (m_tabManager) {
                    m_tabManager->resetFontSize();
                }
                return true; // Event handled
            } else if (keyEvent->key() == Qt::Key_PageUp) {
                // Phase 3: Delegate to TabManager
                if (m_tabManager) {
                    m_tabManager->previousTab();
                }
                return true; // Event handled
            } else if (keyEvent->key() == Qt::Key_PageDown) {
                // Phase 3: Delegate to TabManager
                if (m_tabManager) {
                    m_tabManager->nextTab();
                }
                return true; // Event handled
            }
        }
    }

    // Phase 3: Window management is handled by WindowManager through mouse event delegation
    // WindowManager handles events through the mouse event methods, not eventFilter

    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::createResizeCorners()
{
    // Phase 3: Delegate to WindowManager
    if (m_windowManager) {
        m_windowManager->setupResizeCorners();
    }
}

void MainWindow::updateCornerPositions()
{
    // Phase 3: Delegate to WindowManager
    if (m_windowManager) {
        m_windowManager->updateCornerPositions();
    }
}

void MainWindow::createResizeEdges()
{
    // Phase 3: Delegate to WindowManager
    if (m_windowManager) {
        m_windowManager->setupResizeEdges();
    }
}

void MainWindow::updateEdgePositions()
{
    // Phase 3: Delegate to WindowManager
    if (m_windowManager) {
        m_windowManager->updateEdgePositions();
    }
}

void MainWindow::applyGlobalFontSize()
{
    // Phase 3: Delegate to TabManager
    if (m_tabManager) {
        // TabManager handles font application to all tabs
        m_tabManager->applyGlobalFontSize(14); // Default font size
    }
}

void MainWindow::increaseFontSize()
{
    // Phase 3: Delegate to TabManager
    if (m_tabManager) {
        m_tabManager->increaseFontSize();
    }
}

void MainWindow::decreaseFontSize()
{
    // Phase 3: Delegate to TabManager
    if (m_tabManager) {
        m_tabManager->decreaseFontSize();
    }
}

void MainWindow::resetFontSize()
{
    // Phase 3: Delegate to TabManager
    if (m_tabManager) {
        m_tabManager->resetFontSize();
    }
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
        QString tabName = m_tabManager->getTabName(i);
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
        return m_tabManager->getTabName(currentIndex);
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
        return m_tabManager->getTabName(index);
    }
    return QString();
}

void MainWindow::navigateToSheet(const QString &sheetName, int lineNumber, int cursorPosition)
{
    // Phase 3: Delegate to CrossSheetNavigator
    if (m_crossSheetNavigator) {
        m_crossSheetNavigator->navigateToSheet(sheetName, lineNumber, cursorPosition);
    }
}

void MainWindow::saveNavigationHistory(const QString &sheetName, int lineNumber, int cursorPosition)
{
    // Phase 3: Delegate to CrossSheetNavigator
    if (m_crossSheetNavigator) {
        m_crossSheetNavigator->saveNavigationHistory(sheetName, lineNumber, cursorPosition);
    }
}

bool MainWindow::hasNavigationHistory() const
{
    // Phase 3: Delegate to CrossSheetNavigator
    if (m_crossSheetNavigator) {
        return m_crossSheetNavigator->hasNavigationHistory();
    }
    return false;
}

void MainWindow::returnToPreviousLocation()
{
    // Phase 3: Delegate to CrossSheetNavigator
    if (m_crossSheetNavigator) {
        m_crossSheetNavigator->returnToPreviousLocation();
    }
}

void MainWindow::setupCrossSheetSupport(WorksheetWidget *worksheet, const QString &sheetName)
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
    QString tabName = sheetName;
    if (tabName.isEmpty()) {
        // Fallback: Find the tab index for this worksheet to get its name
        tabName = "Unknown";
        for (int i = 0; i < m_tabWidget->count(); ++i) {
            if (m_tabWidget->widget(i) == worksheet) {
                tabName = m_tabWidget->tabText(i);
                break;
            }
        }
    }

    engine->setCurrentSheetName(tabName);
    LOG_DEBUG(QString("setupCrossSheetSupport: Set up cross-sheet support for sheet: %1").arg(tabName));

    // Disconnect any existing connections to prevent duplicates
    disconnect(worksheet, &WorksheetWidget::lineNumberingChanged, this, &MainWindow::onLineNumberingChanged);
    disconnect(worksheet, &WorksheetWidget::valuesChanged, this, &MainWindow::onValuesChanged);

    // Connect MainWindow signals for cross-sheet LN auto-updates
    connect(worksheet, &WorksheetWidget::lineNumberingChanged, this, &MainWindow::onLineNumberingChanged);
    LOG_DEBUG("MainWindow: Connected lineNumberingChanged signal for cross-sheet LN auto-updates");

    // Connect value changes for cross-sheet recalculation
    connect(worksheet, &WorksheetWidget::valuesChanged, this, &MainWindow::onValuesChanged);
    LOG_DEBUG("MainWindow: Connected valuesChanged signal for cross-sheet recalculation");
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

    // Phase B: Re-enable cross-sheet processing and recalculate ALL sheets
    LOG_DEBUG("Phase B: Cross-sheet calculations (cross-sheet enabled)");
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        WorksheetWidget *worksheet = qobject_cast<WorksheetWidget*>(m_tabWidget->widget(i));
        if (worksheet) {
            QString tabName = m_tabWidget->tabText(i);

            // Re-enable cross-sheet processing
            setupCrossSheetSupport(worksheet);

            // Recalculate ALL sheets, not just ones with outgoing cross-sheet references
            // This is necessary because sheets that are referenced BY other sheets
            // also need to be recalculated to provide correct values
            bool hasRefs = worksheet->hasCrossSheetReferences();
            LOG_DEBUG(QString("Phase B: Sheet %1 has cross-sheet refs: %2, recalculating anyway").arg(tabName).arg(hasRefs ? "true" : "false"));
            LOG_DEBUG(QString("Phase B: Force recalculating sheet %1").arg(tabName));
            worksheet->forceRecalculation(); // Force recalculation of ALL sheets
        }
    }

    LOG_DEBUG("=== Coordinated cross-sheet recalculation complete ===");
}

void MainWindow::onLineNumberingChanged(const QString &sheetName, const QList<LineChange> &changes)
{
    LOG_DEBUG(QString("=== MainWindow::onLineNumberingChanged CALLED - sheet '%1', changes: %2 ===").arg(sheetName).arg(changes.size()));
    LOG_DEBUG(QString("=== Cross-Sheet LN Auto-Update triggered by sheet '%1' ===").arg(sheetName));

    // Update LN references in all OTHER worksheets that reference the changed sheet
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        WorksheetWidget *worksheet = qobject_cast<WorksheetWidget*>(m_tabWidget->widget(i));
        if (!worksheet) {
            continue;
        }

        QString currentSheetName = m_tabManager->getTabName(i);

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
        // Save cursor position and scroll position before updating content
        ExpressionEditor* editor = worksheet->getEditor();
        int savedCursorPosition = 0;
        int savedLineNumber = 1;
        int savedPositionInBlock = 0;
        int savedVerticalScroll = 0;
        int savedHorizontalScroll = 0;

        if (editor) {
            QTextCursor cursor = editor->textCursor();
            savedCursorPosition = cursor.position();
            savedLineNumber = cursor.blockNumber() + 1;
            savedPositionInBlock = cursor.positionInBlock();

            // Save scroll positions
            savedVerticalScroll = editor->verticalScrollBar()->value();
            savedHorizontalScroll = editor->horizontalScrollBar()->value();

            LOG_DEBUG(QString("Saving cursor position: line %1, position in block %2, absolute position %3")
                      .arg(savedLineNumber).arg(savedPositionInBlock).arg(savedCursorPosition));
            LOG_DEBUG(QString("Saving scroll position: vertical %1, horizontal %2")
                      .arg(savedVerticalScroll).arg(savedHorizontalScroll));
        }

        QString updatedContent = lines.join('\n');
        worksheet->setContent(updatedContent);

        // Restore cursor position and scroll position after updating content
        if (editor) {
            QTextCursor cursor = editor->textCursor();
            cursor.setPosition(qMin(savedCursorPosition, editor->toPlainText().length()));
            editor->setTextCursor(cursor);

            // Restore scroll positions
            editor->verticalScrollBar()->setValue(savedVerticalScroll);
            editor->horizontalScrollBar()->setValue(savedHorizontalScroll);

            LOG_DEBUG(QString("Restored cursor position to absolute position %1").arg(cursor.position()));
            LOG_DEBUG(QString("Restored scroll position: vertical %1, horizontal %2")
                      .arg(savedVerticalScroll).arg(savedHorizontalScroll));
        }

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

// Phase 3: Manager initialization with dependency injection
void MainWindow::initializeManagers()
{
    LOG_DEBUG("MainWindow: Initializing manager classes with dependency injection");

    // Phase 4.1: Initialize event system first
    m_eventBus = new EventBus(this);
    EventBus::setInstance(m_eventBus);
    LOG_DEBUG("MainWindow: Event system initialized");

    // Create managers in dependency order (TabManager first, then FileManager)
    m_tabManager = new TabManager(m_tabWidget, m_settings, m_eventBus, this);
    m_fileManager = new FileManager(m_tabWidget, m_tabManager, m_settings, m_eventBus, this);
    m_crossSheetNavigator = new CrossSheetNavigator(m_tabManager, m_eventBus, this);
    m_windowManager = new WindowManager(this, m_settings, m_eventBus, this);

    // Connect tab rename to MainWindow method (handles always-on-top properly)
    connect(m_tabWidget, &QTabWidget::tabBarDoubleClicked, this, &MainWindow::renameTab);

    // Phase 4.1: Connect to event system instead of direct signals
    connect(m_eventBus->applicationEvents(), &ApplicationEvents::fileStateChanged,
            this, [this](bool hasUnsavedChanges) {
                // Update window title to show modified state
                QString title = "CalcForge";
                if (hasUnsavedChanges) {
                    title += " *";
                }
                setWindowTitle(title);
            });

    connect(m_eventBus->applicationEvents(), &ApplicationEvents::currentFileChanged,
            this, [this](const QString& filePath) {
                LOG_DEBUG("MainWindow: Current file changed to " + filePath);
            });

    // Note: FileManager dialog signals removed - using direct dialog flags instead
    
    connect(m_eventBus->applicationEvents(), &ApplicationEvents::currentTabChanged,
            this, [this](int index) {
                LOG_DEBUG("MainWindow: Current tab changed to index " + QString::number(index));
            });

    connect(m_eventBus->worksheetEvents(), &WorksheetEvents::navigationRequested,
            this, [this](const QString& sheetName, int lineNumber, int cursorPosition) {
                LOG_DEBUG(QString("MainWindow: Navigation requested to sheet %1, line %2")
                         .arg(sheetName).arg(lineNumber));
            });
    
    // Connect to event system for worksheet setup
    connect(m_eventBus->applicationEvents(), &ApplicationEvents::tabSetupRequested,
            this, [this](const QString& worksheetName) {
                // Find the worksheet by name and set up cross-sheet support
                for (int i = 0; i < m_tabWidget->count(); ++i) {
                    if (m_tabWidget->tabText(i) == worksheetName) {
                        WorksheetWidget* worksheet = qobject_cast<WorksheetWidget*>(m_tabWidget->widget(i));
                        if (worksheet) {
                            setupCrossSheetSupport(worksheet);
                            LOG_DEBUG("MainWindow: Cross-sheet support setup for worksheet '" + worksheetName + "'");
                        }
                        break;
                    }
                }
            });
    
    // Initialize window manager components
    m_windowManager->setupResizeCorners();
    m_windowManager->setupResizeEdges();
    m_windowManager->restoreWindowState();
    
    // Load initial worksheets through FileManager
    m_fileManager->loadWorksheets();
    
    LOG_DEBUG("MainWindow: Manager initialization completed");
}
 
 
 
 
 
