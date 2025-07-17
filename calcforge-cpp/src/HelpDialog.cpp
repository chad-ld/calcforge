#include "HelpDialog.h"
#include <QApplication>
#include <QScreen>
#include <QScrollBar>
#include <QFileInfo>
#include <QApplication>
#include <QClipboard>
#include <QTimer>

HelpDialog::HelpDialog(QWidget *parent)
    : QDialog(parent)
    , m_mainLayout(nullptr)
    , m_buttonLayout(nullptr)
    , m_splitter(nullptr)
    , m_topicList(nullptr)
    , m_contentArea(nullptr)
    , m_closeButton(nullptr)
    , m_clipboardButton(nullptr)
    , m_filePathLabel(nullptr)
{
    setupUI();
    setupContent();
    
    // Set initial selection
    if (m_topicList->count() > 0) {
        m_topicList->setCurrentRow(0);
        onTopicSelected(0);
    }
}

void HelpDialog::setupUI()
{
    setWindowTitle("CalcForge Help");
    setModal(true);
    
    // Set reasonable size (16:9 aspect ratio)
    int width = 1000;
    int height = 600;
    resize(width, height);
    
    // Center on parent
    if (parentWidget()) {
        QRect parentGeometry = parentWidget()->geometry();
        move(parentGeometry.center() - rect().center());
    }
    
    // Main layout
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(10, 10, 10, 10);
    m_mainLayout->setSpacing(10);
    
    // Create splitter for topic list and content
    m_splitter = new QSplitter(Qt::Horizontal, this);
    
    createTopicList();
    createContentArea();
    
    // Set splitter proportions (25% topics, 75% content)
    m_splitter->setSizes({250, 750});
    m_splitter->setStretchFactor(0, 0);
    m_splitter->setStretchFactor(1, 1);
    
    m_mainLayout->addWidget(m_splitter);

    // Button layout with file path
    m_buttonLayout = new QHBoxLayout();

    // File path label (compact, on same line as Close button)
    m_filePathLabel = new QLabel("No file loaded", this);
    m_filePathLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_filePathLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_filePathLabel->setStyleSheet(
        "QLabel {"
        "  background-color: transparent;"
        "  border: none;"
        "  padding: 4px 8px;"
        "  font-size: 14px;"
        "  color: #ffffff;"
        "  font-family: 'Consolas', 'Monaco', monospace;"
        "}"
    );

    m_buttonLayout->addWidget(m_filePathLabel);

    // Clipboard button (small, next to file path)
    m_clipboardButton = new QPushButton("📋", this);
    m_clipboardButton->setFixedSize(24, 24);
    m_clipboardButton->setToolTip("Copy file path to clipboard");
    m_clipboardButton->setStyleSheet(
        "QPushButton {"
        "  background-color: #21262d;"
        "  color: #e6edf3;"
        "  border: 1px solid #30363d;"
        "  border-radius: 4px;"
        "  padding: 2px;"
        "  font-size: 12px;"
        "  margin-left: 8px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #30363d;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #0c7ff2;"
        "}"
    );
    connect(m_clipboardButton, &QPushButton::clicked, this, &HelpDialog::copyFilePathToClipboard);
    m_buttonLayout->addWidget(m_clipboardButton);

    m_buttonLayout->addStretch();

    m_closeButton = new QPushButton("Close", this);
    m_closeButton->setFixedSize(80, 30);
    connect(m_closeButton, &QPushButton::clicked, this, &HelpDialog::closeDialog);

    m_buttonLayout->addWidget(m_closeButton);
    m_mainLayout->addLayout(m_buttonLayout);
    
    // Apply dark theme styling
    setStyleSheet(
        "QDialog {"
        "  background-color: #0d1117;"
        "  color: #e6edf3;"
        "}"
        "QListWidget {"
        "  background-color: #161b22;"
        "  border: 1px solid #30363d;"
        "  border-radius: 6px;"
        "  padding: 5px;"
        "  font-size: 13px;"
        "}"
        "QListWidget::item {"
        "  padding: 8px;"
        "  border-radius: 4px;"
        "  margin: 1px;"
        "}"
        "QListWidget::item:selected {"
        "  background-color: #0c7ff2;"
        "  color: white;"
        "}"
        "QListWidget::item:hover {"
        "  background-color: #21262d;"
        "}"
        "QTextEdit {"
        "  background-color: #161b22;"
        "  border: 1px solid #30363d;"
        "  border-radius: 6px;"
        "  padding: 10px;"
        "  font-size: 13px;"
        "  line-height: 1.5;"
        "}"
        "QPushButton {"
        "  background-color: #21262d;"
        "  color: #e6edf3;"
        "  border: 1px solid #30363d;"
        "  border-radius: 6px;"
        "  padding: 6px 12px;"
        "  font-size: 13px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #30363d;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #0c7ff2;"
        "}"
    );
}

