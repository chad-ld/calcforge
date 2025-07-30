#include "FileManager.h"
#include "WorksheetWidget.h"
#include "ExpressionEditor.h"
#include "TabManager.h"
#include "MainWindow.h"
#include "EventBus.h"
#include "Logger.h"

#include <QFileDialog>
#include <QDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMessageBox>
#include <QStandardPaths>
#include <QDir>
#include <QCoreApplication>
#include <QTimer>

FileManager::FileManager(QTabWidget* tabWidget, TabManager* tabManager, QSettings* settings, EventBus* eventBus, QObject* parent)
    : QObject(parent)
    , m_tabWidget(tabWidget)
    , m_tabManager(tabManager)
    , m_settings(settings)
    , m_eventBus(eventBus)
    , m_isModified(false)
    , m_isLoading(false)
{
    if (!m_tabWidget || !m_tabManager || !m_settings || !m_eventBus) {
        LOG_DEBUG("FileManager: Invalid dependencies injected");
        return;
    }
    
    // Load recent files from settings
    loadRecentFiles();

    // Connect to TabManager signals to track tab changes
    if (m_tabManager) {
        connect(m_tabManager, &TabManager::tabClosed, this, [this](int index, const QString& tabName) {
            LOG_DEBUG(QString("FileManager: Tab closed signal received - index: %1, name: %2").arg(index).arg(tabName));
            // Only mark as modified if we're not in the loading phase
            if (!m_isLoading) {
                // Closing a tab is a workspace change that should be saved
                markAsModified();
                LOG_DEBUG("FileManager: Marked as modified due to tab closure (workspace change)");
            } else {
                LOG_DEBUG("FileManager: Skipping modification marking during loading phase");
            }
        });

        connect(m_tabManager, &TabManager::tabAdded, this, [this](int index, const QString& tabName) {
            LOG_DEBUG(QString("FileManager: Tab added signal received - index: %1, name: %2").arg(index).arg(tabName));
            // Only mark as modified if we're not in the loading phase
            if (!m_isLoading) {
                // Adding a tab is a workspace change that should be saved
                markAsModified();
                LOG_DEBUG("FileManager: Marked as modified due to tab addition (workspace change)");
            } else {
                LOG_DEBUG("FileManager: Skipping modification marking during loading phase");
            }
        });

        connect(m_tabManager, &TabManager::tabRenamed, this, [this](int index, const QString& oldName, const QString& newName) {
            LOG_DEBUG(QString("FileManager: Tab renamed signal received - index: %1, old: %2, new: %3").arg(index).arg(oldName).arg(newName));
            // Only mark as modified if we're not in the loading phase
            if (!m_isLoading) {
                // Renaming a tab is a workspace change that should be saved
                markAsModified();
                LOG_DEBUG("FileManager: Marked as modified due to tab rename (workspace change)");
            } else {
                LOG_DEBUG("FileManager: Skipping modification marking during loading phase");
            }
        });

        LOG_DEBUG("FileManager: Connected to TabManager signals (tabClosed, tabAdded, tabRenamed)");
    } else {
        LOG_DEBUG("FileManager: WARNING - m_tabManager is null, cannot connect to tabClosed signal");
    }

    LOG_DEBUG("FileManager: Initialized with dependency injection");
}

CalcForgeResult<bool> FileManager::loadWorksheetFile(const QString& filePath)
{
    QString path = filePath;
    if (path.isEmpty()) {
        // Show file dialog
        QString defaultDir = m_currentFile.isEmpty() ? 
            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) :
            QDir(m_currentFile).absolutePath();
            
        QWidget* parentWidget = qobject_cast<QWidget*>(parent());

        // Create file dialog with proper flags to appear above always-on-top parent
        QFileDialog dialog(parentWidget);
        dialog.setWindowTitle("Load CalcForge Worksheet");
        dialog.setDirectory(defaultDir);
        dialog.setNameFilter("CalcForge Files (*.json *.cf);;All Files (*)");
        dialog.setFileMode(QFileDialog::ExistingFile);
        dialog.setAcceptMode(QFileDialog::AcceptOpen);

        // Set window flags to ensure dialog appears above always-on-top parent
        dialog.setWindowFlags(Qt::Dialog | Qt::WindowStaysOnTopHint | Qt::WindowCloseButtonHint);

        if (dialog.exec() == QDialog::Accepted) {
            QStringList files = dialog.selectedFiles();
            if (!files.isEmpty()) {
                path = files.first();
            }
        }
        
        if (path.isEmpty()) {
            return CalcForgeResult<bool>::success(false); // User cancelled
        }
    }
    
    if (loadWorksheetContentFromFile(path)) {
        m_currentFile = path;
        markAsSaved();
        addToRecentFiles(path);
        
        if (m_eventBus) {
            m_eventBus->applicationEvents()->emitCurrentFileChanged(m_currentFile);
        }
        LOG_DEBUG("FileManager: Successfully loaded worksheet: " + path);
        return CalcForgeResult<bool>::success(true);
    }
    
    return CalcForgeResult<bool>::error("Failed to load worksheet file: " + path);
}

