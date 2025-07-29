#ifndef CALCULATIONSERVICE_H
#define CALCULATIONSERVICE_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <memory>

// Forward declarations
class WorksheetModel;
class CalculationEngine;
class DependencyTracker;

/**
 * CalculationService - Calculation coordination and business logic
 * Part of Phase 3.2: Separate Business Logic from UI
 * 
 * Handles:
 * - Expression evaluation coordination
 * - Dependency tracking and selective recalculation
 * - Cross-sheet calculation support
 * - Calculation result management
 */
class CalculationService : public QObject
{
    Q_OBJECT

public:
    explicit CalculationService(QObject *parent = nullptr);
    ~CalculationService();

    // Model management
    void setModel(WorksheetModel* model);
    WorksheetModel* getModel() const;

    // Calculation operations
    void recalculate();
    void forceRecalculation();
    QString evaluateExpression(const QString& expression);
    
    // Line-specific evaluation
    QString evaluateLineExpression(int lineNumber, const QString& expression);
    
    // Cross-sheet support
    void setCurrentSheetName(const QString& sheetName);
    QString getCurrentSheetName() const;
    
    // Dependency management
    void updateDependencies();
    bool hasCircularDependencies() const;

signals:
    void calculationCompleted();
    void lineValueChanged(int lineNumber, double value);
    void errorOccurred(const QString& error);

public slots:
    void onModelContentChanged();
    void onModelLineValueChanged(int lineNumber, double value);

private:
    void evaluateAllLines();
    void evaluateLine(int lineNumber, const QString& expression);
    QStringList splitContentIntoLines(const QString& content) const;
    
    WorksheetModel* m_model;
    std::unique_ptr<CalculationEngine> m_calculationEngine;
    std::unique_ptr<DependencyTracker> m_dependencyTracker;
    QString m_currentSheetName;
    bool m_isRecalculating;
};

#endif // CALCULATIONSERVICE_H