void HelpDialog::createTopicList()
{
    m_topicList = new QListWidget(this);
    m_topicList->setMaximumWidth(300);
    m_topicList->setMinimumWidth(200);
    
    connect(m_topicList, &QListWidget::currentRowChanged, this, &HelpDialog::onTopicSelected);
    
    m_splitter->addWidget(m_topicList);
}

void HelpDialog::createContentArea()
{
    m_contentArea = new QTextEdit(this);
    m_contentArea->setReadOnly(true);
    m_contentArea->setHtml(""); // Will be populated when topics are selected
    
    m_splitter->addWidget(m_contentArea);
}

void HelpDialog::addTopic(const QString &title, const QString &content)
{
    m_topicTitles.append(title);
    m_topicContents.append(content);
    m_topicList->addItem(title);
}

void HelpDialog::onTopicSelected(int row)
{
    if (row >= 0 && row < m_topicContents.size()) {
        m_contentArea->setHtml(m_topicContents[row]);
        m_contentArea->verticalScrollBar()->setValue(0); // Scroll to top
    }
}

void HelpDialog::closeDialog()
{
    accept();
}

void HelpDialog::setupContent()
{
    // Getting Started
    addTopic("🚀 Getting Started", 
        "<h2>Welcome to CalcForge!</h2>"
        "<p>CalcForge is an advanced calculator with powerful features for mathematical calculations, "
        "unit conversions, currency exchange, and much more.</p>"
        
        "<h3>Quick Start:</h3>"
        "<ol>"
        "<li><b>Type expressions</b> in the left column (Expression Editor)</li>"
        "<li><b>See results</b> automatically in the right column (Results Display)</li>"
        "<li><b>Use autocomplete</b> by typing function names or units</li>"
        "<li><b>Reference previous results</b> using LN1, LN2, etc.</li>"
        "</ol>"
        
        "<h3>Interface Overview:</h3>"
        "<ul>"
        "<li><b>Tabs:</b> Create multiple worksheets with the + button</li>"
        "<li><b>Expression Editor:</b> Left side - where you type calculations</li>"
        "<li><b>Results Display:</b> Right side - shows calculated results</li>"
        "<li><b>Line Numbers:</b> Help you reference specific calculations</li>"
        "<li><b>Comments:</b> Lines starting with ::: are comments (no calculation)</li>"
        "<li><b>Always On Top:</b> Click the 📌 pin button to keep CalcForge above other windows</li>"
        "</ul>"
        
        "<p><i>💡 Tip: Try typing 'sqrt(16)' or '100 USD to EUR' to see CalcForge in action!</i></p>");

    // File Operations
    addTopic("💾 File Operations",
        "<h2>Saving and Loading Worksheets</h2>"
        "<p>CalcForge provides powerful file management with automatic recent files tracking and smart defaults.</p>"

        "<h3>File Operations:</h3>"
        "<table border='1' cellpadding='5' style='border-collapse: collapse;'>"
        "<tr><th>Action</th><th>Button</th><th>Keyboard</th><th>Description</th></tr>"
        "<tr><td>Load</td><td>📁</td><td>Ctrl+L</td><td>Open file dialog to load a worksheet</td></tr>"
        "<tr><td>Recent Files</td><td>▼</td><td>-</td><td>Quick access to recently opened files</td></tr>"
        "<tr><td>Save</td><td>💾</td><td>Ctrl+S</td><td>Save current worksheet immediately</td></tr>"
        "<tr><td>Save As</td><td>▼</td><td>-</td><td>Save worksheet with new name/location</td></tr>"
        "</table>"

        "<h3>Window Controls:</h3>"
        "<table border='1' cellpadding='5' style='border-collapse: collapse;'>"
        "<tr><th>Action</th><th>Button</th><th>Keyboard</th><th>Description</th></tr>"
        "<tr><td>Always On Top</td><td>📌</td><td>Ctrl+T</td><td>Toggle window to stay above other applications</td></tr>"
        "</table>"

        "<h3>Startup Behavior:</h3>"
        "<p>CalcForge intelligently chooses what to load on startup:</p>"
        "<ol>"
        "<li><b>Most Recent File:</b> Loads your last opened file automatically</li>"
        "<li><b>Default Worksheets:</b> If no recent files, loads worksheets.json</li>"
        "<li><b>Example Worksheets:</b> If no saved work, loads examples and saves them as your starting point</li>"
        "<li><b>New Worksheet:</b> Creates empty worksheet if no files exist</li>"
        "</ol>"

        "<h3>Save Features:</h3>"
        "<ul>"
        "<li><b>Immediate Save:</b> Save button works instantly - no 'Save As' prompts</li>"
        "<li><b>Smart Defaults:</b> Save As dialog opens in the same folder as current file</li>"
        "<li><b>Auto-naming:</b> Suggests intelligent filenames (e.g., 'project-copy.json')</li>"
        "<li><b>Recent Files:</b> Automatically tracks up to 10 recent files</li>"
        "<li><b>File Protection:</b> Example files are never overwritten</li>"
        "</ul>"

        "<h3>Load Features:</h3>"
        "<ul>"
        "<li><b>File Dialog:</b> Standard OS file picker with JSON/CF file filters</li>"
        "<li><b>Recent Files Menu:</b> Click dropdown arrow for quick access to recent files</li>"
        "<li><b>Auto-cleanup:</b> Non-existent files are automatically removed from recent list</li>"
        "<li><b>Cross-sheet Support:</b> All cross-sheet references are preserved when loading</li>"
        "</ul>"

        "<h3>File Safety:</h3>"
        "<ul>"
        "<li><b>No Auto-save:</b> Files are only saved when you click save buttons</li>"
        "<li><b>Unsaved Changes Warning:</b> Get prompted before closing with unsaved work</li>"
        "<li><b>Confirmation Messages:</b> Clear feedback when files are saved successfully</li>"
        "<li><b>Error Handling:</b> Graceful handling of corrupted or missing files</li>"
        "</ul>"

        "<h3>File Format:</h3>"
        "<p>CalcForge uses JSON format (.json) with support for:</p>"
        "<ul>"
        "<li><b>Multiple Tabs:</b> All worksheets saved in proper order</li>"
        "<li><b>Version Control:</b> Format version tracking for compatibility</li>"
        "<li><b>Cross-platform:</b> Files work on Windows, Mac, and Linux</li>"
        "<li><b>Human-readable:</b> JSON format can be viewed/edited in text editors</li>"
        "</ul>");

    // Basic Math
    addTopic("🔢 Basic Mathematics",
        "<h2>Mathematical Operations</h2>"
        "<p>CalcForge supports all standard mathematical operations with proper order of operations.</p>"
        
        "<h3>Basic Operators:</h3>"
        "<table border='1' cellpadding='5' style='border-collapse: collapse;'>"
        "<tr><th>Operation</th><th>Symbol</th><th>Example</th><th>Result</th></tr>"
        "<tr><td>Addition</td><td>+</td><td>5 + 3</td><td>8</td></tr>"
        "<tr><td>Subtraction</td><td>-</td><td>10 - 4</td><td>6</td></tr>"
        "<tr><td>Multiplication</td><td>*</td><td>7 * 6</td><td>42</td></tr>"
        "<tr><td>Division</td><td>/</td><td>15 / 3</td><td>5</td></tr>"
        "<tr><td>Exponentiation</td><td>^</td><td>2^3</td><td>8</td></tr>"
        "<tr><td>Modulo</td><td>%</td><td>17 % 5</td><td>2</td></tr>"
        "</table>"
        
        "<h3>Order of Operations:</h3>"
        "<ul>"
        "<li><b>Parentheses</b> first: (2 + 3) * 4 = 20</li>"
        "<li><b>Exponents</b> next: 2 + 3^2 = 11</li>"
        "<li><b>Multiplication/Division</b> left to right: 2 + 3 * 4 = 14</li>"
        "<li><b>Addition/Subtraction</b> left to right: 10 - 3 + 2 = 9</li>"
        "</ul>"
        
        "<h3>Mathematical Constants:</h3>"
        "<ul>"
        "<li><b>pi</b> - π (3.14159...)</li>"
        "<li><b>e</b> - Euler's number (2.71828...)</li>"
        "</ul>");

    // LN References (moved up after Basic Mathematics)
    addTopic("🔗 LN References",
        "<h2>Line Number References</h2>"
        "<p>Reference results from other lines using LN variables for powerful calculations.</p>"

        "<h3>Basic LN References:</h3>"
        "<table border='1' cellpadding='5' style='border-collapse: collapse;'>"
        "<tr><th>Reference</th><th>Description</th><th>Example</th></tr>"
        "<tr><td>LN1</td><td>Result from line 1</td><td>LN1 + 10</td></tr>"
        "<tr><td>LN5</td><td>Result from line 5</td><td>LN5 * 2</td></tr>"
        "<tr><td>LN42</td><td>Result from line 42</td><td>sqrt(LN42)</td></tr>"
        "</table>"

        "<h3>LN Reference Features:</h3>"
        "<ul>"
        "<li><b>Auto-update:</b> When you insert/delete lines, LN references update automatically</li>"
        "<li><b>Cross-calculations:</b> Use multiple LN references: LN1 + LN2 + LN3</li>"
        "<li><b>In functions:</b> mean(LN1, LN2, LN3) or sum(LN5, LN10, LN15)</li>"
        "<li><b>Complex expressions:</b> (LN1 + LN2) / (LN3 - LN4)</li>"
        "</ul>"

        "<h3>Cross-Sheet References:</h3>"
        "<p>Reference lines from other sheets using the S function:</p>"
        "<ul>"
        "<li><b>S.SheetName.LN1</b> - Line 1 from 'SheetName'</li>"
        "<li><b>S.Basic Math.LN5</b> - Line 5 from 'Basic Math' sheet</li>"
        "<li><b>Mixed:</b> LN1 + S.Other.LN2 - Local line 1 plus line 2 from 'Other' sheet</li>"
        "</ul>"

        "<h3>LN Reference Tips:</h3>"
        "<ul>"
        "<li><b>Visual highlighting:</b> LN references are color-coded</li>"
        "<li><b>Hover tooltips:</b> Hover over LN references to see their values</li>"
        "<li><b>Smart selection:</b> Ctrl+Left/Right to select LN references</li>"
        "<li><b>Cross-sheet navigation:</b> Ctrl+Enter to jump to referenced line</li>"
        "<li><b>Return navigation:</b> Ctrl+Backspace to return to original expression</li>"
        "</ul>");

    // Mathematical Functions
    addTopic("📐 Mathematical Functions",
        "<h2>Built-in Mathematical Functions</h2>"
        "<p>CalcForge includes 26 verified mathematical functions with autocomplete support.</p>"

        "<h3>Basic Functions:</h3>"
        "<table border='1' cellpadding='5' style='border-collapse: collapse;'>"
        "<tr><th>Function</th><th>Description</th><th>Example</th><th>Result</th></tr>"
        "<tr><td>sqrt(x)</td><td>Square root</td><td>sqrt(16)</td><td>4</td></tr>"
        "<tr><td>abs(x)</td><td>Absolute value</td><td>abs(-5)</td><td>5</td></tr>"
        "<tr><td>round(x, d)</td><td>Round to d decimals</td><td>round(3.14159, 2)</td><td>3.14</td></tr>"
        "<tr><td>ceil(x)</td><td>Round up</td><td>ceil(3.2)</td><td>4</td></tr>"
        "<tr><td>floor(x)</td><td>Round down</td><td>floor(3.8)</td><td>3</td></tr>"
        "</table>"

        "<h3>Trigonometric Functions:</h3>"
        "<table border='1' cellpadding='5' style='border-collapse: collapse;'>"
        "<tr><th>Function</th><th>Description</th><th>Example</th><th>Note</th></tr>"
        "<tr><td>sin(x)</td><td>Sine</td><td>sin(pi/2)</td><td>Radians</td></tr>"
        "<tr><td>cos(x)</td><td>Cosine</td><td>cos(0)</td><td>Radians</td></tr>"
        "<tr><td>tan(x)</td><td>Tangent</td><td>tan(pi/4)</td><td>Radians</td></tr>"
        "</table>"

        "<h3>Logarithmic Functions:</h3>"
        "<table border='1' cellpadding='5' style='border-collapse: collapse;'>"
        "<tr><th>Function</th><th>Description</th><th>Example</th><th>Result</th></tr>"
        "<tr><td>log(x)</td><td>Natural logarithm</td><td>log(e)</td><td>1</td></tr>"
        "<tr><td>log10(x)</td><td>Base-10 logarithm</td><td>log10(100)</td><td>2</td></tr>"
        "<tr><td>exp(x)</td><td>e^x</td><td>exp(1)</td><td>2.718...</td></tr>"
        "</table>");

    // Statistical Functions
    addTopic("📊 Statistical Functions",
        "<h2>Statistical Analysis Functions</h2>"
        "<p>CalcForge provides comprehensive statistical functions with optional rounding.</p>"

        "<h3>Basic Statistics:</h3>"
        "<table border='1' cellpadding='5' style='border-collapse: collapse;'>"
        "<tr><th>Function</th><th>Description</th><th>Example</th></tr>"
        "<tr><td>sum(range)</td><td>Sum of values</td><td>sum(1-5) or sum(above)</td></tr>"
        "<tr><td>mean(range)</td><td>Average</td><td>mean(1-5) or mean(below)</td></tr>"
        "<tr><td>median(range)</td><td>Middle value</td><td>median(1-5)</td></tr>"
        "<tr><td>min(range)</td><td>Minimum value</td><td>min(1-5)</td></tr>"
        "<tr><td>max(range)</td><td>Maximum value</td><td>max(1-5)</td></tr>"
        "<tr><td>count(range)</td><td>Count of values</td><td>count(above)</td></tr>"
        "</table>"

        "<h3>Advanced Statistics:</h3>"
        "<table border='1' cellpadding='5' style='border-collapse: collapse;'>"
        "<tr><th>Function</th><th>Description</th><th>Example</th></tr>"
        "<tr><td>variance(range)</td><td>Variance</td><td>variance(1-10)</td></tr>"
        "<tr><td>stdev(range)</td><td>Standard deviation</td><td>stdev(1-10)</td></tr>"
        "<tr><td>product(range)</td><td>Product of values</td><td>product(1-5)</td></tr>"
        "<tr><td>range(range)</td><td>Max - Min</td><td>range(1-10)</td></tr>"
        "<tr><td>geomean(range)</td><td>Geometric mean</td><td>geomean(1-5)</td></tr>"
        "<tr><td>harmmean(range)</td><td>Harmonic mean</td><td>harmmean(1-5)</td></tr>"
        "<tr><td>sumsq(range)</td><td>Sum of squares</td><td>sumsq(1-5)</td></tr>"
        "</table>"

        "<h3>Range Specifications:</h3>"
        "<ul>"
        "<li><b>Line ranges:</b> 1-5, 10-20, etc.</li>"
        "<li><b>above:</b> All lines above current line</li>"
        "<li><b>below:</b> All lines below current line</li>"
        "</ul>"

        "<h3>Rounding Options:</h3>"
        "<p>Add .2 for 2 decimal places: <code>mean(1-5, .2)</code></p>");

    // Autocomplete System
    addTopic("🤖 Autocomplete System",
        "<h2>Smart Autocomplete Features</h2>"
        "<p>CalcForge features an intelligent 3-step autocomplete system that guides you through complex operations.</p>"

        "<h3>Function Autocomplete (3 Steps):</h3>"
        "<ol>"
        "<li><b>Step 1:</b> Type function name → Select from function list</li>"
        "<li><b>Step 2:</b> Select parameter template → Function becomes complete</li>"
        "<li><b>Step 3:</b> Choose rounding option (statistical functions only)</li>"
        "</ol>"

        "<h3>Unit Conversion Autocomplete (3 Steps):</h3>"
        "<ol>"
        "<li><b>Step 1:</b> Type '100 m' → Select first unit</li>"
        "<li><b>Step 2:</b> Becomes '100 m to ' → Select target unit</li>"
        "<li><b>Step 3:</b> Final result: '100 m to ft'</li>"
        "</ol>"

        "<h3>Cross-Sheet References:</h3>"
        "<ol>"
        "<li><b>Type 'S'</b> → Select S function</li>"
        "<li><b>Becomes 'S.'</b> → Select sheet name</li>"
        "<li><b>Final:</b> S.SheetName.LN1</li>"
        "</ol>"

        "<h3>Autocomplete Tips:</h3>"
        "<ul>"
        "<li><b>Arrow keys</b> to navigate options</li>"
        "<li><b>Enter</b> to select highlighted option</li>"
        "<li><b>Escape</b> to close autocomplete</li>"
        "<li><b>Case insensitive</b> - type 'SIN' or 'sin'</li>"
        "<li><b>Context aware</b> - shows relevant options based on what you're typing</li>"
        "</ul>");

    // Unit Conversions
    addTopic("📏 Unit Conversions",
        "<h2>Unit Conversion System</h2>"
        "<p>Convert between different units using natural language syntax.</p>"

        "<h3>Conversion Syntax:</h3>"
        "<p><code>[number] [from_unit] to [to_unit]</code></p>"

        "<h3>Length Conversions:</h3>"
        "<table border='1' cellpadding='5' style='border-collapse: collapse;'>"
        "<tr><th>Example</th><th>Description</th></tr>"
        "<tr><td>5 feet to meters</td><td>Convert feet to meters</td></tr>"
        "<tr><td>100 inches to centimeters</td><td>Convert inches to cm</td></tr>"
        "<tr><td>1 kilometer to miles</td><td>Convert km to miles</td></tr>"
        "<tr><td>10 yards to meters</td><td>Convert yards to meters</td></tr>"
        "</table>"

        "<h3>Weight/Mass Conversions:</h3>"
        "<table border='1' cellpadding='5' style='border-collapse: collapse;'>"
        "<tr><th>Example</th><th>Description</th></tr>"
        "<tr><td>10 pounds to kilograms</td><td>Convert lbs to kg</td></tr>"
        "<tr><td>500 grams to ounces</td><td>Convert grams to oz</td></tr>"
        "<tr><td>1 ton to pounds</td><td>Convert tons to lbs</td></tr>"
        "</table>"

        "<h3>Temperature Conversions:</h3>"
        "<table border='1' cellpadding='5' style='border-collapse: collapse;'>"
        "<tr><th>Example</th><th>Description</th></tr>"
        "<tr><td>32 fahrenheit to celsius</td><td>Convert °F to °C</td></tr>"
        "<tr><td>100 celsius to fahrenheit</td><td>Convert °C to °F</td></tr>"
        "<tr><td>273 kelvin to celsius</td><td>Convert K to °C</td></tr>"
        "</table>"

        "<h3>Volume Conversions:</h3>"
        "<table border='1' cellpadding='5' style='border-collapse: collapse;'>"
        "<tr><th>Example</th><th>Description</th></tr>"
        "<tr><td>1 gallon to liters</td><td>Convert gallons to liters</td></tr>"
        "<tr><td>500 milliliters to fluid ounces</td><td>Convert ml to fl oz</td></tr>"
        "</table>");

    // Currency Conversions
    addTopic("💰 Currency Conversions",
        "<h2>Currency Exchange System</h2>"
        "<p>Convert between currencies using live exchange rates.</p>"

        "<h3>Currency Syntax:</h3>"
        "<p><code>[amount] [from_currency] to [to_currency]</code></p>"

        "<h3>Common Currency Examples:</h3>"
        "<table border='1' cellpadding='5' style='border-collapse: collapse;'>"
        "<tr><th>Example</th><th>Description</th></tr>"
        "<tr><td>100 USD to EUR</td><td>US Dollars to Euros</td></tr>"
        "<tr><td>50 GBP to JPY</td><td>British Pounds to Japanese Yen</td></tr>"
        "<tr><td>1000 CAD to USD</td><td>Canadian to US Dollars</td></tr>"
        "<tr><td>25 EUR to GBP</td><td>Euros to British Pounds</td></tr>"
        "</table>"

        "<h3>Supported Currencies:</h3>"
        "<p>CalcForge supports 150+ currencies including:</p>"
        "<ul>"
        "<li><b>Major:</b> USD, EUR, GBP, JPY, CAD, AUD, CHF</li>"
        "<li><b>Asian:</b> CNY, INR, KRW, SGD, THB, HKD</li>"
        "<li><b>European:</b> SEK, NOK, DKK, PLN, CZK</li>"
        "<li><b>Others:</b> BRL, MXN, ZAR, NZD, and many more</li>"
        "</ul>"

        "<h3>Exchange Rate Updates:</h3>"
        "<ul>"
        "<li><b>$ Button:</b> Click the $ button to update exchange rates</li>"
        "<li><b>Live rates:</b> Rates are fetched from financial APIs</li>"
        "<li><b>Offline mode:</b> Uses cached rates when internet unavailable</li>"
        "</ul>");

    // Special Functions
    addTopic("⚡ Special Functions",
        "<h2>Advanced Calculation Functions</h2>"
        "<p>CalcForge includes specialized functions for professional workflows.</p>"

        "<h3>Percentage Calculations:</h3>"
        "<table border='1' cellpadding='5' style='border-collapse: collapse;'>"
        "<tr><th>Function</th><th>Example</th><th>Description</th></tr>"
        "<tr><td>Basic %</td><td>percent(25%, 1000)</td><td>25% of 1000 = 250</td></tr>"
        "<tr><td>Reverse %</td><td>percent(250, %, 1000)</td><td>What % is 250 of 1000? = 25%</td></tr>"
        "<tr><td>Increase</td><td>percent(1000, +, 25%)</td><td>1000 + 25% = 1250</td></tr>"
        "<tr><td>Decrease</td><td>percent(1000, -, 15%)</td><td>1000 - 15% = 850</td></tr>"
        "<tr><td>Change</td><td>percent(1000, to, 1200)</td><td>% change from 1000 to 1200 = 20%</td></tr>"
        "</table>"

        "<h3>Timecode Calculations:</h3>"
        "<p>Professional video/audio timecode arithmetic using the TC function:</p>"
        "<table border='1' cellpadding='5' style='border-collapse: collapse;'>"
        "<tr><th>Function</th><th>Example</th><th>Description</th></tr>"
        "<tr><td>Frames to TC</td><td>TC(24, 100)</td><td>Convert 100 frames to timecode at 24fps</td></tr>"
        "<tr><td>TC to Frames</td><td>TC(30, 00:01:00:00)</td><td>Convert timecode to frames at 30fps</td></tr>"
        "<tr><td>TC Math</td><td>TC(24, 00:01:00:00 + 00:00:30:00)</td><td>Add timecodes</td></tr>"
        "</table>"

        "<h3>Aspect Ratio Calculations:</h3>"
        "<p>Calculate missing dimensions using the AR function:</p>"
        "<table border='1' cellpadding='5' style='border-collapse: collapse;'>"
        "<tr><th>Function</th><th>Example</th><th>Description</th></tr>"
        "<tr><td>Find Width</td><td>AR(1920x1080, ?x720)</td><td>Find width for 720p height</td></tr>"
        "<tr><td>Find Height</td><td>AR(1920x1080, 1280x?)</td><td>Find height for 1280 width</td></tr>"
        "<tr><td>From Ratio</td><td>AR(16x9, ?x1080)</td><td>Find width from aspect ratio</td></tr>"
        "</table>"

        "<h3>Date Calculations:</h3>"
        "<p>Date arithmetic using the D function:</p>"
        "<table border='1' cellpadding='5' style='border-collapse: collapse;'>"
        "<tr><th>Function</th><th>Example</th><th>Description</th></tr>"
        "<tr><td>Add Days</td><td>D(July 4, 2023 + 30)</td><td>Add 30 calendar days</td></tr>"
        "<tr><td>Business Days</td><td>D(July 4, 2023 W+ 5)</td><td>Add 5 business days</td></tr>"
        "<tr><td>Date Diff</td><td>D(July 4, 2023 - June 1, 2023)</td><td>Days between dates</td></tr>"
        "<tr><td>Business Diff</td><td>D(July 4, 2023 W- June 1, 2023)</td><td>Business days between</td></tr>"
        "</table>");

    // Keyboard Shortcuts
    addTopic("⌨️ Keyboard Shortcuts",
        "<h2>Keyboard Shortcuts & Navigation</h2>"
        "<p>Boost your productivity with these keyboard shortcuts.</p>"

        "<h3>Font Size Control:</h3>"
        "<table border='1' cellpadding='5' style='border-collapse: collapse;'>"
        "<tr><th>Shortcut</th><th>Action</th></tr>"
        "<tr><td>Ctrl + Period (.)</td><td>Increase font size</td></tr>"
        "<tr><td>Ctrl + Comma (,)</td><td>Decrease font size</td></tr>"
        "<tr><td>Ctrl + 0</td><td>Reset font size to default</td></tr>"
        "</table>"

        "<h3>Tab Navigation:</h3>"
        "<table border='1' cellpadding='5' style='border-collapse: collapse;'>"
        "<tr><th>Shortcut</th><th>Action</th></tr>"
        "<tr><td>Ctrl + PageUp</td><td>Previous tab</td></tr>"
        "<tr><td>Ctrl + PageDown</td><td>Next tab</td></tr>"
        "<tr><td>Double-click tab</td><td>Rename tab</td></tr>"
        "<tr><td>Click X on tab</td><td>Close tab</td></tr>"
        "</table>"

        "<h3>Text Selection & Navigation:</h3>"
        "<table border='1' cellpadding='5' style='border-collapse: collapse;'>"
        "<tr><th>Shortcut</th><th>Action</th></tr>"
        "<tr><td>Ctrl + Left/Right</td><td>Smart selection (numbers, LN refs)</td></tr>"
        "<tr><td>Ctrl + Up/Down</td><td>Navigate nested parentheses</td></tr>"
        "<tr><td>Ctrl + Down</td><td>Select entire line</td></tr>"
        "<tr><td>Ctrl + C (no selection)</td><td>Copy current line result</td></tr>"
        "<tr><td>Ctrl + A</td><td>Select all text</td></tr>"
        "</table>"

        "<h3>Cross-Sheet Navigation:</h3>"
        "<table border='1' cellpadding='5' style='border-collapse: collapse;'>"
        "<tr><th>Shortcut</th><th>Action</th></tr>"
        "<tr><td>Ctrl + Enter</td><td>Jump to cross-sheet line reference</td></tr>"
        "<tr><td>Ctrl + Backspace</td><td>Return to original cross-sheet expression</td></tr>"
        "</table>"

        "<h3>File Operations:</h3>"
        "<table border='1' cellpadding='5' style='border-collapse: collapse;'>"
        "<tr><th>Shortcut</th><th>Action</th></tr>"
        "<tr><td>Ctrl + S</td><td>Save current worksheet</td></tr>"
        "<tr><td>Ctrl + L</td><td>Load worksheet file</td></tr>"
        "</table>"

        "<h3>Window Controls:</h3>"
        "<table border='1' cellpadding='5' style='border-collapse: collapse;'>"
        "<tr><th>Shortcut</th><th>Action</th></tr>"
        "<tr><td>Ctrl + T</td><td>Toggle always on top</td></tr>"
        "</table>"

        "<h3>Autocomplete Navigation:</h3>"
        "<table border='1' cellpadding='5' style='border-collapse: collapse;'>"
        "<tr><th>Key</th><th>Action</th></tr>"
        "<tr><td>↑ ↓ Arrow Keys</td><td>Navigate autocomplete options</td></tr>"
        "<tr><td>Enter</td><td>Select highlighted option</td></tr>"
        "<tr><td>Escape</td><td>Close autocomplete</td></tr>"
        "<tr><td>Tab</td><td>Accept first suggestion</td></tr>"
        "</table>");

    // Tips and Tricks
    addTopic("💡 Tips & Tricks",
        "<h2>Pro Tips for CalcForge</h2>"
        "<p>Advanced techniques to maximize your CalcForge experience.</p>"

        "<h3>Workflow Tips:</h3>"
        "<ul>"
        "<li><b>Comments:</b> Use ::: to add comments that don't calculate</li>"
        "<li><b>Organization:</b> Use blank lines and comments to organize complex calculations</li>"
        "<li><b>Templates:</b> Save common calculation patterns as templates</li>"
        "<li><b>Cross-references:</b> Use multiple sheets for different calculation categories</li>"
        "</ul>"

        "<h3>File Management Tips:</h3>"
        "<ul>"
        "<li><b>Regular Saving:</b> Use the 💾 save button frequently to avoid losing work</li>"
        "<li><b>Project Organization:</b> Use descriptive filenames and keep related files together</li>"
        "<li><b>Recent Files:</b> Use the dropdown arrow next to load button for quick access</li>"
        "<li><b>Backup Strategy:</b> Save important calculations with different names using Save As</li>"
        "<li><b>Version Control:</b> Add dates or version numbers to filenames for tracking changes</li>"
        "</ul>"

        "<h3>Calculation Tips:</h3>"
        "<ul>"
        "<li><b>Parentheses:</b> Use liberally to ensure correct order of operations</li>"
        "<li><b>Constants:</b> Use 'pi' and 'e' instead of decimal approximations</li>"
        "<li><b>Precision:</b> Add rounding to statistical functions: mean(1-10, .3)</li>"
        "<li><b>Units:</b> Always specify units for conversions: '100 meters to feet'</li>"
        "</ul>"

        "<h3>Performance Tips:</h3>"
        "<ul>"
        "<li><b>Large datasets:</b> Use statistical functions instead of manual calculations</li>"
        "<li><b>Cross-sheet refs:</b> Minimize excessive cross-sheet references for better performance</li>"
        "<li><b>Currency updates:</b> Update exchange rates periodically with $ button</li>"
        "</ul>"

        "<h3>Troubleshooting:</h3>"
        "<ul>"
        "<li><b>Red text:</b> Indicates calculation errors - check syntax</li>"
        "<li><b>Missing results:</b> Ensure proper function syntax and parameters</li>"
        "<li><b>LN references:</b> Make sure referenced lines contain valid numbers</li>"
        "<li><b>Units:</b> Check spelling of unit names in conversions</li>"
        "</ul>"

        "<h3>Advanced Features:</h3>"
        "<ul>"
        "<li><b>Hover tooltips:</b> Hover over LN references to see their values</li>"
        "<li><b>Visual highlighting:</b> LN references and cross-sheet refs are color-coded</li>"
        "<li><b>Auto-updates:</b> LN references update automatically when lines are inserted/deleted</li>"
        "<li><b>Synchronized scrolling:</b> Expression and results columns scroll together</li>"
        "</ul>");
}

