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

class WorksheetWidget;
struct LineChange;

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

private:
    void setupUI();
    void setupMenuBar();
    void setupToolBar();
    void setupStatusBar();
    void setupShortcuts();
    void createInitialTab();
    void createResizeCorners();
    void updateCornerPositions();
    void createResizeEdges();
    void updateEdgePositions();
    Qt::Edges getResizeEdges(const QPoint &pos) const;
    void loadWorksheets();
    void loadSingleWorksheet(const QString &tabName, const QString &content);
    void saveWorksheets();
    void restoreWindowState();
    void saveWindowState();
    void setupCrossSheetSupport(WorksheetWidget *worksheet);

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
    void triggerCrossSheetRecalculation();
    void recalculateAllWorksheets();
    bool updateCrossSheetReferences(WorksheetWidget *worksheet, const QString &changedSheetName, const QList<LineChange> &changes);
    int calculateNewLineNumber(int originalLine, const QList<LineChange> &changes) const;
    bool hasCrossSheetReferencesToSheet(WorksheetWidget *worksheet, const QString &sheetName) const;
    bool hasCircularCrossSheetDependencies(const QString &sheetName) const;
    bool detectSimpleCircularReferences(const QString &startSheet) const;
    bool detectCircularReferencesRecursive(const QString &sheetName, QSet<QString> &visited, QSet<QString> &currentPath) const;
    QSet<QString> getReferencedSheets(const QString &sheetName) const;
    
    // UI Components
    QWidget *m_centralWidget;
    QVBoxLayout *m_mainLayout;
    QHBoxLayout *m_topLayout;
    QTabWidget *m_tabWidget;
    
    // Top bar controls
    QPushButton *m_addButton;
    QPushButton *m_currencyButton;
    QPushButton *m_helpButton;

    // Resize corner indicators
    QWidget *m_bottomLeftCorner;
    QWidget *m_bottomRightCorner;

    // Resize edge indicators
    QWidget *m_leftEdgeIndicator;
    QWidget *m_rightEdgeIndicator;
    QWidget *m_bottomEdgeIndicator;
    
    // Menu and toolbar
    QMenuBar *m_menuBar;
    QToolBar *m_toolBar;
    QStatusBar *m_statusBar;
    
    // Settings
    QSettings *m_settings;
    QByteArray m_splitterState;

    // Global font size management
    int m_globalFontSize;

    // Font size shortcuts
    QShortcut *m_increaseFontShortcut;
    QShortcut *m_decreaseFontShortcut;
    QShortcut *m_resetFontShortcut;
    
    // Application state
    QString m_currentFile;
    bool m_isModified;

    // Window dragging and resizing
    QPoint m_dragPosition;
    bool m_dragging;
    bool m_resizing;
    Qt::Edges m_resizeEdges;
};

#endif // MAINWINDOW_H
