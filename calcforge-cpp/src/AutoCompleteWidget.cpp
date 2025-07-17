#include "AutoCompleteWidget.h"
#include "ExpressionEditor.h"
#include "MainWindow.h"
#include "Logger.h"
#include <QApplication>
#include <QScrollBar>
#include <QTextCursor>
#include <QTextBlock>

// Static constants
const int AutoCompleteWidget::POPUP_WIDTH = 300;
const int AutoCompleteWidget::POPUP_HEIGHT = 200;
const int AutoCompleteWidget::DESCRIPTION_WIDTH = 250;
const int AutoCompleteManager::SHOW_DELAY_MS = 100;

const QString AutoCompleteWidget::POPUP_STYLE = R"(
    QWidget {
        background-color: #161B22;
        border: 1px solid #30363D;
        border-radius: 6px;
    }
    QListWidget {
        background-color: #161B22;
        color: #E6EDF3;
        border: 1px solid #30363D;
        border-radius: 6px;
        selection-background-color: #0969DA;
        selection-color: #FFFFFF;
        padding: 4px;
        font-family: 'Roboto Mono', 'Consolas', 'Courier New', monospace;
        font-size: 13pt;
        outline: none;
    }
    QListWidget::item {
        padding: 6px 12px;
        border: none;
        color: #E6EDF3;
        background-color: transparent;
        border-radius: 4px;
        margin: 1px;
    }
    QListWidget::item:selected {
        background-color: #0969DA;
        color: #FFFFFF;
    }
    QListWidget::item:hover {
        background-color: #21262D;
        color: #E6EDF3;
    }
    QLabel {
        background-color: #21262D;
        color: #E6EDF3;
        border: 1px solid #30363D;
        border-radius: 6px;
        padding: 8px;
        font-family: 'Roboto Mono', 'Consolas', 'Courier New', monospace;
        font-size: 9pt;
    }
)";

// AutoCompleteList Implementation
AutoCompleteList::AutoCompleteList(QWidget *parent)
    : QListWidget(parent)
{
    // Don't set window flags - this is a child widget, not a top-level window
    setFocusPolicy(Qt::NoFocus);
    setMouseTracking(true);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setUniformItemSizes(true);
    setSelectionMode(QAbstractItemView::SingleSelection);

    LOG_DEBUG("AutoCompleteList: Created list widget");
}

void AutoCompleteList::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
        case Qt::Key_Up:
        case Qt::Key_Down:
        case Qt::Key_PageUp:
        case Qt::Key_PageDown:
        case Qt::Key_Home:
        case Qt::Key_End:
            QListWidget::keyPressEvent(event);
            break;
        case Qt::Key_Return:
        case Qt::Key_Enter:
        case Qt::Key_Tab:
            if (currentItem()) {
                emit itemSelected(currentItem()->text());
            }
            event->accept();
            break;
        case Qt::Key_Escape:
            emit escapePressed();
            event->accept();
            break;
        default:
            event->ignore();
            break;
    }
}

void AutoCompleteList::mousePressEvent(QMouseEvent *event)
{
    QListWidget::mousePressEvent(event);
    if (event->button() == Qt::LeftButton && currentItem()) {
        emit itemSelected(currentItem()->text());
    }
}

// AutoCompleteDescriptionBox Implementation
AutoCompleteDescriptionBox::AutoCompleteDescriptionBox(QWidget *parent)
    : QLabel(parent)
{
    setupStyling();
    setWordWrap(true);
    setAlignment(Qt::AlignTop | Qt::AlignLeft);
    setText("Select a function to see its description");
}

void AutoCompleteDescriptionBox::setupStyling()
{
    // Don't set window flags - this is a child widget, not a top-level window
    setFocusPolicy(Qt::NoFocus);

    LOG_DEBUG("AutoCompleteDescriptionBox: Setup styling completed");
}

void AutoCompleteDescriptionBox::updateDescription(const QString &text)
{
    setText(text);
}

// AutoCompleteWidget Implementation
AutoCompleteWidget::AutoCompleteWidget(QWidget *parent)
    : QWidget(parent)
    , m_mainLayout(nullptr)
    , m_listWidget(nullptr)
    , m_descriptionBox(nullptr)
    , m_selectedIndex(0)
{
    setupUI();
    setupStyling();
    hide();
}

AutoCompleteWidget::~AutoCompleteWidget()
{
    // Qt handles cleanup automatically
}

