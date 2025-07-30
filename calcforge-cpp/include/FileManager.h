#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include <QObject>
#include <QSettings>
#include <QString>
#include <QStringList>
#include <QTabWidget>
#include "CalcForgeResult.h"

class WorksheetWidget;
class TabManager;
class MainWindow;
class EventBus;

/**
 * FileManager handles all file I/O operations for CalcForge
 * Including load/save worksheets, recent files management, and file state tracking
 * 
 * This class was extracted from MainWindow as part of Phase 2 refactoring
 * to decompose the God Object pattern and apply dependency injection.
 */
class FileManager : public QObject
{
    Q_OBJECT

public:
    explicit FileManager(QTabWidget* tabWidget, TabManager* tabManager, QSettings* settings, EventBus* eventBus, QObject* parent = nullptr);
    ~FileManager() = default;

    // Core file operations
    CalcForgeResult<bool> loadWorksheetFile(const QString& filePath = QString());
    CalcForgeResult<bool> saveWorksheetFile(const QString& filePath = QString());
    CalcForgeResult<bool> saveWorksheetFileAs();
    
    // File state management
    void markAsModified();
    void markAsSaved();
    bool hasUnsavedChanges() const;
    QString getCurrentFile() const { return m_currentFile; }
    
    // Recent files management
    QStringList getRecentFiles() const { return m_recentFiles; }
    void addToRecentFiles(const QString& filePath);
    void loadRecentFile(const QString& filePath);
    
    // Initialization
    void loadWorksheets();
    void loadExampleWorksheets();

signals:
    void fileStateChanged(bool hasUnsavedChanges);
    void currentFileChanged(const QString& filePath);
    void recentFilesChanged(const QStringList& recentFiles);
    void requestTemporaryDisableAlwaysOnTop();
    void requestRestoreAlwaysOnTop();

private slots:
    void onWorksheetModified();

private:
    // Internal file operations
    bool loadWorksheetContentFromFile(const QString& filePath);
    bool saveWorksheetsToFile(const QString& filePath);
    void loadSingleWorksheet(const QString& tabName, const QString& content);
    void loadIntoExistingTab(int tabIndex, const QString& tabName, const QString& content);
    void loadExampleWorksheetsAndSave();
    
    // Recent files internal management
    void loadRecentFiles();
    void saveRecentFiles();
    void createInitialRecentFilesJson();
    
    // Dependencies (injected)
    QTabWidget* m_tabWidget;
    TabManager* m_tabManager;
    QSettings* m_settings;
    EventBus* m_eventBus;
    
    // Internal state
    QString m_currentFile;
    bool m_isModified;
    bool m_isLoading;  // Flag to prevent marking as modified during initial load
    QStringList m_recentFiles;
    static const int MAX_RECENT_FILES = 10;
    
    // Helper methods
    WorksheetWidget* getWorksheetWidget(int index) const;
    void connectWorksheetSignals(WorksheetWidget* worksheet);
};

#endif // FILEMANAGER_H