#include "CalculationService.h"
#include "WorksheetModel.h"
#include "CalculationEngine.h"
#include "DependencyTracker.h"
#include "Logger.h"
#include <QRegularExpression>
#include <cmath>

CalculationService::CalculationService(QObject *parent)
    : QObject(parent)
    , m_model(nullptr)
    , m_calculationEngine(std::make_unique<CalculationEngine>())
    , m_dependencyTracker(std::make_unique<DependencyTracker>())
    , m_isRecalculating(false)
{
    LOG_DEBUG("CalculationService: Created new calculation service");
}

CalculationService::~CalculationService()
{
    LOG_DEBUG("CalculationService: Destroyed calculation service");
}

void CalculationService::setModel(WorksheetModel* model)
{
    if (m_model) {
        // Disconnect from previous model
        disconnect(m_model, &WorksheetModel::contentChanged, this, &CalculationService::onModelContentChanged);
        disconnect(m_model, &WorksheetModel::lineValueChanged, this, &CalculationService::onModelLineValueChanged);
    }
    
    m_model = model;
    
    if (m_model) {
        // Connect to new model
        connect(m_model, &WorksheetModel::contentChanged, this, &CalculationService::onModelContentChanged);
        connect(m_model, &WorksheetModel::lineValueChanged, this, &CalculationService::onModelLineValueChanged);
        
        // Set sheet name in calculation engine
        if (!m_currentSheetName.isEmpty()) {
            m_calculationEngine->setCurrentSheetName(m_currentSheetName);
        }
        
        LOG_DEBUG("CalculationService: Model connected");
    }
}

WorksheetModel* CalculationService::getModel() const
{
    return m_model;
}

void CalculationService::recalculate()
{
    if (!m_model || m_isRecalculating) {
        return;
    }
    
    m_isRecalculating = true;
    LOG_DEBUG("CalculationService: Starting recalculation");
    
    try {
        evaluateAllLines();
        updateDependencies();
        emit calculationCompleted();
        LOG_DEBUG("CalculationService: Recalculation completed");
    } catch (const std::exception& e) {
        LOG_DEBUG(QString("CalculationService: Recalculation error: %1").arg(e.what()));
        emit errorOccurred(QString("Calculation error: %1").arg(e.what()));
    }
    
    m_isRecalculating = false;
}

void CalculationService::forceRecalculation()
{
    if (!m_model) {
        return;
    }
    
    LOG_DEBUG("CalculationService: Force recalculation requested");
    m_model->clearAllLineValues();
    recalculate();
}

QString CalculationService::evaluateExpression(const QString& expression)
{
    if (!m_calculationEngine) {
        return "Error: No calculation engine";
    }
    
    try {
        return m_calculationEngine->evaluateExpression(expression);
    } catch (const std::exception& e) {
        return QString("Error: %1").arg(e.what());
    }
}

QString CalculationService::evaluateLineExpression(int lineNumber, const QString& expression)
{
    if (!m_model || !m_calculationEngine) {
        return "Error: No model or calculation engine";
    }
    
    try {
        QString result = m_calculationEngine->evaluateExpression(expression);
        
        // Try to parse as numeric value and store in model
        bool ok;
        double numericValue = result.toDouble(&ok);
        if (ok && !std::isnan(numericValue) && !std::isinf(numericValue)) {
            m_model->setLineValue(lineNumber, numericValue);
        }
        
        return result;
    } catch (const std::exception& e) {
        return QString("Error: %1").arg(e.what());
    }
}

void CalculationService::setCurrentSheetName(const QString& sheetName)
{
    m_currentSheetName = sheetName;
    if (m_calculationEngine) {
        m_calculationEngine->setCurrentSheetName(sheetName);
    }
    if (m_model) {
        m_model->setSheetName(sheetName);
    }
    LOG_DEBUG(QString("CalculationService: Current sheet name set to '%1'").arg(sheetName));
}

QString CalculationService::getCurrentSheetName() const
{
    return m_currentSheetName;
}

void CalculationService::updateDependencies()
{
    if (!m_model || !m_dependencyTracker) {
        return;
    }

    // Update dependency tracker with current content
    QString content = m_model->getContent();
    QStringList lines = content.split('\n');

    // Clear existing dependencies
    m_dependencyTracker->clear();

    // Update dependencies for each line
    for (int i = 0; i < lines.size(); ++i) {
        int lineNumber = i + 1; // 1-based line numbering
        QString line = lines[i].trimmed();

        if (!line.isEmpty() && !line.startsWith(":::")) { // Skip comments
            m_dependencyTracker->updateLineDependencies(lineNumber, line, lines.size());
            if (!m_currentSheetName.isEmpty()) {
                m_dependencyTracker->updateCrossSheetDependencies(lineNumber, line, m_currentSheetName);
            }
        }
    }
}

bool CalculationService::hasCircularDependencies() const
{
    if (!m_dependencyTracker) {
        return false;
    }
    
    return m_dependencyTracker->hasCircularDependencies();
}

void CalculationService::onModelContentChanged()
{
    LOG_DEBUG("CalculationService: Model content changed, triggering recalculation");
    recalculate();
}

void CalculationService::onModelLineValueChanged(int lineNumber, double value)
{
    emit lineValueChanged(lineNumber, value);
}

void CalculationService::evaluateAllLines()
{
    if (!m_model) {
        return;
    }
    
    QString content = m_model->getContent();
    QStringList lines = splitContentIntoLines(content);
    
    for (int i = 0; i < lines.size(); ++i) {
        int lineNumber = i + 1; // 1-based line numbering
        QString line = lines[i].trimmed();
        
        if (!line.isEmpty() && !line.startsWith(":::")) { // Skip comments
            evaluateLine(lineNumber, line);
        }
    }
}

void CalculationService::evaluateLine(int lineNumber, const QString& expression)
{
    QString result = evaluateLineExpression(lineNumber, expression);
    // Result is already handled in evaluateLineExpression
}

QStringList CalculationService::splitContentIntoLines(const QString& content) const
{
    return content.split('\n');
}