void AutoCompleteWidget::setupUI()
{
    // Use ToolTip which worked before, but with better focus handling
    setWindowFlags(Qt::ToolTip | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setAttribute(Qt::WA_X11DoNotAcceptFocus);
    setFocusPolicy(Qt::NoFocus);

    // Ensure this widget never accepts focus
    setFocusProxy(nullptr);
    
    m_mainLayout = new QHBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(2);
    
    // Create list widget
    m_listWidget = new AutoCompleteList(this);
    m_listWidget->setFixedSize(POPUP_WIDTH, POPUP_HEIGHT);
    
    // Create description box
    m_descriptionBox = new AutoCompleteDescriptionBox(this);
    m_descriptionBox->setFixedSize(DESCRIPTION_WIDTH, POPUP_HEIGHT);
    
    // Add to layout
    m_mainLayout->addWidget(m_listWidget);
    m_mainLayout->addWidget(m_descriptionBox);
    
    // Connect signals
    connect(m_listWidget, &AutoCompleteList::itemSelected, this, &AutoCompleteWidget::itemSelected);
    connect(m_listWidget, &AutoCompleteList::escapePressed, this, &AutoCompleteWidget::cancelled);
    connect(m_listWidget, &QListWidget::currentRowChanged, this, &AutoCompleteWidget::onItemSelectionChanged);
    connect(m_listWidget, &QListWidget::itemClicked, this, &AutoCompleteWidget::onItemClicked);
    
    // Install event filter to catch clicks outside
    qApp->installEventFilter(this);
}

void AutoCompleteWidget::setupStyling()
{
    setStyleSheet(POPUP_STYLE);
    setFixedSize(POPUP_WIDTH + DESCRIPTION_WIDTH + 2, POPUP_HEIGHT);
}

void AutoCompleteWidget::showCompletions(const QStringList &completions, const QStringList &descriptions, const QPoint &position)
{
    if (completions.isEmpty()) {
        hideCompletions();
        return;
    }

    m_completions = completions;
    m_descriptions = descriptions;

    LOG_DEBUG(QString("AutoCompleteWidget: Setting up list with %1 items: %2").arg(completions.size()).arg(completions.join(", ")));
    LOG_DEBUG(QString("AutoCompleteWidget: Received %1 descriptions: %2").arg(descriptions.size()).arg(descriptions.join(" | ")));
    LOG_DEBUG(QString("AutoCompleteWidget: Received %1 descriptions: %2").arg(descriptions.size()).arg(descriptions.join(" | ")));

    // Update list widget
    m_listWidget->clear();
    m_listWidget->addItems(completions);

    // Force the list widget to be visible and update
    m_listWidget->show();
    m_listWidget->update();

    // Select first item
    m_selectedIndex = 0;
    if (m_listWidget->count() > 0) {
        m_listWidget->setCurrentRow(0);
        updateDescriptionForCurrentItem();
        LOG_DEBUG(QString("AutoCompleteWidget: Selected first item: %1").arg(m_listWidget->currentItem()->text()));
    }

    // Force description box to be visible
    m_descriptionBox->show();
    m_descriptionBox->update();

    // Position and show
    positionWidget(position);

    LOG_DEBUG(QString("AutoCompleteWidget: About to show widget - isVisible before: %1").arg(isVisible()));
    show();
    raise();

    // DON'T call activateWindow() - that steals focus!
    // activateWindow();

    // Force update of all child widgets
    update();
    repaint();

    // Ensure the editor keeps focus - get reference from AutoCompleteManager
    // The parent hierarchy should be: AutoCompleteWidget -> AutoCompleteManager -> ExpressionEditor
    // But let's use a more direct approach through the manager

    LOG_DEBUG(QString("AutoCompleteWidget: After show() - isVisible: %1, geometry: (%2,%3,%4,%5)")
              .arg(isVisible()).arg(geometry().x()).arg(geometry().y()).arg(geometry().width()).arg(geometry().height()));
    LOG_DEBUG(QString("AutoCompleteWidget: List widget - isVisible: %1, count: %2")
              .arg(m_listWidget->isVisible()).arg(m_listWidget->count()));

    LOG_DEBUG(QString("AutoComplete: Showing %1 completions").arg(completions.size()));
}

void AutoCompleteWidget::hideCompletions()
{
    hide();
    m_completions.clear();
    m_descriptions.clear();
    m_selectedIndex = 0;
}

bool AutoCompleteWidget::isVisible() const
{
    return QWidget::isVisible();
}

void AutoCompleteWidget::selectNext()
{
    if (m_listWidget->count() > 0) {
        int newIndex = (m_selectedIndex + 1) % m_listWidget->count();
        m_selectedIndex = newIndex;
        m_listWidget->setCurrentRow(newIndex);
        updateDescriptionForCurrentItem();
    }
}

void AutoCompleteWidget::selectPrevious()
{
    if (m_listWidget->count() > 0) {
        int newIndex = (m_selectedIndex - 1 + m_listWidget->count()) % m_listWidget->count();
        m_selectedIndex = newIndex;
        m_listWidget->setCurrentRow(newIndex);
        updateDescriptionForCurrentItem();
    }
}

void AutoCompleteWidget::selectFirst()
{
    if (m_listWidget->count() > 0) {
        m_selectedIndex = 0;
        m_listWidget->setCurrentRow(0);
        updateDescriptionForCurrentItem();
    }
}

QString AutoCompleteWidget::getCurrentSelection() const
{
    if (m_selectedIndex >= 0 && m_selectedIndex < m_completions.size()) {
        return m_completions[m_selectedIndex];
    }
    return QString();
}

void AutoCompleteWidget::handleKeyPress(QKeyEvent *event)
{
    // Forward key events to the list widget
    switch (event->key()) {
        case Qt::Key_Up:
            selectPrevious();
            event->accept();
            break;
        case Qt::Key_Down:
            selectNext();
            event->accept();
            break;
        case Qt::Key_Return:
        case Qt::Key_Enter:
        case Qt::Key_Tab:
            if (!getCurrentSelection().isEmpty()) {
                emit itemSelected(getCurrentSelection());
                event->accept();
            }
            break;
        case Qt::Key_Escape:
            emit cancelled();
            event->accept();
            break;
        default:
            event->ignore();
            break;
    }
}

void AutoCompleteWidget::keyPressEvent(QKeyEvent *event)
{
    handleKeyPress(event);
}

void AutoCompleteWidget::focusOutEvent(QFocusEvent *event)
{
    Q_UNUSED(event)
    // Don't hide on focus out - we handle this differently
}

bool AutoCompleteWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress && isVisible()) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        if (!geometry().contains(mapFromGlobal(mouseEvent->globalPosition().toPoint()))) {
            // Click outside the popup - hide it
            hideCompletions();
            emit cancelled();
            return false; // Don't consume the event
        }
    }
    return QWidget::eventFilter(obj, event);
}

void AutoCompleteWidget::onItemSelectionChanged()
{
    m_selectedIndex = m_listWidget->currentRow();
    updateDescriptionForCurrentItem();
}

void AutoCompleteWidget::onItemClicked(QListWidgetItem *item)
{
    if (item) {
        emit itemSelected(item->text());
    }
}

void AutoCompleteWidget::positionWidget(const QPoint &position)
{
    // Find the screen that contains the given position (where CalcForge is)
    QScreen *screen = QApplication::screenAt(position);
    if (!screen) {
        // Fallback to primary screen if position is not on any screen
        screen = QApplication::primaryScreen();
    }

    QRect screenGeometry = screen->availableGeometry();

    LOG_DEBUG(QString("AutoCompleteWidget: Using screen geometry: (%1,%2,%3,%4)")
              .arg(screenGeometry.x()).arg(screenGeometry.y()).arg(screenGeometry.width()).arg(screenGeometry.height()));
    LOG_DEBUG(QString("AutoCompleteWidget: Requested position: (%1,%2)").arg(position.x()).arg(position.y()));

    QPoint newPos = position;

    // Ensure popup stays within the correct screen bounds
    if (newPos.x() < screenGeometry.left()) {
        newPos.setX(screenGeometry.left() + 10);
    }
    if (newPos.x() + width() > screenGeometry.right()) {
        newPos.setX(screenGeometry.right() - width() - 10);
    }

    if (newPos.y() < screenGeometry.top()) {
        newPos.setY(screenGeometry.top() + 10);
    }
    if (newPos.y() + height() > screenGeometry.bottom()) {
        newPos.setY(position.y() - height() - 5); // Show above cursor
        if (newPos.y() < screenGeometry.top()) {
            newPos.setY(screenGeometry.top() + 10); // Fallback to top of screen
        }
    }

    LOG_DEBUG(QString("AutoCompleteWidget: Final position: (%1,%2) on screen (%3,%4,%5,%6)")
              .arg(newPos.x()).arg(newPos.y())
              .arg(screenGeometry.x()).arg(screenGeometry.y()).arg(screenGeometry.width()).arg(screenGeometry.height()));
    move(newPos);
}

void AutoCompleteWidget::updateDescriptionForCurrentItem()
{
    LOG_DEBUG(QString("AutoCompleteWidget::updateDescriptionForCurrentItem: selectedIndex=%1, descriptions.size()=%2")
              .arg(m_selectedIndex).arg(m_descriptions.size()));

    if (m_selectedIndex >= 0 && m_selectedIndex < m_descriptions.size()) {
        QString description = m_descriptions[m_selectedIndex];
        LOG_DEBUG(QString("AutoCompleteWidget: Setting description for index %1: '%2'").arg(m_selectedIndex).arg(description));
        m_descriptionBox->updateDescription(description);
    } else {
        LOG_DEBUG(QString("AutoCompleteWidget: No valid description - using fallback"));
        m_descriptionBox->updateDescription("No description available");
    }
}

