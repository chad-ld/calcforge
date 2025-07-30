#ifndef ICALCULATOR_H
#define ICALCULATOR_H

#include <QString>
#include <memory>
#include "CalcForgeResult.h"

/**
 * Interface for all CalcForge calculator implementations
 * Enables dependency injection and polymorphic calculator management
 * Replaces hard-coded calculator instantiation in CalculationEngine
 */
class ICalculator
{
public:
    virtual ~ICalculator() = default;
    
    /**
     * Calculate/evaluate the given expression
     * @param expression The expression to evaluate
     * @return CalcForgeResult<QString> with the formatted result or error
     */
    virtual CalcForgeResult<QString> calculate(const QString& expression) = 0;
    
    /**
     * Get the type identifier for this calculator
     * @return String identifier (e.g., "UnitConverter", "TimecodeCalculator")
     */
    virtual QString getType() const = 0;
    
    /**
     * Check if this calculator can handle the given expression
     * @param expression The expression to check
     * @return True if this calculator can process the expression
     */
    virtual bool canHandle(const QString& expression) const = 0;
    
    /**
     * Get a human-readable description of this calculator
     * @return Description string for UI/help purposes
     */
    virtual QString getDescription() const = 0;
    
    /**
     * Get the priority for this calculator when multiple calculators can handle an expression
     * Higher priority calculators are tried first
     * @return Priority value (higher = more priority)
     */
    virtual int getPriority() const { return 0; }
    
    /**
     * Get example expressions that this calculator can handle
     * @return List of example expressions for documentation/help
     */
    virtual QStringList getExamples() const = 0;
};

/**
 * Factory interface for creating calculator instances
 * Enables plugin architecture and runtime calculator registration
 */
class ICalculatorFactory
{
public:
    virtual ~ICalculatorFactory() = default;
    
    /**
     * Create a new instance of the calculator
     * @return Unique pointer to the calculator instance
     */
    virtual std::unique_ptr<ICalculator> createCalculator() = 0;
    
    /**
     * Get the calculator type this factory creates
     * @return Calculator type identifier
     */
    virtual QString getCalculatorType() const = 0;
};

/**
 * Template factory implementation for easy calculator registration
 */
template<typename CalculatorType>
class CalculatorFactory : public ICalculatorFactory
{
public:
    std::unique_ptr<ICalculator> createCalculator() override {
        return std::make_unique<CalculatorType>();
    }
    
    QString getCalculatorType() const override {
        // Create temporary instance to get type - could be optimized with static method
        auto temp = std::make_unique<CalculatorType>();
        return temp->getType();
    }
};

// Convenience macros for calculator registration
#define REGISTER_CALCULATOR(CalculatorClass) \
    std::make_unique<CalculatorFactory<CalculatorClass>>()

#define DECLARE_CALCULATOR_TYPE(TypeName) \
    QString getType() const override { return TypeName; }

#endif // ICALCULATOR_H