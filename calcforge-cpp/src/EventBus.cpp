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
    // Phase 4.1: Enhanced cross-cutting event coordination

    // When worksheet content changes, it may affect file state
    connect(m_worksheetEvents.get(), &WorksheetEvents::contentChanged,
            this, [this](const QString& sheetName) {
                Q_UNUSED(sheetName)
                // Content change implies the file has been modified
                m_applicationEvents->emitFileStateChanged(true);
                LOG_DEBUG("EventBus: Content change triggered file state update");
            });

    // When cross-sheet recalculation is requested, coordinate application-level response
    connect(m_worksheetEvents.get(), &WorksheetEvents::crossSheetRecalculationRequested,
            this, [this]() {
                LOG_DEBUG("EventBus: Cross-sheet recalculation requested - coordinating response");
                // Could trigger additional application-level events here if needed
            });

    // When line values change, it may trigger cross-sheet updates
    connect(m_worksheetEvents.get(), &WorksheetEvents::lineValueChanged,
            this, [this](const QString& sheetName, int lineNumber, double value) {
                LOG_DEBUG(QString("EventBus: Line value changed - Sheet: %1, Line: %2, Value: %3")
                         .arg(sheetName).arg(lineNumber).arg(value));
                // This could trigger cross-sheet dependency updates
            });

    // When calculation errors occur, log them at application level
    connect(m_worksheetEvents.get(), &WorksheetEvents::calculationError,
            this, [this](const QString& sheetName, const QString& error) {
                LOG_DEBUG(QString("EventBus: Calculation error in sheet %1: %2").arg(sheetName).arg(error));
            });

    LOG_DEBUG("EventBus: Enhanced event connections established");
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

// Phase 4.1: Enhanced event coordination methods
void EventBus::emitNavigationRequested(const QString& sheetName, int lineNumber, int cursorPosition)
{
    m_worksheetEvents->emitNavigationRequested(sheetName, lineNumber, cursorPosition);
}

void EventBus::emitCalculationCompleted(const QString& sheetName)
{
    m_worksheetEvents->emitCalculationCompleted(sheetName);
}

void EventBus::emitCalculationError(const QString& sheetName, const QString& error)
{
    m_worksheetEvents->emitCalculationError(sheetName, error);
}