// AutoCompleteManager Implementation
AutoCompleteManager::AutoCompleteManager(ExpressionEditor *editor, QWidget *parent)
    : QObject(parent)
    , m_editor(editor)
    , m_widget(nullptr)
    , m_wordStartPos(0)
    , m_showTimer(nullptr)
    , m_startupDelay(true)
{
    // Create UI widget
    m_widget = new AutoCompleteWidget(parent);

    // Setup timer for delayed showing
    m_showTimer = new QTimer(this);
    m_showTimer->setSingleShot(true);
    m_showTimer->setInterval(SHOW_DELAY_MS);

    // Connect signals
    connect(m_widget, &AutoCompleteWidget::itemSelected, this, &AutoCompleteManager::onItemSelected);
    connect(m_widget, &AutoCompleteWidget::cancelled, this, &AutoCompleteManager::onCancelled);
    connect(m_showTimer, &QTimer::timeout, this, &AutoCompleteManager::showAutocomplete);

    // Setup data
    setupFunctions();
    setupUnits();
    setupCurrencies();

    // Disable autocomplete for the first 2 seconds to prevent startup glitches
    QTimer::singleShot(2000, this, [this]() {
        m_startupDelay = false;
        LOG_DEBUG("AutoCompleteManager: Startup delay ended, autocomplete now enabled");
    });

    LOG_DEBUG("AutoCompleteManager initialized with startup delay");
}

AutoCompleteManager::~AutoCompleteManager()
{
    if (m_widget) {
        delete m_widget;
    }
}

void AutoCompleteManager::setupFunctions()
{
    // Mathematical functions
    m_functions["sqrt"] = AutoCompleteFunction("sqrt", "Square root function", {"value"});
    m_functions["sin"] = AutoCompleteFunction("sin", "Sine function (radians)", {"angle"});
    m_functions["cos"] = AutoCompleteFunction("cos", "Cosine function (radians)", {"angle"});
    m_functions["tan"] = AutoCompleteFunction("tan", "Tangent function (radians)", {"angle"});
    m_functions["log"] = AutoCompleteFunction("log", "Natural logarithm", {"value"});
    m_functions["log10"] = AutoCompleteFunction("log10", "Base-10 logarithm", {"value"});
    m_functions["exp"] = AutoCompleteFunction("exp", "Exponential function (e^x)", {"value"});
    m_functions["abs"] = AutoCompleteFunction("abs", "Absolute value", {"value"});
    m_functions["ceil"] = AutoCompleteFunction("ceil", "Round up to nearest integer", {"value"});
    m_functions["floor"] = AutoCompleteFunction("floor", "Round down to nearest integer", {"value"});
    m_functions["round"] = AutoCompleteFunction("round", "Round to specified decimal places", {"value", "decimals"});

    // Statistical functions
    m_functions["sum"] = AutoCompleteFunction("sum", "Sum of values", {"range"}, "statistical");
    m_functions["mean"] = AutoCompleteFunction("mean", "Average of values", {"range"}, "statistical");
    m_functions["min"] = AutoCompleteFunction("min", "Minimum value", {"range"}, "statistical");
    m_functions["max"] = AutoCompleteFunction("max", "Maximum value", {"range"}, "statistical");
    m_functions["count"] = AutoCompleteFunction("count", "Count of values", {"range"}, "statistical");
    m_functions["median"] = AutoCompleteFunction("median", "Median value", {"range"}, "statistical");
    m_functions["variance"] = AutoCompleteFunction("variance", "Variance of values", {"range"}, "statistical");
    m_functions["stdev"] = AutoCompleteFunction("stdev", "Standard deviation", {"range"}, "statistical");

    // Special functions
    m_functions["TC"] = AutoCompleteFunction("TC", "Timecode calculation", {"framerate", "timecode"}, "special");
    m_functions["AR"] = AutoCompleteFunction("AR", "Aspect ratio calculation", {"dimensions"}, "special");
    m_functions["D"] = AutoCompleteFunction("D", "Date calculation", {"date_expression"}, "special");
    m_functions["percent"] = AutoCompleteFunction("percent", "Percentage calculation", {"value", "operation", "value2"}, "special");

    // Cross-sheet function
    m_functions["S"] = AutoCompleteFunction("S", "Cross-sheet reference", {"sheet", "reference"}, "reference");

    // Constants
    m_functions["pi"] = AutoCompleteFunction("pi", "Mathematical constant π (3.14159...)", {}, "constant");
    m_functions["e"] = AutoCompleteFunction("e", "Mathematical constant e (2.71828...)", {}, "constant");

    // Setup parameter templates for statistical functions
    QStringList statParams = {"above", "below", "1-10", "1,2,3,4,5"};
    m_functionParameters["sum"] = statParams;
    m_functionParameters["mean"] = statParams;
    m_functionParameters["min"] = statParams;
    m_functionParameters["max"] = statParams;
    m_functionParameters["count"] = statParams;
    m_functionParameters["median"] = statParams;
    m_functionParameters["variance"] = statParams;
    m_functionParameters["stdev"] = statParams;

    // Setup parameter templates for special functions
    // TC function - first parameter (framerate)
    m_functionParameters["TC"] = {"24", "29.97", "30", "23.976", "25", "50", "59.94", "60"};
    m_functionParameters["tc"] = {"24", "29.97", "30", "23.976", "25", "50", "59.94", "60"};
    // TC function - second parameter (timecode expressions with usage examples)
    m_functionParameters["TC_param2"] = {
        "100",
        "00:00:10:00",
        "00:00:10:00 + 00:00:05:00",
        "00:00:10:00 - 00:00:05:00",
        "00:00:10:00 * 2",
        "00:00:10:00 / 2"
    };
    m_functionParameters["tc_param2"] = {
        "100",
        "00:00:10:00",
        "00:00:10:00 + 00:00:05:00",
        "00:00:10:00 - 00:00:05:00",
        "00:00:10:00 * 2",
        "00:00:10:00 / 2"
    };
    // D function - date expressions with usage examples
    m_functionParameters["D"] = {
        "July 4, 2023",
        "July 4, 2023 + 30",
        "July 4, 2023 - 30",
        "July 4, 2023 W+ 5",
        "July 4, 2023 W- 5",
        "July 4, 2023 - January 1, 2023"
    };
    m_functionParameters["d"] = {
        "July 4, 2023",
        "July 4, 2023 + 30",
        "July 4, 2023 - 30",
        "July 4, 2023 W+ 5",
        "July 4, 2023 W- 5",
        "July 4, 2023 - January 1, 2023"
    };

    // Setup parameter templates for other functions
    m_functionParameters["AR"] = {"\"1920x1080\"", "\"1280x720\"", "\"3840x2160\"", "\"16:9\"", "\"4:3\""};
    m_functionParameters["ar"] = {"\"1920x1080\"", "\"1280x720\"", "\"3840x2160\"", "\"16:9\"", "\"4:3\""};
    // Percent function - percentage expressions with usage examples
    m_functionParameters["percent"] = {
        "25%, 1000",
        "250, %, 1000",
        "1000, +, 25%",
        "1000, -, 25%",
        "1000, to, 1200"
    };
    m_functionParameters["TR"] = {"2", "0", "1", "3", "4"};
    m_functionParameters["tr"] = {"2", "0", "1", "3", "4"};
    m_functionParameters["truncate"] = {"2", "0", "1", "3", "4"};

    // Setup function descriptions
    for (auto it = m_functions.begin(); it != m_functions.end(); ++it) {
        m_functionDescriptions[it.key()] = it.value().description;
    }

    LOG_DEBUG(QString("Setup %1 functions for autocomplete").arg(m_functions.size()));
}

