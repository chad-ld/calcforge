#include "EventBus.h"
#include "WorksheetEvents.h"
#include "ApplicationEvents.h"
#include "Logger.h"

// Static instance for singleton pattern
EventBus* EventBus::s_instance = nullptr;

EventBus::EventBus(QObject *parent)
    : QObject(parent)
    , m_worksheetEvents(std::make_unique<WorksheetEvents>(this))
    , m_applicationEvents(std::make_unique<ApplicationEvents>(this))
{
    LOG_DEBUG("EventBus: Initializing central event coordination hub");
    setupEventConnections();
}

EventBus::~EventBus()
{
    LOG_DEBUG("EventBus: Shutting down event coordination hub");
    if (s_instance == this) {
        s_instance = nullptr;
    }
}

EventBus* EventBus::instance()
{
    return s_instance;
}

void EventBus::setInstance(EventBus* instance)
{
    s_instance = instance;
    LOG_DEBUG("EventBus: Global instance set");
}

void EventBus::setupEventConnections()
{
    // Connect cross-cutting concerns between worksheet and application events
    
    // When worksheet content changes, it may affect file state
    connect(m_worksheetEvents.get(), &WorksheetEvents::contentChanged,
            this, [this](const QString& sheetName) {
                Q_UNUSED(sheetName)
                // Content change implies the file has been modified
                m_applicationEvents->emitFileStateChanged(true);
            });
    
    // When cross-sheet recalculation is requested, log it at application level
    connect(m_worksheetEvents.get(), &WorksheetEvents::crossSheetRecalculationRequested,
            this, []() {
                LOG_DEBUG("EventBus: Cross-sheet recalculation requested via worksheet events");
            });
    
    LOG_DEBUG("EventBus: Event connections established");
}

// Convenience methods for common event patterns
void EventBus::emitWorksheetContentChanged(const QString& sheetName)
{
    m_worksheetEvents->emitContentChanged(sheetName);
}

void EventBus::emitWorksheetValueChanged(const QString& sheetName, int lineNumber, double value)
{
    m_worksheetEvents->emitLineValueChanged(sheetName, lineNumber, value);
}

void EventBus::emitTabChanged(int index, const QString& name)
{
    m_applicationEvents->emitCurrentTabChanged(index);
}

void EventBus::emitFileStateChanged(bool hasUnsavedChanges)
{
    m_applicationEvents->emitFileStateChanged(hasUnsavedChanges);
}

void EventBus::emitCrossSheetRecalculationNeeded()
{
    m_worksheetEvents->emitCrossSheetReferencesChanged("");
}