CalcForgeResult<bool> FileManager::saveWorksheetFile(const QString& filePath)
{
    QString path = filePath.isEmpty() ? m_currentFile : filePath;
    
    if (path.isEmpty()) {
        // No current file, use Save As
        return saveWorksheetFileAs();
    }
    
    if (saveWorksheetsToFile(path)) {
        m_currentFile = path;
        markAsSaved();
        addToRecentFiles(path);
        
        if (m_eventBus) {
            m_eventBus->applicationEvents()->emitCurrentFileChanged(m_currentFile);
        }
        LOG_DEBUG("FileManager: Successfully saved worksheet: " + path);
        
        // Show success confirmation
        QMessageBox::information(
            qobject_cast<QWidget*>(parent()),
            "Save Successful",
            "Worksheet saved successfully to:\n" + path
        );
        
        return CalcForgeResult<bool>::success(true);
    }
    
    return CalcForgeResult<bool>::error("Failed to save worksheet file: " + path);
}

CalcForgeResult<bool> FileManager::saveWorksheetFileAs()
{
    QString defaultDir = m_currentFile.isEmpty() ? 
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) :
        QDir(m_currentFile).absolutePath();
        
    QWidget* parentWidget = qobject_cast<QWidget*>(parent());

    // Create save dialog with proper flags to appear above always-on-top parent
    QFileDialog dialog(parentWidget);
    dialog.setWindowTitle("Save CalcForge Worksheet As");
    dialog.setDirectory(defaultDir);
    dialog.setNameFilter("CalcForge Files (*.json *.cf);;All Files (*)");
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setAcceptMode(QFileDialog::AcceptSave);

    // Set window flags to ensure dialog appears above always-on-top parent
    dialog.setWindowFlags(Qt::Dialog | Qt::WindowStaysOnTopHint | Qt::WindowCloseButtonHint);

    QString path;
    if (dialog.exec() == QDialog::Accepted) {
        QStringList files = dialog.selectedFiles();
        if (!files.isEmpty()) {
            path = files.first();
        }
    }
    
    if (path.isEmpty()) {
        return CalcForgeResult<bool>::success(false); // User cancelled
    }
    
    return saveWorksheetFile(path);
}

void FileManager::markAsModified()
{
    if (!m_isModified) {
        m_isModified = true;
        if (m_eventBus) {
            m_eventBus->applicationEvents()->emitFileStateChanged(true);
        }
        LOG_DEBUG("FileManager: Marked as modified");
    }
}

void FileManager::markAsSaved()
{
    if (m_isModified) {
        m_isModified = false;
        if (m_eventBus) {
            m_eventBus->applicationEvents()->emitFileStateChanged(false);
        }
        LOG_DEBUG("FileManager: Marked as saved");
    }

    // Also mark all worksheets as saved
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        WorksheetWidget* worksheet = getWorksheetWidget(i);
        if (worksheet && worksheet->isModified()) {
            worksheet->setModified(false);
            LOG_DEBUG(QString("FileManager: Marked worksheet %1 as saved").arg(i));
        }
    }
}