void AutoCompleteManager::setupUnits()
{
    // Distance units
    m_units << "meters" << "kilometers" << "miles" << "yards" << "feet" << "inches" << "centimeters" << "millimeters";
    m_units << "m" << "km" << "mi" << "yd" << "ft" << "in" << "cm" << "mm";

    // Weight units
    m_units << "pounds" << "kilograms" << "grams" << "ounces" << "tons";
    m_units << "lbs" << "kg" << "g" << "oz" << "t";

    // Volume units
    m_units << "liters" << "gallons" << "quarts" << "pints" << "cups" << "milliliters";
    m_units << "l" << "gal" << "qt" << "pt" << "cup" << "ml";

    // Temperature units
    m_units << "celsius" << "fahrenheit" << "kelvin";
    m_units << "C" << "F" << "K";

    // Time units
    m_units << "seconds" << "minutes" << "hours" << "days" << "weeks" << "months" << "years";
    m_units << "s" << "min" << "h" << "d" << "w" << "mo" << "y";

    // Conversion keywords
    m_units << "to";

    LOG_DEBUG(QString("Setup %1 units for autocomplete").arg(m_units.size()));
}

void AutoCompleteManager::setupCurrencies()
{
    // Major currencies
    m_currencies << "USD" << "EUR" << "GBP" << "JPY" << "CNY" << "CAD" << "AUD" << "CHF" << "SEK" << "NOK";
    m_currencies << "dollars" << "euros" << "pounds" << "yen" << "yuan" << "canadian" << "australian";
    m_currencies << "usd" << "eur" << "gbp" << "jpy" << "cny" << "cad" << "aud" << "chf" << "sek" << "nok";

    // Conversion keywords
    m_currencies << "to";

    LOG_DEBUG(QString("Setup %1 currencies for autocomplete").arg(m_currencies.size()));
}

void AutoCompleteManager::showAutocomplete()
{
    LOG_DEBUG("AutoCompleteManager: showAutocomplete() called");

    if (!m_editor) {
        LOG_DEBUG("AutoCompleteManager: No editor available");
        return;
    }

    // Check if we're on a comment line - disable autocomplete for comments
    QTextCursor commentCursor = m_editor->textCursor();
    QString commentLineText = commentCursor.block().text().trimmed();
    if (commentLineText.startsWith(":::")) {
        LOG_DEBUG("AutoCompleteManager: On comment line - hiding autocomplete");
        hideAutocomplete();
        return;
    }

    QString currentWord = getCurrentWord();
    QString context = getContextType();

    LOG_DEBUG(QString("AutoCompleteManager: currentWord='%1', context='%2'").arg(currentWord).arg(context));

    // Enhanced conditions to prevent inappropriate autocomplete
    if (currentWord.isEmpty()) {
        if (context != "function_param" && context != "rounding_options") {
            LOG_DEBUG("AutoCompleteManager: Current word is empty and not in function_param or rounding_options context - hiding");
            hideAutocomplete();
            return;
        }
    }

    // Don't show autocomplete if current word is too short (less than 1 character)
    // unless we're in a special context like function parameters or rounding options
    if (currentWord.length() < 1 && context != "function_param" && context != "rounding_options") {
        LOG_DEBUG("AutoCompleteManager: Current word too short and not in special context - hiding");
        hideAutocomplete();
        return;
    }

    // Don't show autocomplete on completely empty lines unless typing a function
    QTextCursor cursor = m_editor->textCursor();
    QString lineText = cursor.block().text().trimmed();
    if (lineText.isEmpty() && currentWord.isEmpty()) {
        LOG_DEBUG("AutoCompleteManager: Empty line and empty word - hiding");
        hideAutocomplete();
        return;
    }

    QStringList completions = filterCompletions(currentWord, context);
    LOG_DEBUG(QString("AutoCompleteManager: Found %1 completions").arg(completions.size()));

    if (completions.isEmpty()) {
        LOG_DEBUG("AutoCompleteManager: No completions found - hiding");
        hideAutocomplete();
        return;
    }

    QStringList descriptions = getDescriptions(completions, context);

    // Calculate position (reuse cursor from above)
    QRect cursorRect = m_editor->cursorRect(cursor);
    QPoint globalPos = m_editor->mapToGlobal(cursorRect.bottomLeft());
    globalPos.setY(globalPos.y() + 5); // Offset below cursor

    LOG_DEBUG(QString("AutoCompleteManager: Showing popup at position (%1, %2)").arg(globalPos.x()).arg(globalPos.y()));

    m_widget->showCompletions(completions, descriptions, globalPos);

    // Immediately restore focus to the editor to allow continued typing
    m_editor->setFocus();
    LOG_DEBUG("AutoCompleteManager: Restored focus to editor after showing popup");

    LOG_DEBUG(QString("AutoComplete: Showing %1 completions for word '%2' in context '%3'")
              .arg(completions.size()).arg(currentWord).arg(context));
}

void AutoCompleteManager::hideAutocomplete()
{
    if (m_widget) {
        m_widget->hideCompletions();
    }
    m_showTimer->stop();
}

bool AutoCompleteManager::isVisible() const
{
    return m_widget && m_widget->isVisible();
}

void AutoCompleteManager::handleKeyPress(QKeyEvent *event)
{
    if (!isVisible()) {
        return;
    }

    switch (event->key()) {
        case Qt::Key_Up:
            m_widget->selectPrevious();
            event->accept();
            break;
        case Qt::Key_Down:
            m_widget->selectNext();
            event->accept();
            break;
        case Qt::Key_Return:
        case Qt::Key_Enter:
        case Qt::Key_Tab:
            if (!m_widget->getCurrentSelection().isEmpty()) {
                onItemSelected(m_widget->getCurrentSelection());
                event->accept();
            }
            break;
        case Qt::Key_Escape:
            hideAutocomplete();
            event->accept();
            break;
        default:
            // Let the editor handle other keys
            break;
    }
}

