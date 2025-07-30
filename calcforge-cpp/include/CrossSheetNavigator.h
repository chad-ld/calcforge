#ifndef CROSSSHEETNAVIGATOR_H
#define CROSSSHEETNAVIGATOR_H

#include <QObject>
#include <QSet>
#include <QString>
#include <QList>
#include "CalcForgeResult.h"
#include "LineChangeDetector.h"

class TabManager;
class WorksheetWidget;
class EventBus;

/**
 * CrossSheetNavigator handles cross-sheet references and navigation for CalcForge
 * Including navigation history, cross-sheet value lookup, circular dependency detection,
 * and cross-sheet reference updates when line numbers change
 * 
 * This class was extracted from MainWindow as part of Phase 2 refactoring
 * to decompose the God Object pattern and apply dependency injection.
 */
class CrossSheetNavigator : public QObject
{
    Q_OBJECT

public:
    explicit CrossSheetNavigator(TabManager* tabManager, EventBus* eventBus, QObject* parent = nullptr);
    ~CrossSheetNavigator() = default;

    // Navigation operations
    void navigateToSheet(const QString& sheetName, int lineNumber, int cursorPosition = -1);
    void saveNavigationHistory(const QString& sheetName, int lineNumber, int cursorPosition);
    bool hasNavigationHistory() const;
    void returnToPreviousLocation();
    
    // Cross-sheet value operations
    double getCrossSheetValue(const QString& sheetName, int lineNumber) const;
    
    // Cross-sheet reference management
    void triggerCrossSheetRecalculation();
    void recalculateAllWorksheets();
    bool updateCrossSheetReferences(WorksheetWidget* worksheet, const QString& changedSheetName, 
                                   const QList<LineChange>& changes);
    
    // Circular dependency detection
    bool hasCircularCrossSheetDependencies(const QString& sheetName) const;
    bool hasCrossSheetReferencesToSheet(WorksheetWidget* worksheet, const QString& sheetName) const;
    
    // Utility methods
    int calculateNewLineNumber(int originalLine, const QList<LineChange>& changes) const;
    QSet<QString> getReferencedSheets(const QString& sheetName) const;

signals:
    void navigationRequested(const QString& sheetName, int lineNumber, int cursorPosition);
    void crossSheetRecalculationTriggered();

public slots:
    void onLineNumberingChanged(const QString& sheetName, const QList<LineChange>& changes);
    void onValuesChanged(const QString& sheetName);

private:
    // Navigation history structure
    struct NavigationHistory {
        QString sheetName;
        int lineNumber;
        int cursorPosition;
        
        NavigationHistory() : lineNumber(-1), cursorPosition(-1) {}
        NavigationHistory(const QString& sheet, int line, int cursor)
            : sheetName(sheet), lineNumber(line), cursorPosition(cursor) {}
        
        bool isValid() const { return !sheetName.isEmpty() && lineNumber >= 0; }
    };
    
    // Internal navigation methods
    void performNavigation(const QString& sheetName, int lineNumber, int cursorPosition);
    void highlightIncomingCrossSheetReferences(WorksheetWidget* targetSheet);
    
    // Circular dependency detection helpers
    bool detectSimpleCircularReferences(const QString& startSheet) const;
    bool detectCircularReferencesRecursive(const QString& sheetName, 
                                         QSet<QString>& visited, 
                                         QSet<QString>& currentPath) const;
    
    // Cross-sheet reference extraction
    QSet<QString> extractReferencedSheetsFromContent(const QString& content) const;
    
    // Dependencies (injected)
    TabManager* m_tabManager;
    EventBus* m_eventBus;
    
    // Internal state
    NavigationHistory m_navigationHistory;
    bool m_hasNavigationHistory;
};

#endif // CROSSSHEETNAVIGATOR_H