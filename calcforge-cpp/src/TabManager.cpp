#include "TabManager.h"
#include "WorksheetWidget.h"
#include "ExpressionEditor.h"
#include "ResultsDisplay.h"
#include "Logger.h"

#include <QInputDialog>
#include <QMessageBox>
#include <QPushButton>
#include <QHBoxLayout>
#include <QTabBar>

TabManager::TabManager(QTabWidget* tabWidget, QSettings* settings, QObject* parent)
    : QObject(parent)
    , m_tabWidget(tabWidget)
    , m_settings(settings)
    , m_currentFontSize(16)  // Increased from 14 to 16 (2 increments larger)
    , m_nextTabNumber(1)
{
    if (!m_tabWidget || !m_settings) {
        LOG_DEBUG("TabManager: Invalid dependencies injected");
        return;
    }
    
    // Connect tab widget signals
    connect(m_tabWidget, QOverload<int>::of(&QTabWidget::currentChanged),
            this, &TabManager::onTabChanged);
    
    // Load font size from settings
    m_settings->beginGroup("UI");
    m_currentFontSize = m_settings->value("fontSize", 16).toInt();  // Changed default from 14 to 16
    m_settings->endGroup();
    
    LOG_DEBUG("TabManager: Initialized with dependency injection");
}

CalcForgeResult<int> TabManager::addTab(const QString& name)
{
    QString tabName = name.isEmpty() ? generateUniqueTabName() : name;
    
    // Create new worksheet widget
    WorksheetWidget* worksheet = createWorksheetWidget();
    if (!worksheet) {
        return CalcForgeResult<int>::error("Failed to create worksheet widget");
    }
    
    // Setup the worksheet
    setupWorksheetWidget(worksheet, tabName);
    
    // Add to tab widget - try rich text format to bypass mnemonics
    QString richTextName = QString("<span>%1</span>").arg(tabName.toHtmlEscaped());
    int index = m_tabWidget->addTab(worksheet, richTextName);
    
    // Store original name in map for accurate retrieval
    m_originalTabNames[index] = tabName;
    
    // Setup custom close button for this tab
    setupCustomCloseButton(index);
    
    // Set as current tab
    m_tabWidget->setCurrentIndex(index);
    
    emit tabAdded(index, tabName);
    LOG_DEBUG("TabManager: Added tab '" + tabName + "' at index " + QString::number(index));
    
    return CalcForgeResult<int>::success(index);
}

CalcForgeResult<bool> TabManager::closeTab(int index)
{
    if (!isValidTabIndex(index)) {
        return CalcForgeResult<bool>::error("Invalid tab index: " + QString::number(index));
    }
    
    QString tabName = m_tabWidget->tabText(index);
    
    // Don't close the last tab
    if (m_tabWidget->count() <= 1) {
        LOG_DEBUG("TabManager: Cannot close last remaining tab");
        return CalcForgeResult<bool>::success(false);
    }
    
    // Get the widget before removing
    WorksheetWidget* worksheet = getWorksheetWidget(index);
    
    // Remove the tab
    m_tabWidget->removeTab(index);
    
    // Clean up original name mapping
    m_originalTabNames.remove(index);
    // Shift indices down for remaining tabs
    QMap<int, QString> newMapping;
    for (auto it = m_originalTabNames.begin(); it != m_originalTabNames.end(); ++it) {
        int oldIndex = it.key();
        if (oldIndex > index) {
            newMapping[oldIndex - 1] = it.value();
        } else {
            newMapping[oldIndex] = it.value();
        }
    }
    m_originalTabNames = newMapping;
    
    // Delete the widget
    if (worksheet) {
        worksheet->deleteLater();
    }
    
    emit tabClosed(index, tabName);
    LOG_DEBUG("TabManager: Closed tab '" + tabName + "' at index " + QString::number(index));
    
    return CalcForgeResult<bool>::success(true);
}