void AutoCompleteManager::handleTextChanged()
{
    LOG_DEBUG("AutoCompleteManager: handleTextChanged() called");

    if (!m_editor) {
        LOG_DEBUG("AutoCompleteManager: No editor available in handleTextChanged");
        return;
    }

    // Check if we're on a comment line - disable autocomplete for comments
    QTextCursor commentCheckCursor = m_editor->textCursor();
    QString commentCheckLineText = commentCheckCursor.block().text().trimmed();
    if (commentCheckLineText.startsWith(":::")) {
        LOG_DEBUG("AutoCompleteManager: On comment line - hiding autocomplete");
        hideAutocomplete();
        return;
    }

    // Don't show autocomplete during startup delay
    if (m_startupDelay) {
        LOG_DEBUG("AutoCompleteManager: Startup delay active, ignoring text change");
        return;
    }

    // Check if popup is currently visible
    bool popupVisible = m_widget && m_widget->isVisible();
    LOG_DEBUG(QString("AutoCompleteManager: Popup currently visible: %1").arg(popupVisible));

    // Quick check to avoid unnecessary timer starts
    QString currentWord = getCurrentWord();
    QTextCursor cursor = m_editor->textCursor();
    QString lineText = cursor.block().text().trimmed();

    // Don't start timer if we're on an empty line with no current word
    if (currentWord.isEmpty() && lineText.isEmpty()) {
        LOG_DEBUG("AutoCompleteManager: Empty word and empty line - not starting timer");
        m_showTimer->stop();
        hideAutocomplete();
        return;
    }

    // Delay showing autocomplete to avoid flickering
    m_showTimer->stop();
    m_showTimer->start();

    LOG_DEBUG(QString("AutoCompleteManager: Timer started with %1ms delay").arg(SHOW_DELAY_MS));
}

void AutoCompleteManager::onItemSelected(const QString &text)
{
    insertCompletion(text);
    hideAutocomplete();
}

void AutoCompleteManager::onCancelled()
{
    hideAutocomplete();
}

QString AutoCompleteManager::getCurrentWord()
{
    if (!m_editor) {
        LOG_DEBUG("AutoCompleteManager::getCurrentWord: No editor");
        return QString();
    }

    QTextCursor cursor = m_editor->textCursor();
    QTextBlock block = cursor.block();
    QString lineText = block.text();
    int posInLine = cursor.positionInBlock();

    LOG_DEBUG(QString("AutoCompleteManager::getCurrentWord: lineText='%1', posInLine=%2").arg(lineText).arg(posInLine));

    // Find word boundaries
    int wordStart = posInLine;
    int wordEnd = posInLine;

    // Move back to find start of word
    while (wordStart > 0 && (lineText[wordStart - 1].isLetterOrNumber() || lineText[wordStart - 1] == '_')) {
        wordStart--;
    }

    // Move forward to find end of word
    while (wordEnd < lineText.length() && (lineText[wordEnd].isLetterOrNumber() || lineText[wordEnd] == '_')) {
        wordEnd++;
    }

    m_wordStartPos = block.position() + wordStart;
    QString word = lineText.mid(wordStart, wordEnd - wordStart);

    LOG_DEBUG(QString("AutoCompleteManager::getCurrentWord: word='%1', wordStart=%2, wordEnd=%3").arg(word).arg(wordStart).arg(wordEnd));

    return word;
}

QString AutoCompleteManager::getContextType()
{
    if (!m_editor) {
        return "function";
    }

    QTextCursor cursor = m_editor->textCursor();
    QString lineText = cursor.block().text();
    int posInLine = cursor.positionInBlock();
    QString textBeforeCursor = lineText.left(posInLine);

    // Check if we're inside function parentheses (including immediately after opening parenthesis)
    QRegularExpression functionPattern(R"((\w+)\(\s*([^)]*)$)");
    QRegularExpressionMatch functionMatch = functionPattern.match(textBeforeCursor);

    LOG_DEBUG(QString("AutoCompleteManager::getContextType: textBeforeCursor='%1'").arg(textBeforeCursor));

    if (functionMatch.hasMatch()) {
        QString functionName = functionMatch.captured(1).toLower();
        QString paramContent = functionMatch.captured(2);

        LOG_DEBUG(QString("AutoCompleteManager::getContextType: Regex matched! functionName='%1', paramContent='%2'").arg(functionName).arg(paramContent));

        // Check both lowercase and uppercase versions of the function name
        QString actualFunctionName = functionName;
        if (!m_functions.contains(functionName)) {
            actualFunctionName = functionName.toUpper();
        }

        if (m_functions.contains(actualFunctionName)) {
            // Count commas to determine which parameter we're on
            int commaCount = paramContent.count(',');

            LOG_DEBUG(QString("AutoCompleteManager::getContextType: Function '%1' found in m_functions as '%2', commaCount=%3").arg(functionName).arg(actualFunctionName).arg(commaCount));

            // For TC function, check if we're on the second parameter
            if ((functionName == "tc" || functionName == "TC") && commaCount >= 1) {
                m_currentContext = "function_param:" + functionName + "_param2";
                LOG_DEBUG(QString("AutoCompleteManager::getContextType: Setting context to second parameter: %1").arg(m_currentContext));
            } else {
                m_currentContext = "function_param:" + functionName;
                LOG_DEBUG(QString("AutoCompleteManager::getContextType: Setting context to first parameter: %1").arg(m_currentContext));
            }
            LOG_DEBUG("AutoCompleteManager::getContextType: Returning 'function_param'");
            return "function_param";
        } else {
            LOG_DEBUG(QString("AutoCompleteManager::getContextType: Function '%1' NOT found in m_functions (tried both '%2' and '%3')").arg(functionName).arg(functionName).arg(functionName.toUpper()));
        }
    } else {
        LOG_DEBUG("AutoCompleteManager::getContextType: Regex did not match");
    }

    // PRIORITY: Check for cross-sheet function context (S.)
    if (textBeforeCursor.endsWith("S.") || textBeforeCursor.endsWith("s.")) {
        LOG_DEBUG("AutoCompleteManager::getContextType: Cross-sheet function context (S.) - returning 'function_param'");
        m_currentContext = "function_param:S";
        return "function_param";
    }

    // PRIORITY: Check for rounding options context (after statistical function completion)
    QRegularExpression roundingPattern(R"((sum|mean|median|min|max|count|variance|stdev|product|range|geomean|harmmean|sumsq)\s*\([^)]*\)\s*$)");
    QRegularExpressionMatch roundingMatch = roundingPattern.match(textBeforeCursor);
    if (roundingMatch.hasMatch()) {
        QString functionName = roundingMatch.captured(1).toLower();
        LOG_DEBUG(QString("AutoCompleteManager::getContextType: Rounding options context for '%1' - returning 'rounding_options'").arg(functionName));
        m_currentContext = "rounding_options:" + functionName;
        return "rounding_options";
    }

    // PRIORITY: Check if we're after a number (for units/currencies) - this takes precedence
    if (isAfterNumber() || isAfterConversionTo()) {
        LOG_DEBUG("AutoCompleteManager::getContextType: After number or conversion 'to' - returning 'unit'");
        return "unit";
    }

    // Check if we're at start of line (for functions)
    // Functions should only appear at start of line, except S function which can appear anywhere
    QString currentWord = getCurrentWord();
    bool atStartOfLine = isAtStartOfLine();
    bool isSFunction = currentWord.toLower() == "s";

    LOG_DEBUG(QString("AutoCompleteManager::getContextType: currentWord='%1', atStartOfLine=%2, isSFunction=%3")
              .arg(currentWord).arg(atStartOfLine).arg(isSFunction));

    if (atStartOfLine || isSFunction) {
        LOG_DEBUG("AutoCompleteManager::getContextType: At start of line or S function - returning 'function'");
        return "function";
    }

    // Default: if we're in the middle of a line and not after a number, show units
    LOG_DEBUG("AutoCompleteManager::getContextType: Middle of line, not after number - returning 'unit'");
    return "unit";
}

