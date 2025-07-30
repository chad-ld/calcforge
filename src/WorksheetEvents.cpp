#include "WorksheetEvents.h"
#include "LineChangeDetector.h"
#include "Logger.h"

WorksheetEvents::WorksheetEvents(QObject *parent)
    : QObject(parent)
{
    LOG_DEBUG("WorksheetEvents: Event hub initialized");
}

void WorksheetEvents::emitLineValueChanged(const QString& sheetName, int lineNumber, double value)
{
    LOG_DEBUG(QString("WorksheetEvents: Line value changed - Sheet: %1, Line: %2, Value: %3")
              .arg(sheetName).arg(lineNumber).arg(value));
    emit lineValueChanged(sheetName, lineNumber, value);
}

void WorksheetEvents::emitContentChanged(const QString& sheetName)
{
    LOG_DEBUG(QString("WorksheetEvents: Content changed - Sheet: %1").arg(sheetName));
    emit contentChanged(sheetName);
}

void WorksheetEvents::emitSheetRenamed(const QString& oldName, const QString& newName)
{
    LOG_DEBUG(QString("WorksheetEvents: Sheet renamed - Old: %1, New: %2").arg(oldName, newName));
    emit sheetRenamed(oldName, newName);
}

void WorksheetEvents::emitLineNumberingChanged(const QString& sheetName, const QList<LineChange>& changes)
{
    LOG_DEBUG(QString("WorksheetEvents: Line numbering changed - Sheet: %1, Changes: %2")
              .arg(sheetName).arg(changes.size()));
    emit lineNumberingChanged(sheetName, changes);
}

void WorksheetEvents::emitValuesChanged(const QString& sheetName)
{
    LOG_DEBUG(QString("WorksheetEvents: Values changed - Sheet: %1").arg(sheetName));
    emit valuesChanged(sheetName);
}

void WorksheetEvents::emitSplitterMoved(const QString& sheetName, const QByteArray& newState)
{
    LOG_DEBUG(QString("WorksheetEvents: Splitter moved - Sheet: %1").arg(sheetName));
    emit splitterMoved(sheetName, newState);
}

void WorksheetEvents::emitCrossSheetReferencesChanged(const QString& sheetName)
{
    LOG_DEBUG(QString("WorksheetEvents: Cross-sheet references changed - Sheet: %1").arg(sheetName));
    emit crossSheetReferencesChanged(sheetName);
    
    // Automatically trigger cross-sheet recalculation when references change
    emit crossSheetRecalculationRequested();
}

void WorksheetEvents::emitCalculationCompleted(const QString& sheetName)
{
    LOG_DEBUG(QString("WorksheetEvents: Calculation completed - Sheet: %1").arg(sheetName));
    emit calculationCompleted(sheetName);
}

void WorksheetEvents::emitCalculationError(const QString& sheetName, const QString& error)
{
    LOG_DEBUG(QString("WorksheetEvents: Calculation error - Sheet: %1, Error: %2").arg(sheetName, error));
    emit calculationError(sheetName, error);
}

void WorksheetEvents::emitNavigationRequested(const QString& sheetName, int lineNumber, int cursorPosition)
{
    LOG_DEBUG(QString("WorksheetEvents: Navigation requested - Sheet: %1, Line: %2, Cursor: %3")
              .arg(sheetName).arg(lineNumber).arg(cursorPosition));
    emit navigationRequested(sheetName, lineNumber, cursorPosition);
}
