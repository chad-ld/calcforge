#ifndef EVENTBUS_H
#define EVENTBUS_H

#include <QObject>
#include <memory>
#include "WorksheetEvents.h"
#include "ApplicationEvents.h"

/**
 * @brief Central event coordination hub for CalcForge
 * 
 * The EventBus provides a single point of access to all event systems in
 * CalcForge. It coordinates between WorksheetEvents and ApplicationEvents,
 * and provides a clean interface for components to emit and subscribe to events.
 * 
 * This implements the Mediator pattern to reduce coupling between components
 * and centralize event coordination.
 * 
 * Phase 4.1: Event System Implementation
 */
class EventBus : public QObject
{
    Q_OBJECT

public:
    explicit EventBus(QObject *parent = nullptr);
    ~EventBus();

    // Singleton access (for global event coordination)
    static EventBus* instance();
    static void setInstance(EventBus* instance);

    // Event hub accessors
    WorksheetEvents* worksheetEvents() const { return m_worksheetEvents.get(); }
    ApplicationEvents* applicationEvents() const { return m_applicationEvents.get(); }

    // Convenience methods for common event patterns
    void emitWorksheetContentChanged(const QString& sheetName);
    void emitWorksheetValueChanged(const QString& sheetName, int lineNumber, double value);
    void emitTabChanged(int index, const QString& name);
    void emitFileStateChanged(bool hasUnsavedChanges);
    void emitCrossSheetRecalculationNeeded();

    // Phase 4.1: Enhanced event coordination methods
    void emitNavigationRequested(const QString& sheetName, int lineNumber, int cursorPosition = -1);
    void emitCalculationCompleted(const QString& sheetName);
    void emitCalculationError(const QString& sheetName, const QString& error);

private:
    std::unique_ptr<WorksheetEvents> m_worksheetEvents;
    std::unique_ptr<ApplicationEvents> m_applicationEvents;
    
    static EventBus* s_instance;
    
    void setupEventConnections();
};

#endif // EVENTBUS_H