bool AutoCompleteManager::isAfterNumber()
{
    if (!m_editor) {
        return false;
    }

    QTextCursor cursor = m_editor->textCursor();
    QString lineText = cursor.block().text();
    int posInLine = cursor.positionInBlock();

    // Look backwards for a number followed by whitespace and then the current word
    // Pattern: number + whitespace + current word (e.g., "100 mi" when cursor is after "mi")
    QRegularExpression numberPattern(R"(\d+\.?\d*\s+\w*$)");
    QString textBeforeCursor = lineText.left(posInLine);

    LOG_DEBUG(QString("AutoCompleteManager::isAfterNumber: textBeforeCursor='%1', pattern match: %2")
              .arg(textBeforeCursor).arg(numberPattern.match(textBeforeCursor).hasMatch()));

    return numberPattern.match(textBeforeCursor).hasMatch();
}

bool AutoCompleteManager::isAtStartOfLine()
{
    if (!m_editor) {
        return false;
    }

    QTextCursor cursor = m_editor->textCursor();
    QString lineText = cursor.block().text();

    // Check if the current word is the first non-whitespace content on the line
    // This means we're typing a function at the start of the line
    QString trimmedLine = lineText.trimmed();
    QString currentWord = getCurrentWord();

    // If the trimmed line is empty or equals the current word, we're at start of line
    bool result = trimmedLine.isEmpty() || trimmedLine == currentWord;

    LOG_DEBUG(QString("AutoCompleteManager::isAtStartOfLine: lineText='%1', currentWord='%2', result=%3")
              .arg(lineText).arg(currentWord).arg(result));

    return result;
}

bool AutoCompleteManager::isAfterConversionTo()
{
    if (!m_editor) {
        return false;
    }

    QTextCursor cursor = m_editor->textCursor();
    QString lineText = cursor.block().text();
    int posInLine = cursor.positionInBlock();

    // Check if we're after "to " in a conversion pattern like "100 mi to "
    QRegularExpression conversionPattern(R"(\d+\.?\d*\s+\w+\s+to\s+$)");
    QString textBeforeCursor = lineText.left(posInLine);

    return conversionPattern.match(textBeforeCursor).hasMatch();
}

QStringList AutoCompleteManager::filterCompletions(const QString &prefix, const QString &context)
{
    QStringList results;
    QString lowerPrefix = prefix.toLower();

    LOG_DEBUG(QString("AutoCompleteManager::filterCompletions: prefix='%1', context='%2'").arg(prefix).arg(context));

    if (context == "function") {
        LOG_DEBUG(QString("AutoCompleteManager: Filtering functions with %1 total functions").arg(m_functions.size()));
        // Filter functions
        for (auto it = m_functions.begin(); it != m_functions.end(); ++it) {
            if (it.key().toLower().contains(lowerPrefix)) {
                results << it.key();
                LOG_DEBUG(QString("AutoCompleteManager: Added function '%1'").arg(it.key()));
            }
        }
        results.sort();
    }
    else if (context == "unit") {
        LOG_DEBUG(QString("AutoCompleteManager: Filtering units/currencies with %1 units, %2 currencies").arg(m_units.size()).arg(m_currencies.size()));
        // Filter units and currencies
        for (const QString &unit : m_units) {
            if (unit.toLower().contains(lowerPrefix)) {
                results << unit;
            }
        }
        for (const QString &currency : m_currencies) {
            if (currency.toLower().contains(lowerPrefix)) {
                results << currency;
            }
        }
        results.sort();
    }
    else if (context == "function_param") {
        // Handle function parameter completion
        QString functionName = m_currentContext.split(":").last();

        LOG_DEBUG(QString("AutoCompleteManager: Function parameter completion for '%1'").arg(functionName));

        if (functionName.toLower() == "s") {
            // Cross-sheet completion - show available sheets
            results = getAvailableSheets();
        }
        else if (m_functionParameters.contains(functionName)) {
            // Show parameter templates
            results = m_functionParameters[functionName];
        }
    }
    else if (context == "rounding_options") {
        // Rounding options completion - show rounding choices
        QString functionName = m_currentContext.split(":").last();

        LOG_DEBUG(QString("AutoCompleteManager: Rounding options completion for '%1'").arg(functionName));

        // Offer rounding options
        results << "no rounding" << "rounding";
    }

    // Limit results to maximum 10 items for performance
    if (results.size() > 10) {
        results = results.mid(0, 10);
    }

    LOG_DEBUG(QString("AutoCompleteManager::filterCompletions: Returning %1 results").arg(results.size()));

    return results;
}