CalcForgeResult<bool> TabManager::renameTab(int index, const QString& newName)
{
    if (!isValidTabIndex(index)) {
        return CalcForgeResult<bool>::error("Invalid tab index: " + QString::number(index));
    }
    
    QString currentName = getTabName(index);
    QString finalName = newName;
    
    if (finalName.isEmpty()) {
        // Show input dialog
        bool ok;
        finalName = QInputDialog::getText(
            nullptr,
            "Rename Tab",
            "Enter new tab name:",
            QLineEdit::Normal,
            currentName,  // Use original name in dialog
            &ok
        );
        
        if (!ok || finalName.isEmpty()) {
            return CalcForgeResult<bool>::success(false); // User cancelled
        }
    }
    
    // Check for duplicate names
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        if (i != index && m_tabWidget->tabText(i) == finalName) {
            return CalcForgeResult<bool>::error("Tab name '" + finalName + "' already exists");
        }
    }
    
    // Apply the rename - try rich text format to bypass mnemonics
    QString richTextName = QString("<span>%1</span>").arg(finalName.toHtmlEscaped());
    m_tabWidget->setTabText(index, richTextName);
    
    // Store original name in map for accurate retrieval
    m_originalTabNames[index] = finalName;
    
    LOG_DEBUG(QString("TabManager: Renamed tab - Original: '%1', RichText: '%2'").arg(finalName).arg(richTextName));
    
    emit tabRenamed(index, currentName, finalName);
    LOG_DEBUG("TabManager: Renamed tab from '" + currentName + "' to '" + finalName + "'");
    
    return CalcForgeResult<bool>::success(true);
}

void TabManager::navigateToTab(int index)
{
    if (isValidTabIndex(index)) {
        m_tabWidget->setCurrentIndex(index);
        LOG_DEBUG("TabManager: Navigated to tab index " + QString::number(index));
    }
}

void TabManager::previousTab()
{
    int currentIndex = m_tabWidget->currentIndex();
    int newIndex = (currentIndex - 1 + m_tabWidget->count()) % m_tabWidget->count();
    navigateToTab(newIndex);
}

void TabManager::nextTab()
{
    int currentIndex = m_tabWidget->currentIndex();
    int newIndex = (currentIndex + 1) % m_tabWidget->count();
    navigateToTab(newIndex);
}

int TabManager::getTabCount() const
{
    return m_tabWidget->count();
}

QString TabManager::getTabName(int index) const
{
    if (isValidTabIndex(index)) {
        // Return original name from map if available
        if (m_originalTabNames.contains(index)) {
            return m_originalTabNames[index];
        }
        
        // Fallback: extract text from rich text or handle escaped text
        QString displayText = m_tabWidget->tabText(index);
        
        // If it's rich text, try to extract plain text
        if (displayText.contains("<span>") && displayText.contains("</span>")) {
            // Extract text between <span> tags
            int start = displayText.indexOf("<span>") + 6;
            int end = displayText.indexOf("</span>");
            if (start > 5 && end > start) {
                QString extracted = displayText.mid(start, end - start);
                // Convert HTML entities back to plain text
                extracted.replace("&amp;", "&");
                extracted.replace("&lt;", "<");
                extracted.replace("&gt;", ">");
                return extracted;
            }
        }
        
        // Fallback: un-escape display text for legacy tabs
        QString unescaped = displayText;
        unescaped.replace("&&", "&");
        return unescaped;
    }
    return QString();
}

int TabManager::getCurrentTabIndex() const
{
    return m_tabWidget->currentIndex();
}

QString TabManager::getCurrentTabName() const
{
    return getTabName(getCurrentTabIndex());
}

WorksheetWidget* TabManager::getWorksheetWidget(int index) const
{
    if (!isValidTabIndex(index)) {
        return nullptr;
    }
    
    return qobject_cast<WorksheetWidget*>(m_tabWidget->widget(index));
}

WorksheetWidget* TabManager::getCurrentWorksheetWidget() const
{
    return getWorksheetWidget(getCurrentTabIndex());
}

WorksheetWidget* TabManager::getWorksheetByName(const QString& name) const
{
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        if (m_tabWidget->tabText(i).compare(name, Qt::CaseInsensitive) == 0) {
            return getWorksheetWidget(i);
        }
    }
    return nullptr;
}

void TabManager::increaseFontSize()
{
    if (m_currentFontSize < 32) { // Maximum pixel size
        m_currentFontSize++;
        applyGlobalFontSize(m_currentFontSize);
    }
}

void TabManager::decreaseFontSize()
{
    if (m_currentFontSize > 8) { // Minimum pixel size
        m_currentFontSize--;
        applyGlobalFontSize(m_currentFontSize);
    }
}

void TabManager::resetFontSize()
{
    m_currentFontSize = 16; // Updated default font size to match new default
    applyGlobalFontSize(m_currentFontSize);
}

void TabManager::applyGlobalFontSize(int fontSize)
{
    m_currentFontSize = fontSize;
    
    // Apply to all worksheet widgets
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        WorksheetWidget* worksheet = getWorksheetWidget(i);
        if (worksheet) {
            // Apply font size to editor and results displays
            QFont editorFont = worksheet->getEditor()->font();
            QFont resultsFont = worksheet->getResults()->font();
            
            editorFont.setPixelSize(fontSize);
            resultsFont.setPixelSize(fontSize);
            
            worksheet->getEditor()->setFont(editorFont);
            worksheet->getResults()->setFont(resultsFont);
        }
    }
    
    // Save to settings
    m_settings->beginGroup("UI");
    m_settings->setValue("fontSize", fontSize);
    m_settings->endGroup();
    m_settings->sync();
    
    LOG_DEBUG("TabManager: Applied global font size " + QString::number(fontSize) + " to all tabs");
}

