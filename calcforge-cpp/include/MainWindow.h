#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QCheckBox>
#include <QMenuBar>
#include <QStatusBar>
#include <QToolBar>
#include <QSettings>
#include <QCloseEvent>
#include <QSplitter>
#include <QLabel>
#include <QShortcut>
#include "CustomTabWidget.h"

class WorksheetWidget;
struct LineChange;

// Phase 3: Manager classes for dependency injection
class FileManager;
class TabManager;
class CrossSheetNavigator;
class WindowManager;

// Phase 4.1: Event system
class EventBus;

/**
 * Main application window for CalcForge C++
 * Manages tabs, menu bar, toolbar, and overall application state
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void addTab();
    void closeTab(int index);
    void renameTab(int index);
    void onTabChanged(int index);
    void onTabMoved(int from, int to);
    void onSplitterMoved(const QByteArray &newState);
    void showHelp();
    void updateCurrencyRates();
    void toggleStayOnTop(bool enabled);
    void newFile();
    void openFile();
    void saveFile();
    void saveAsFile();
    void exitApplication();
    void showAbout();
    void onLineNumberingChanged(const QString &sheetName, const QList<LineChange> &changes);
    void onValuesChanged(const QString &sheetName);

    // Load/Save functionality - Phase 3: Now handled by managers
    void loadWorksheetFile();
    bool saveWorksheetFile();
    bool saveWorksheetFileAs();
    void showLoadDropdown();
    void showSaveDropdown();
    void toggleAlwaysOnTop(bool enabled);

    // Helper methods for dialog handling with always-on-top
    void temporarilyDisableAlwaysOnTop();
    void restoreAlwaysOnTop();

private:
    void setupUI();
    void setupMenuBar();
    void setupToolBar();
    void setupStatusBar();
    void setupShortcuts();
    
    // Phase 3: Manager initialization
    void initializeManagers();
    void createInitialTab();
    void createResizeCorners();
    void updateCornerPositions();
    void createResizeEdges();
    void updateEdgePositions();
    Qt::Edges getResizeEdges(const QPoint &pos) const;
    void loadWorksheets();
    void loadExampleWorksheets();
    void loadExampleWorksheetsAndSave();
    void loadWorksheetFromFile(const QString &filePath);
    bool loadWorksheetContentFromFile(const QString &filePath);
    void loadSingleWorksheet(const QString &tabName, const QString &content);
    void saveWorksheets();
    bool saveWorksheetsToFile(const QString &filePath);
    void restoreWindowState();
    void saveWindowState();
    void setupCrossSheetSupport(WorksheetWidget *worksheet, const QString &sheetName = QString());

    // Phase 3: Recent files and file state now handled by FileManager
    // Recent files management - REMOVED
    // Unsaved changes tracking - REMOVED

    // Font management
    void increaseFontSize();
    void decreaseFontSize();
    void resetFontSize();
    void applyGlobalFontSize(); // Helper method to apply font size to all tabs

    // Text selection shortcuts
    void selectCurrentLine();
    void smartParenthesesSelection();

    // Tab navigation
    void previousTab();
    void nextTab();

    // Cross-sheet reference support
    WorksheetWidget* getSheetByName(const QString &sheetName) const;
    void highlightIncomingCrossSheetReferences(WorksheetWidget *targetSheet);

public:
    // Cross-sheet navigation support
    QString getCurrentSheetName() const;
    void navigateToSheet(const QString &sheetName, int lineNumber, int cursorPosition = -1);
    void saveNavigationHistory(const QString &sheetName, int lineNumber, int cursorPosition);
    bool hasNavigationHistory() const;
    void returnToPreviousLocation();
    void triggerCrossSheetRecalculation();
    void recalculateAllWorksheets();
    bool updateCrossSheetReferences(WorksheetWidget *worksheet, const QString &changedSheetName, const QList<LineChange> &changes);
    int calculateNewLineNumber(int originalLine, const QList<LineChange> &changes) const;
    bool hasCrossSheetReferencesToSheet(WorksheetWidget *worksheet, const QString &sheetName) const;
    bool hasCircularCrossSheetDependencies(const QString &sheetName) const;
    bool detectSimpleCircularReferences(const QString &startSheet) const;
    bool detectCircularReferencesRecursive(const QString &sheetName, QSet<QString> &visited, QSet<QString> &currentPath) const;
    QSet<QString> getReferencedSheets(const QString &sheetName) const;
    double getCrossSheetValue(const QString &sheetName, int lineNumber) const;

    // Tab management for autocomplete
    int getTabCount() const;
    QString getTabName(int index) const;

    // UI Components
    QWidget *m_centralWidget;
    QVBoxLayout *m_mainLayout;
    QHBoxLayout *m_topLayout;
    CustomTabWidget *m_tabWidget;
    
    // Top bar controls
    QPushButton *m_addButton;
    QPushButton *m_loadButton;
    QPushButton *m_loadDropdownButton;
    QPushButton *m_saveButton;
    QPushButton *m_saveDropdownButton;
    QPushButton *m_currencyButton;
    QPushButton *m_alwaysOnTopButton;
    QPushButton *m_helpButton;

    // Note: Resize corners and edges now handled by WindowManager
    
    // Menu and toolbar
    QMenuBar *m_menuBar;
    QToolBar *m_toolBar;
    QStatusBar *m_statusBar;
    
    // Settings
    QSettings *m_settings;

    // Always on top state tracking
    bool m_isAlwaysOnTop;
    QByteArray m_splitterState;

    // Global font size management
    int m_globalFontSize;

    // Font size shortcuts
    QShortcut *m_increaseFontShortcut;
    QShortcut *m_decreaseFontShortcut;
    QShortcut *m_resetFontShortcut;
    
    // Note: File state (current file, modified) now handled by FileManager

    // Note: Recent files, window dragging/resizing now handled by managers

    // Phase 3: Manager classes (dependency injection)
    FileManager* m_fileManager;
    TabManager* m_tabManager;
    CrossSheetNavigator* m_crossSheetNavigator;
    WindowManager* m_windowManager;

    // Phase 4.1: Event system
    EventBus* m_eventBus;
};

#endif // MAINWINDOW_H