bool FileManager::hasUnsavedChanges() const
{
    LOG_DEBUG(QString("FileManager::hasUnsavedChanges() - Global m_isModified: %1, Tab count: %2").arg(m_isModified).arg(m_tabWidget->count()));

    // Check global modified state
    if (m_isModified) {
        LOG_DEBUG("FileManager::hasUnsavedChanges() - Returning true due to global m_isModified flag");
        return true;
    }

    // Check if any individual worksheet has unsaved changes
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        WorksheetWidget* worksheet = getWorksheetWidget(i);
        if (worksheet) {
            bool isModified = worksheet->isModified();
            LOG_DEBUG(QString("FileManager::hasUnsavedChanges() - Tab %1 (%2): isModified = %3")
                     .arg(i)
                     .arg(m_tabWidget->tabText(i))
                     .arg(isModified));
            if (isModified) {
                LOG_DEBUG(QString("FileManager::hasUnsavedChanges() - Tab %1 has unsaved changes").arg(i));
                return true;
            }
        }
    }

    LOG_DEBUG("FileManager::hasUnsavedChanges() - No unsaved changes found, returning false");
    return false;
}

void FileManager::addToRecentFiles(const QString& filePath)
{
    if (filePath.isEmpty()) return;
    
    // Remove if already exists
    m_recentFiles.removeAll(filePath);
    
    // Add to front
    m_recentFiles.prepend(filePath);
    
    // Limit to MAX_RECENT_FILES
    while (m_recentFiles.size() > MAX_RECENT_FILES) {
        m_recentFiles.removeLast();
    }
    
    saveRecentFiles();
    if (m_eventBus) {
        m_eventBus->applicationEvents()->emitRecentFilesChanged(m_recentFiles);
    }
    LOG_DEBUG("FileManager: Added to recent files: " + filePath);
}

void FileManager::loadRecentFile(const QString& filePath)
{
    auto result = loadWorksheetFile(filePath);
    if (!result.isValid()) {
        // Remove invalid file from recent files
        m_recentFiles.removeAll(filePath);
        saveRecentFiles();
        if (m_eventBus) {
            m_eventBus->applicationEvents()->emitRecentFilesChanged(m_recentFiles);
        }
        
        QWidget* parentWidget = qobject_cast<QWidget*>(parent());

        // Create message box with proper flags to appear above always-on-top parent
        QMessageBox msgBox(parentWidget);
        msgBox.setWindowTitle("Load Error");
        msgBox.setText(result.errorMessage());
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setStandardButtons(QMessageBox::Ok);

        // Set window flags to ensure dialog appears above always-on-top parent
        msgBox.setWindowFlags(Qt::Dialog | Qt::WindowStaysOnTopHint | Qt::WindowCloseButtonHint);

        msgBox.exec();
    }
}

void FileManager::loadWorksheets()
{
    QString appDir = QCoreApplication::applicationDirPath();
    QString worksheetsPath = appDir + "/worksheets.json";
    
    QFile file(worksheetsPath);
    if (file.exists()) {
        LOG_DEBUG("FileManager: Loading worksheets from: " + worksheetsPath);
        if (loadWorksheetContentFromFile(worksheetsPath)) {
            m_currentFile = worksheetsPath;
            markAsSaved();
            return;
        }
    }
    
    // Fall back to example worksheets
    LOG_DEBUG("FileManager: worksheets.json not found, loading examples");
    loadExampleWorksheets();
}

void FileManager::loadExampleWorksheets()
{
    QString appDir = QCoreApplication::applicationDirPath();
    QString examplePath = appDir + "/example_worksheets.json";
    
    QFile file(examplePath);
    if (file.exists()) {
        LOG_DEBUG("FileManager: Loading example worksheets from: " + examplePath);
        if (loadWorksheetContentFromFile(examplePath)) {
            // Save as worksheets.json for future use
            loadExampleWorksheetsAndSave();
        }
    } else {
        LOG_DEBUG("FileManager: No example worksheets found, ensuring default tab exists");
        // Ensure we have at least one tab if no files exist
        if (m_tabManager->getTabCount() == 0) {
            auto result = m_tabManager->addTab("Sheet 1");
            if (result.isValid()) {
                LOG_DEBUG("FileManager: Created default tab since no worksheets found");
            }
        }
    }
}

void FileManager::onWorksheetModified()
{
    markAsModified();
}

