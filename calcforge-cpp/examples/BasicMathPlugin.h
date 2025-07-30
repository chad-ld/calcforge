#ifndef BASICMATHPLUGIN_H
#define BASICMATHPLUGIN_H

#include "../include/ICalculatorPlugin.h"
#include "../include/ICalculator.h"
#include <QRegularExpression>
#include <cmath>

/**
 * Example plugin that provides basic mathematical operations
 * Demonstrates how to create a CalcForge calculator plugin
 * 
 * Phase 4.2: Plugin Architecture Example
 */
class BasicMathCalculator : public ICalculator
{
public:
    CalcForgeResult<QString> calculate(const QString& expression) override;
    QString getType() const override { return "BasicMath"; }
    bool canHandle(const QString& expression) const override;
    QString getDescription() const override { return "Basic mathematical operations (factorial, power, etc.)"; }
    int getPriority() const override { return 10; }
    QStringList getExamples() const override;

private:
    double factorial(int n) const;
    double power(double base, double exponent) const;
};

/**
 * Plugin implementation that provides the BasicMathCalculator
 */
class BasicMathPlugin : public BaseCalculatorPlugin
{
public:
    BasicMathPlugin() : BaseCalculatorPlugin("Basic Math Plugin", "1.0.0", "CalcForge Team") {}
    
    std::unique_ptr<ICalculator> createCalculator() override {
        return std::make_unique<BasicMathCalculator>();
    }
    
    QString getDescription() const override {
        return "Provides basic mathematical operations like factorial and power functions";
    }
    
    QString getCalculatorType() const override {
        return "BasicMath";
    }
};

// Plugin export macros
DECLARE_CALCFORGE_PLUGIN(BasicMathPlugin)

#endif // BASICMATHPLUGIN_H