QStringList AutoCompleteManager::getDescriptions(const QStringList &completions, const QString &context)
{
    QStringList descriptions;

    // Handle rounding options first to avoid fallback processing
    if (context == "rounding_options") {
        QString functionName = m_currentContext.split(":").last();

        LOG_DEBUG(QString("AutoCompleteManager: Getting rounding descriptions for context='%1', completions=%2")
                  .arg(context).arg(completions.join(", ")));

        for (const QString &completion : completions) {
            QString description;
            if (completion == "no rounding") {
                description = "does not include the rounding option";
            } else if (completion == "rounding") {
                description = "inserts variable to adjust rounding of the result";
            }
            LOG_DEBUG(QString("AutoCompleteManager: Rounding description - completion='%1', description='%2'")
                      .arg(completion).arg(description));
            descriptions << description;
        }
        return descriptions;
    }

    for (const QString &completion : completions) {
        QString description;

        if (context == "function") {
            if (m_functions.contains(completion)) {
                const AutoCompleteFunction &func = m_functions[completion];
                description = func.description;
                if (!func.parameters.isEmpty()) {
                    description += QString("\nParameters: %1").arg(func.parameters.join(", "));
                }
            }
        }
        else if (context == "unit") {
            if (m_units.contains(completion)) {
                description = QString("Unit: %1").arg(completion);
            } else if (m_currencies.contains(completion)) {
                description = QString("Currency: %1").arg(completion);
            }
        }
        else if (context == "function_param") {
            QString functionName = m_currentContext.split(":").last();
            if (functionName == "s") {
                description = QString("Sheet: %1\nClick to reference this sheet").arg(completion);
            } else if (functionName == "tc_param2" || functionName == "TC_param2") {
                // Special descriptions for TC function second parameter
                if (completion == "100") {
                    description = "Convert frames to timecode";
                } else if (completion == "00:00:10:00") {
                    description = "Convert timecode to frames";
                } else if (completion.contains(" + ")) {
                    description = "Add two timecodes";
                } else if (completion.contains(" - ")) {
                    description = "Subtract two timecodes";
                } else if (completion.contains(" * ")) {
                    description = "Multiply timecode";
                } else if (completion.contains(" / ")) {
                    description = "Divide timecode";
                } else {
                    description = "Timecode expression";
                }
            } else if (functionName == "d" || functionName == "D") {
                // Special descriptions for D function date expressions
                if (completion == "July 4, 2023") {
                    description = "Format and display date\nSupported formats:\nJuly 4, 2023\n07/04/2023\n2023-07-04\n04.07.2023";
                } else if (completion.contains(" + ") && !completion.contains("W")) {
                    description = "Add calendar days to date\nSupported formats:\nJuly 4, 2023\n07/04/2023\n2023-07-04\n04.07.2023";
                } else if (completion.contains(" - ") && !completion.contains("W")) {
                    description = "Subtract calendar days or calculate days between dates\nSupported formats:\nJuly 4, 2023\n07/04/2023\n2023-07-04\n04.07.2023";
                } else if (completion.contains(" W+ ")) {
                    description = "Add business days (skip weekends)\nSupported formats:\nJuly 4, 2023\n07/04/2023\n2023-07-04\n04.07.2023";
                } else if (completion.contains(" W- ")) {
                    description = "Subtract business days or calculate business days between dates\nSupported formats:\nJuly 4, 2023\n07/04/2023\n2023-07-04\n04.07.2023";
                } else {
                    description = "Date expression\nSupported formats:\nJuly 4, 2023\n07/04/2023\n2023-07-04\n04.07.2023";
                }
            } else if (functionName == "percent") {
                // Special descriptions for percent function - check more specific patterns first
                if (completion.contains(", %, ")) {
                    description = "Calculate what percent 250 is of 1000\nAdd .2 for 2 decimal places: percent(250, %, 1000, .2)";
                } else if (completion.contains("%, ")) {
                    description = "Calculate percentage of value (25% of 1000)\nAdd .2 for 2 decimal places: percent(25%, 1000, .2)";
                } else if (completion.contains(", +, ")) {
                    description = "Increase value by percentage\nAdd .2 for 2 decimal places: percent(1000, +, 25%, .2)";
                } else if (completion.contains(", -, ")) {
                    description = "Decrease value by percentage\nAdd .2 for 2 decimal places: percent(1000, -, 25%, .2)";
                } else if (completion.contains(", to, ")) {
                    description = "Calculate percentage change between values\nAdd .2 for 2 decimal places: percent(1000, to, 1200, .2)";
                } else {
                    description = "Percentage calculation\nAdd .2 for 2 decimal places";
                }
            } else {
                // Check if this is a statistical function that supports rounding
                QStringList statFunctions = {"sum", "mean", "median", "mode", "min", "max", "count",
                                           "product", "variance", "stdev", "range", "geomean",
                                           "harmmean", "sumsq", "perc5", "perc95"};
                if (statFunctions.contains(functionName.toLower())) {
                    // Specific descriptions for statistical function parameters
                    if (completion == "above") {
                        description = QString("Use all whole numbers above current line\nAdd .2 for 2 decimal places: %1(above, .2)").arg(functionName);
                    } else if (completion == "below") {
                        description = QString("Use all whole numbers below current line\nAdd .2 for 2 decimal places: %1(below, .2)").arg(functionName);
                    } else if (completion == "1-10") {
                        description = QString("Use numbers from line 1 to line 10 (range)\nAdd .2 for 2 decimal places: %1(1-10, .2)").arg(functionName);
                    } else if (completion == "1,2,3,4,5") {
                        description = QString("Use specific line numbers (comma separated)\nAdd .2 for 2 decimal places: %1(1,2,3,4,5, .2)").arg(functionName);
                    } else {
                        description = QString("Parameter template for %1\nAdd .2 for 2 decimal places: %1(above, .2)").arg(functionName);
                    }
                } else {
                    description = QString("Parameter template for %1").arg(functionName);
                }
            }
        }

        if (description.isEmpty()) {
            description = "No description available";
        }

        descriptions << description;
    }

    return descriptions;
}

QStringList AutoCompleteManager::getAvailableSheets()
{
    QStringList sheets;

    // Get the main window to access sheet names
    QWidget *widget = m_editor;
    while (widget && !qobject_cast<class MainWindow*>(widget)) {
        widget = widget->parentWidget();
    }

    if (widget) {
        class MainWindow *mainWindow = qobject_cast<class MainWindow*>(widget);
        if (mainWindow) {
            // Get all sheet names from the tab widget
            for (int i = 0; i < mainWindow->getTabCount(); ++i) {
                QString sheetName = mainWindow->getTabName(i);
                if (!sheetName.isEmpty()) {
                    sheets << sheetName;
                }
            }
        }
    }

    return sheets;
}