bool FileManager::loadWorksheetContentFromFile(const QString& filePath)
{
    // Set loading flag to prevent marking workspace as modified during load
    m_isLoading = true;
    LOG_DEBUG("FileManager: Started loading worksheets, set m_isLoading = true");

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        LOG_DEBUG("FileManager: Cannot open file: " + filePath);
        m_isLoading = false;
        return false;
    }
    
    QByteArray data = file.readAll();
    file.close();

    // Fast JSON parsing without detailed error reporting for startup speed
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    
    if (parseError.error != QJsonParseError::NoError) {
        LOG_DEBUG("FileManager: JSON parse error: " + parseError.errorString());
        m_isLoading = false;
        return false;
    }
    
    QJsonObject root = doc.object();
    
    // Clear existing tabs using TabManager (but leave at least one)
    while (m_tabManager->getTabCount() > 1) {
        auto result = m_tabManager->closeTab(0);
        if (!result.isValid() || !result.value()) {
            // Failed to close tab, break to avoid infinite loop
            LOG_DEBUG("FileManager: Failed to close tab, stopping cleanup");
            break;
        }
    }
    
    // Load worksheets - support both "worksheets" and "tabs" for backwards compatibility
    QJsonArray worksheets;
    if (root.contains("worksheets") && root["worksheets"].isArray()) {
        worksheets = root["worksheets"].toArray();
    } else if (root.contains("tabs") && root["tabs"].isArray()) {
        worksheets = root["tabs"].toArray();
    }
    
    // Load all worksheets as new tabs (no initial tab to reuse)
    for (const QJsonValue& value : worksheets) {
        if (value.isObject()) {
            QJsonObject worksheet = value.toObject();
            QString name = worksheet["name"].toString();
            QString content = worksheet["content"].toString();

            if (!name.isEmpty()) {
                // Always create new tabs to avoid flicker from tab manipulation
                loadSingleWorksheet(name, content);
            }
        }
    }

    LOG_DEBUG(QString("FileManager: Loaded %1 worksheets from file").arg(m_tabManager->getTabCount()));

    // Ensure we have at least one tab
    if (m_tabManager->getTabCount() == 0) {
        LOG_DEBUG("FileManager: No worksheets loaded, creating default tab");
        auto result = m_tabManager->addTab("Sheet 1");
        if (result.isValid()) {
            LOG_DEBUG("FileManager: Created default tab");
        }
    }

    // Trigger cross-sheet recalculation after all worksheets are loaded
    // This ensures cross-sheet references work correctly on startup
    if (m_tabManager->getTabCount() > 0) {
        // Get the MainWindow to trigger recalculation
        QWidget* parent = qobject_cast<QWidget*>(this->parent());
        while (parent && !qobject_cast<class MainWindow*>(parent)) {
            parent = qobject_cast<QWidget*>(parent->parent());
        }

        if (parent) {
            class MainWindow* mainWindow = qobject_cast<class MainWindow*>(parent);
            if (mainWindow) {
                mainWindow->triggerCrossSheetRecalculation();
                LOG_DEBUG("FileManager: Triggered cross-sheet recalculation after loading worksheets");
            }
        }

        // Ensure first tab is focused after loading
        QTimer::singleShot(50, [this]() {
            if (m_tabWidget && m_tabWidget->count() > 0) {
                m_tabWidget->setCurrentIndex(0);
                WorksheetWidget* firstWorksheet = qobject_cast<WorksheetWidget*>(m_tabWidget->widget(0));
                if (firstWorksheet) {
                    firstWorksheet->getEditor()->setFocus();
                    LOG_DEBUG("FileManager: Set focus to first tab after loading");
                }
            }
        });
    }

    // Clear loading flag - workspace changes after this point should be tracked
    m_isLoading = false;
    LOG_DEBUG("FileManager: Finished loading worksheets, set m_isLoading = false");

    return true;
}