void TabManager::onTabChanged(int index)
{
    if (isValidTabIndex(index)) {
        emit currentTabChanged(index);
        LOG_DEBUG("TabManager: Current tab changed to index " + QString::number(index));
    }
}

void TabManager::onSplitterMoved(const QByteArray& newState)
{
    emit splitterMoved(newState);
}

WorksheetWidget* TabManager::createWorksheetWidget()
{
    try {
        WorksheetWidget* worksheet = new WorksheetWidget();
        return worksheet;
    } catch (const std::exception& e) {
        LOG_DEBUG("TabManager: Failed to create WorksheetWidget: " + QString(e.what()));
        return nullptr;
    }
}

void TabManager::setupWorksheetWidget(WorksheetWidget* worksheet, const QString& name)
{
    if (!worksheet) return;
    
    // Apply current font size
    QFont editorFont = worksheet->getEditor()->font();
    QFont resultsFont = worksheet->getResults()->font();
    
    editorFont.setPixelSize(m_currentFontSize);
    resultsFont.setPixelSize(m_currentFontSize);
    
    worksheet->getEditor()->setFont(editorFont);
    worksheet->getResults()->setFont(resultsFont);
    
    // Connect signals
    connectWorksheetSignals(worksheet);
    
    // Emit signal for MainWindow to setup cross-sheet support
    emit worksheetNeedsSetup(worksheet, name);
    
    LOG_DEBUG("TabManager: Setup worksheet widget for tab '" + name + "'");
}

void TabManager::connectWorksheetSignals(WorksheetWidget* worksheet)
{
    if (!worksheet) return;
    
    // Connect splitter moved signal
    connect(worksheet, &WorksheetWidget::splitterMoved,
            this, &TabManager::onSplitterMoved);
    
    // Add other worksheet signal connections as needed
    LOG_DEBUG("TabManager: Connected worksheet signals");
}

bool TabManager::isValidTabIndex(int index) const
{
    return index >= 0 && index < m_tabWidget->count();
}

QString TabManager::generateUniqueTabName(const QString& baseName) const
{
    QString name;
    bool nameExists;
    
    do {
        name = baseName + " " + QString::number(m_nextTabNumber);
        nameExists = false;
        
        // Check if name already exists
        for (int i = 0; i < m_tabWidget->count(); ++i) {
            if (m_tabWidget->tabText(i) == name) {
                nameExists = true;
                break;
            }
        }
        
        if (nameExists) {
            const_cast<TabManager*>(this)->m_nextTabNumber++;
        }
        
    } while (nameExists);
    
    const_cast<TabManager*>(this)->m_nextTabNumber++;
    return name;
}

void TabManager::setupCustomCloseButton(int index)
{
    QTabBar *tabBar = m_tabWidget->tabBar();
    if (!tabBar || !isValidTabIndex(index)) {
        return;
    }
    
    // Create custom close button container (reduced width to bring X closer to tab name)
    QWidget *buttonContainer = new QWidget();
    buttonContainer->setFixedSize(21, 20);  // Reduced from 26 to 21 (5 pixel reduction)

    QHBoxLayout *buttonLayout = new QHBoxLayout(buttonContainer);
    buttonLayout->setContentsMargins(0, 0, 8, 0);  // Restored original right margin
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

    // Connect close button to close this specific tab
    connect(closeButton, &QPushButton::clicked, [this, index]() {
        closeTab(index);
    });

    buttonLayout->addWidget(closeButton);
    tabBar->setTabButton(index, QTabBar::RightSide, buttonContainer);
    
    LOG_DEBUG("TabManager: Setup custom close button for tab index " + QString::number(index));
}

void TabManager::fixExistingTabNames()
{
    // Fix existing tabs that were created before the & escaping system
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        if (!m_originalTabNames.contains(i)) {
            // This tab wasn't created with the new system
            QString currentText = m_tabWidget->tabText(i);
            
            // Convert to rich text format to handle & characters properly
            QString richTextName = QString("<span>%1</span>").arg(currentText.toHtmlEscaped());
            m_tabWidget->setTabText(i, richTextName);
            m_originalTabNames[i] = currentText; // Store original
            LOG_DEBUG("TabManager: Fixed tab " + QString::number(i) + " with rich text: " + currentText);
        }
    }
}