void AutoCompleteManager::insertCompletion(const QString &completion)
{
    if (!m_editor) {
        return;
    }

    QString context = getContextType();

    if (context == "function_param") {
        QString functionName = m_currentContext.split(":").last();

        if (functionName.toLower() == "s") {
            // Cross-sheet completion: replace S. with S.SheetName.LN1
            QString insertion = QString("S.%1.LN1").arg(completion);

            // Special handling: replace the entire "S." prefix, not just current word
            QTextCursor cursor = m_editor->textCursor();
            QString lineText = cursor.block().text();
            int posInLine = cursor.positionInBlock();

            // Find the "S." or "s." pattern before cursor
            int dotPos = lineText.lastIndexOf('.', posInLine - 1);
            if (dotPos > 0) {
                int sPos = dotPos - 1;
                if (sPos >= 0 && (lineText[sPos] == 'S' || lineText[sPos] == 's')) {
                    // Replace from 'S' to current position
                    cursor.setPosition(cursor.block().position() + sPos);
                    cursor.setPosition(cursor.block().position() + posInLine, QTextCursor::KeepAnchor);
                    cursor.insertText(insertion);
                    m_editor->setTextCursor(cursor);
                } else {
                    replaceCurrentWord(insertion);
                }
            } else {
                replaceCurrentWord(insertion);
            }
        } else {
            // Check if this is a multi-parameter function that needs a comma instead of closing
            if (functionName == "tc" || functionName == "TC") {
                // TC function needs two parameters: framerate, timecode
                replaceCurrentWord(completion + ", ");
                // Trigger autocomplete for second parameter (timecode expressions)
                QTimer::singleShot(50, this, &AutoCompleteManager::handleTextChanged);
            } else if (functionName == "ar" || functionName == "AR") {
                // AR function can have multiple parameters
                replaceCurrentWord(completion + ", ");
                QTimer::singleShot(50, this, &AutoCompleteManager::handleTextChanged);
            } else {
                // Single parameter function - check if it's a statistical function that supports rounding
                QStringList statFunctions = {"sum", "mean", "median", "min", "max", "count",
                                           "variance", "stdev", "product", "range",
                                           "geomean", "harmmean", "sumsq"};

                LOG_DEBUG(QString("AutoCompleteManager: Function parameter completion - functionName='%1', completion='%2', isStatFunction=%3")
                          .arg(functionName).arg(completion).arg(statFunctions.contains(functionName.toLower())));

                if (statFunctions.contains(functionName.toLower())) {
                    // Statistical function - trigger rounding options autocomplete
                    replaceCurrentWord(completion + ")");
                    LOG_DEBUG(QString("AutoCompleteManager: Statistical function '%1' completed, triggering rounding options").arg(functionName));

                    // Set context for rounding options
                    m_currentContext = "rounding_options:" + functionName.toLower();
                    QTimer::singleShot(50, this, &AutoCompleteManager::handleTextChanged);
                } else {
                    // Non-statistical function - close with parenthesis
                    LOG_DEBUG(QString("AutoCompleteManager: Non-statistical function '%1' - closing with parenthesis").arg(functionName));
                    replaceCurrentWord(completion + ")");
                }
            }
        }
    }
    else if (context == "function") {
        // Function completion
        if (m_functions.contains(completion)) {
            const AutoCompleteFunction &func = m_functions[completion];
            LOG_DEBUG(QString("AutoCompleteManager: Found function '%1' with %2 parameters").arg(func.name).arg(func.parameters.size()));
            if (func.name == "S") {
                // Special handling for cross-sheet function
                replaceCurrentWord("S.");
                // Trigger another autocomplete for sheet names
                QTimer::singleShot(50, this, &AutoCompleteManager::handleTextChanged);
            } else if (!func.parameters.isEmpty()) {
                // Function with parameters - show parameter completion
                replaceCurrentWord(completion + "(");
                LOG_DEBUG(QString("AutoCompleteManager: Function '%1' has parameters, triggering autocomplete").arg(completion));
                QTimer::singleShot(50, this, &AutoCompleteManager::handleTextChanged);
            } else {
                // Simple function or constant
                replaceCurrentWord(completion);
            }
        } else {
            replaceCurrentWord(completion);
        }
    }
    else if (context == "rounding_options") {
        // Rounding options completion
        QString functionName = m_currentContext.split(":").last();

        LOG_DEBUG(QString("AutoCompleteManager: Rounding option selected: '%1' for function '%2'").arg(completion).arg(functionName));

        // For both options, we need to position cursor after the closing parenthesis
        QTextCursor cursor = m_editor->textCursor();
        QString lineText = cursor.block().text();
        int posInLine = cursor.positionInBlock();

        if (completion == "no rounding") {
            // Function is already complete - just move cursor after closing parenthesis
            int closeParenPos = lineText.indexOf(')', posInLine);
            if (closeParenPos != -1) {
                cursor.setPosition(cursor.block().position() + closeParenPos + 1);
                m_editor->setTextCursor(cursor);
                LOG_DEBUG("AutoCompleteManager: No rounding selected - cursor positioned after closing parenthesis");
            }
        } else if (completion == "rounding") {
            // Add rounding parameter: change "function(params)" to "function(params, .2)"
            // Find the closing parenthesis and insert ", .2" before it
            int closeParenPos = lineText.lastIndexOf(')', posInLine);
            if (closeParenPos != -1) {
                cursor.setPosition(cursor.block().position() + closeParenPos);
                cursor.insertText(", .2");

                // After insertion, find the new closing parenthesis position and move cursor after it
                QString updatedLineText = cursor.block().text();
                int newCloseParenPos = updatedLineText.indexOf(')', closeParenPos);
                if (newCloseParenPos != -1) {
                    cursor.setPosition(cursor.block().position() + newCloseParenPos + 1);
                    m_editor->setTextCursor(cursor);
                    LOG_DEBUG("AutoCompleteManager: Added .2 rounding parameter and positioned cursor after closing parenthesis");
                }
            }
        }
    }
    else {
        // Unit or currency completion - implement 3-step conversion workflow
        // Check context BEFORE making any changes to cursor position
        bool afterConversionTo = isAfterConversionTo();
        bool afterNumber = isAfterNumber();

        // Get current line text for debugging
        QTextCursor cursor = m_editor->textCursor();
        QString lineText = cursor.block().text();
        int posInLine = cursor.positionInBlock();
        QString textBeforeCursor = lineText.left(posInLine);

        LOG_DEBUG(QString("AutoCompleteManager: Unit/currency completion - afterConversionTo: %1, afterNumber: %2, completion: '%3'")
                  .arg(afterConversionTo).arg(afterNumber).arg(completion));
        LOG_DEBUG(QString("AutoCompleteManager: Context - lineText: '%1', posInLine: %2, textBeforeCursor: '%3'")
                  .arg(lineText).arg(posInLine).arg(textBeforeCursor));

        if (afterConversionTo) {
            // Step 3: Complete the conversion (e.g., "100 mi to km")
            LOG_DEBUG("AutoCompleteManager: Step 3 - Completing conversion");
            replaceCurrentWord(completion);
        } else if (afterNumber) {
            // Step 2: Add " to " and trigger autocomplete for target unit
            LOG_DEBUG("AutoCompleteManager: Step 2 - Adding 'to' after unit and triggering next autocomplete");
            replaceCurrentWord(completion + " to ");
            QTimer::singleShot(50, this, &AutoCompleteManager::handleTextChanged);
        } else {
            // Default: just insert the unit/currency
            LOG_DEBUG("AutoCompleteManager: Default - Just inserting unit/currency");
            replaceCurrentWord(completion);
        }
    }
}

void AutoCompleteManager::replaceCurrentWord(const QString &replacement)
{
    if (!m_editor) {
        return;
    }

    QTextCursor cursor = m_editor->textCursor();

    // Select the current word
    cursor.setPosition(m_wordStartPos);
    cursor.setPosition(cursor.position() + getCurrentWord().length(), QTextCursor::KeepAnchor);

    // Replace with completion
    cursor.insertText(replacement);

    // Update cursor position
    m_editor->setTextCursor(cursor);
}