void HelpDialog::setCurrentFilePath(const QString &filePath)
{
    if (m_filePathLabel) {
        if (filePath.isEmpty()) {
            m_filePathLabel->setText("No file loaded");
            // Disable clipboard button when no file is loaded
            if (m_clipboardButton) {
                m_clipboardButton->setEnabled(false);
            }
        } else {
            // Show just the filename and directory for better readability
            QFileInfo fileInfo(filePath);
            QString displayPath = QString("Current file: %1").arg(filePath);
            m_filePathLabel->setText(displayPath);
            m_filePathLabel->setToolTip(filePath); // Full path in tooltip
            // Enable clipboard button when file is loaded
            if (m_clipboardButton) {
                m_clipboardButton->setEnabled(true);
            }
        }
    }

    // Store the current file path for clipboard copying
    m_currentFilePath = filePath;
}

void HelpDialog::copyFilePathToClipboard()
{
    if (!m_currentFilePath.isEmpty()) {
        QClipboard *clipboard = QApplication::clipboard();
        clipboard->setText(m_currentFilePath);

        // Provide visual feedback by temporarily changing the button text
        QString originalText = m_clipboardButton->text();
        m_clipboardButton->setText("✓");
        m_clipboardButton->setToolTip("Copied to clipboard!");

        // Reset the button after a short delay
        QTimer::singleShot(1000, [this, originalText]() {
            if (m_clipboardButton) {
                m_clipboardButton->setText(originalText);
                m_clipboardButton->setToolTip("Copy file path to clipboard");
            }
        });
    }
}
