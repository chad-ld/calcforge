#include "WorksheetModel.h"
#include "Logger.h"

WorksheetModel::WorksheetModel(QObject *parent)
    : QObject(parent)
    , m_isModified(false)
    , m_hasCrossSheetReferences(false)
{
    LOG_DEBUG("WorksheetModel: Created new worksheet model");
}

WorksheetModel::~WorksheetModel()
{
    LOG_DEBUG("WorksheetModel: Destroyed worksheet model");
}

QString WorksheetModel::getContent() const
{
    return m_content;
}

void WorksheetModel::setContent(const QString& content)
{
    if (m_content != content) {
        m_content = content;
        setModified(true);
        emit contentChanged();
        LOG_DEBUG("WorksheetModel: Content updated");
    }
}

bool WorksheetModel::isModified() const
{
    return m_isModified;
}

void WorksheetModel::setModified(bool modified)
{
    if (m_isModified != modified) {
        m_isModified = modified;
        emit modificationStateChanged(modified);
        LOG_DEBUG(QString("WorksheetModel: Modification state changed to %1").arg(modified ? "true" : "false"));
    }
}

double WorksheetModel::getLineValue(int lineNumber) const
{
    return m_lineValues.value(lineNumber, 0.0);
}

bool WorksheetModel::hasLineValue(int lineNumber) const
{
    return m_lineValues.contains(lineNumber);
}

void WorksheetModel::setLineValue(int lineNumber, double value)
{
    if (!m_lineValues.contains(lineNumber) || m_lineValues[lineNumber] != value) {
        m_lineValues[lineNumber] = value;
        emit lineValueChanged(lineNumber, value);
        LOG_DEBUG(QString("WorksheetModel: Line %1 value set to %2").arg(lineNumber).arg(value));
    }
}

void WorksheetModel::clearLineValue(int lineNumber)
{
    int removedCount = m_lineValues.remove(lineNumber);
    if (removedCount > 0) {
        emit lineValueChanged(lineNumber, 0.0);
        LOG_DEBUG(QString("WorksheetModel: Line %1 value cleared").arg(lineNumber));
    }
}

void WorksheetModel::clearAllLineValues()
{
    if (!m_lineValues.isEmpty()) {
        m_lineValues.clear();
        LOG_DEBUG("WorksheetModel: All line values cleared");
    }
}

QString WorksheetModel::getSheetName() const
{
    return m_sheetName;
}

void WorksheetModel::setSheetName(const QString& sheetName)
{
    if (m_sheetName != sheetName) {
        m_sheetName = sheetName;
        LOG_DEBUG(QString("WorksheetModel: Sheet name set to '%1'").arg(sheetName));
    }
}

bool WorksheetModel::hasCrossSheetReferences() const
{
    return m_hasCrossSheetReferences;
}

void WorksheetModel::setHasCrossSheetReferences(bool hasReferences)
{
    if (m_hasCrossSheetReferences != hasReferences) {
        m_hasCrossSheetReferences = hasReferences;
        emit crossSheetReferencesChanged();
        LOG_DEBUG(QString("WorksheetModel: Cross-sheet references changed to %1").arg(hasReferences ? "true" : "false"));
    }
}