bool FileManager::saveWorksheetsToFile(const QString& filePath)
{
    QJsonObject root;
    root["version"] = "2.0";
    root["app"] = "CalcForge C++";
    
    QJsonArray worksheets;
    
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        WorksheetWidget* worksheet = getWorksheetWidget(i);
        if (worksheet) {
            QJsonObject worksheetObj;
            // Use TabManager to get the unescaped tab name
            worksheetObj["name"] = m_tabManager->getTabName(i);
            worksheetObj["content"] = worksheet->getContent();
            worksheets.append(worksheetObj);
        }
    }
    
    root["worksheets"] = worksheets;
    
    QJsonDocument doc(root);
    
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        LOG_DEBUG("FileManager: Cannot write to file: " + filePath);
        return false;
    }
    
    file.write(doc.toJson());
    file.close();
    
    LOG_DEBUG(QString("FileManager: Saved %1 worksheets to file").arg(m_tabWidget->count()));
    return true;
}

void FileManager::loadSingleWorksheet(const QString& tabName, const QString& content)
{
    // Use TabManager to create the tab properly
    auto result = m_tabManager->addTab(tabName);
    if (result.isValid()) {
        int tabIndex = result.value();
        WorksheetWidget* worksheet = m_tabManager->getWorksheetWidget(tabIndex);
        if (worksheet) {
            worksheet->setContent(content);
            
            // Connect signals for modification tracking
            connectWorksheetSignals(worksheet);
            
            LOG_DEBUG("FileManager: Loaded worksheet tab: " + tabName);
        }
    } else {
        LOG_DEBUG("FileManager: Failed to create tab for: " + tabName + " - " + result.errorMessage());
    }
}

void FileManager::loadIntoExistingTab(int tabIndex, const QString& tabName, const QString& content)
{
    // Use existing tab to avoid duplication
    WorksheetWidget* worksheet = m_tabManager->getWorksheetWidget(tabIndex);
    if (worksheet) {
        // Rename the tab and set content
        auto renameResult = m_tabManager->renameTab(tabIndex, tabName);
        if (renameResult.isValid() && renameResult.value()) {
            worksheet->setContent(content);
            
            // Connect signals for modification tracking
            connectWorksheetSignals(worksheet);
            
            LOG_DEBUG("FileManager: Loaded into existing tab: " + tabName);
        } else {
            LOG_DEBUG("FileManager: Failed to rename existing tab to: " + tabName);
        }
    } else {
        LOG_DEBUG("FileManager: Failed to get existing worksheet widget at index: " + QString::number(tabIndex));
    }
}

void FileManager::loadExampleWorksheetsAndSave()
{
    QString appDir = QCoreApplication::applicationDirPath();
    QString worksheetsPath = appDir + "/worksheets.json";
    
    if (saveWorksheetsToFile(worksheetsPath)) {
        m_currentFile = worksheetsPath;
        markAsSaved();
        LOG_DEBUG("FileManager: Saved example worksheets as worksheets.json");
    }
}

void FileManager::loadRecentFiles()
{
    m_settings->beginGroup("RecentFiles");
    m_recentFiles = m_settings->value("files", QStringList()).toStringList();
    m_settings->endGroup();
    
    // Remove non-existent files
    QStringList validFiles;
    for (const QString& file : m_recentFiles) {
        if (QFile::exists(file)) {
            validFiles.append(file);
        }
    }
    
    if (validFiles.size() != m_recentFiles.size()) {
        m_recentFiles = validFiles;
        saveRecentFiles();
    }
    
    LOG_DEBUG(QString("FileManager: Loaded %1 recent files").arg(m_recentFiles.size()));
}

void FileManager::saveRecentFiles()
{
    m_settings->beginGroup("RecentFiles");
    m_settings->setValue("files", m_recentFiles);
    m_settings->endGroup();
    m_settings->sync();
}

void FileManager::createInitialRecentFilesJson()
{
    // This method was in the original but seems unused
    // Keeping for compatibility but implementing as no-op
    LOG_DEBUG("FileManager: createInitialRecentFilesJson called (no-op)");
}

WorksheetWidget* FileManager::getWorksheetWidget(int index) const
{
    if (index < 0 || index >= m_tabWidget->count()) {
        return nullptr;
    }
    
    return qobject_cast<WorksheetWidget*>(m_tabWidget->widget(index));
}

void FileManager::connectWorksheetSignals(WorksheetWidget* worksheet)
{
    if (worksheet) {
        // Connect modification signals
        connect(worksheet, &WorksheetWidget::contentChanged, 
                this, &FileManager::onWorksheetModified);
        // Add other worksheet signals as needed
    }
} 
