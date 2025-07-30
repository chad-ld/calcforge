#ifndef WORKSHEETEVENTS_H
#define WORKSHEETEVENTS_H

#include <QObject>
#include <QString>
#include <QList>
#include <QByteArray>
#include "LineChangeDetector.h"

/**
 * @brief Central event hub for worksheet-related events
 * 
 * This class implements the observer pattern to decouple worksheet components.
 * Instead of direct signal-slot connections between components, all worksheet
 * events flow through this central hub, reducing coupling and improving
 * maintainability.
 * 
 * Phase 4.1: Event System Implementation
 */
class WorksheetEvents : public QObject
{
    Q_OBJECT

public:
    explicit WorksheetEvents(QObject *parent = nullptr);
    ~WorksheetEvents() = default;

    // Event emission methods
    void emitLineValueChanged(const QString& sheetName, int lineNumber, double value);
    void emitContentChanged(const QString& sheetName);
    void emitSheetRenamed(const QString& oldName, const QString& newName);
    void emitLineNumberingChanged(const QString& sheetName, const QList<LineChange>& changes);
    void emitValuesChanged(const QString& sheetName);
    void emitSplitterMoved(const QString& sheetName, const QByteArray& newState);
    void emitCrossSheetReferencesChanged(const QString& sheetName);
    void emitCalculationCompleted(const QString& sheetName);
    void emitCalculationError(const QString& sheetName, const QString& error);
    void emitNavigationRequested(const QString& sheetName, int lineNumber, int cursorPosition = -1);

signals:
    // Core worksheet events
    void lineValueChanged(const QString& sheetName, int lineNumber, double value);
    void contentChanged(const QString& sheetName);
    void sheetRenamed(const QString& oldName, const QString& newName);
    void lineNumberingChanged(const QString& sheetName, const QList<LineChange>& changes);
    void valuesChanged(const QString& sheetName);
    
    // UI-related events
    void splitterMoved(const QString& sheetName, const QByteArray& newState);
    
    // Cross-sheet events
    void crossSheetReferencesChanged(const QString& sheetName);
    void crossSheetRecalculationRequested();
    
    // Calculation events
    void calculationCompleted(const QString& sheetName);
    void calculationError(const QString& sheetName, const QString& error);
    
    // Navigation events
    void navigationRequested(const QString& sheetName, int lineNumber, int cursorPosition);
};

#endif // WORKSHEETEVENTS_H
