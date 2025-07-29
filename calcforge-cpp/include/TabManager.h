#ifndef TABMANAGER_H
#define TABMANAGER_H

#include <QObject>
#include <QTabWidget>
#include <QSettings>
#include <QMap>
#include "CalcForgeResult.h"

class WorksheetWidget;

/**
 * TabManager handles all tab-related operations for CalcForge
 * Including tab creation, deletion, navigation, and tab state management
 * 
 * This class was extracted from MainWindow as part of Phase 2 refactoring
 * to decompose the God Object pattern and apply dependency injection.
 */
class TabManager : public QObject
{
    Q_OBJECT

public:
    explicit TabManager(QTabWidget* tabWidget, QSettings* settings, QObject* parent = nullptr);
    ~TabManager() = default;

    // Tab operations
    CalcForgeResult<int> addTab(const QString& name = QString());
    CalcForgeResult<bool> closeTab(int index);
    CalcForgeResult<bool> renameTab(int index, const QString& newName = QString());
    
    // Tab navigation
    void navigateToTab(int index);
    void previousTab();
    void nextTab();
    
    // Tab information
    int getTabCount() const;
    QString getTabName(int index) const;
    int getCurrentTabIndex() const;
    QString getCurrentTabName() const;
    
    // WorksheetWidget access
    WorksheetWidget* getWorksheetWidget(int index) const;
    WorksheetWidget* getCurrentWorksheetWidget() const;
    
    // Font management
    void increaseFontSize();
    void decreaseFontSize();
    void resetFontSize();
    WorksheetWidget* getWorksheetByName(const QString& name) const;
    
    // Font management for all tabs
    void applyGlobalFontSize(int fontSize);
    
    // Fix existing tabs to handle & characters properly
    void fixExistingTabNames();

signals:
    void tabAdded(int index, const QString& name);
    void tabClosed(int index, const QString& name);
    void tabRenamed(int index, const QString& oldName, const QString& newName);
    void currentTabChanged(int index);
    void splitterMoved(const QByteArray& newState);
    void worksheetNeedsSetup(WorksheetWidget* worksheet, const QString& name);

public slots:
    void onTabChanged(int index);
    void onSplitterMoved(const QByteArray& newState);

private:
    // Tab creation and setup
    WorksheetWidget* createWorksheetWidget();
    void setupWorksheetWidget(WorksheetWidget* worksheet, const QString& name);
    void connectWorksheetSignals(WorksheetWidget* worksheet);
    void setupCustomCloseButton(int index);
    
    // Tab validation
    bool isValidTabIndex(int index) const;
    QString generateUniqueTabName(const QString& baseName = "Worksheet") const;
    
    // Dependencies (injected)
    QTabWidget* m_tabWidget;
    QSettings* m_settings;
    
    // Internal state
    int m_currentFontSize;
    int m_nextTabNumber;
    QMap<int, QString> m_originalTabNames;  // Map to store original tab names without escaping
};

#endif // TABMANAGER_H