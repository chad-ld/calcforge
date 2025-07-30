#include "BasicMathPlugin.h"
#include <QRegularExpression>
#include <cmath>

CalcForgeResult<QString> BasicMathCalculator::calculate(const QString& expression)
{
    QString expr = expression.trimmed();
    
    // Handle factorial: n!
    QRegularExpression factorialRegex(R"((\d+)!)");
    QRegularExpressionMatch factorialMatch = factorialRegex.match(expr);
    if (factorialMatch.hasMatch()) {
        bool ok;
        int n = factorialMatch.captured(1).toInt(&ok);
        if (ok && n >= 0 && n <= 20) { // Limit to prevent overflow
            double result = factorial(n);
            return CalcForgeResult<QString>::success(QString::number(result, 'f', 0));
        } else {
            return CalcForgeResult<QString>::error("Invalid factorial input (must be 0-20)");
        }
    }
    
    // Handle power: base^exponent
    QRegularExpression powerRegex(R"((-?\d+(?:\.\d+)?)\s*\^\s*(-?\d+(?:\.\d+)?))");
    QRegularExpressionMatch powerMatch = powerRegex.match(expr);
    if (powerMatch.hasMatch()) {
        bool ok1, ok2;
        double base = powerMatch.captured(1).toDouble(&ok1);
        double exponent = powerMatch.captured(2).toDouble(&ok2);
        if (ok1 && ok2) {
            double result = power(base, exponent);
            if (std::isfinite(result)) {
                return CalcForgeResult<QString>::success(QString::number(result, 'g', 10));
            } else {
                return CalcForgeResult<QString>::error("Power calculation resulted in infinite or invalid value");
            }
        }
    }
    
    // Handle square root: sqrt(n)
    QRegularExpression sqrtRegex(R"(sqrt\s*\(\s*(-?\d+(?:\.\d+)?)\s*\))");
    QRegularExpressionMatch sqrtMatch = sqrtRegex.match(expr);
    if (sqrtMatch.hasMatch()) {
        bool ok;
        double n = sqrtMatch.captured(1).toDouble(&ok);
        if (ok && n >= 0) {
            double result = std::sqrt(n);
            return CalcForgeResult<QString>::success(QString::number(result, 'g', 10));
        } else {
            return CalcForgeResult<QString>::error("Square root of negative number");
        }
    }
    
    return CalcForgeResult<QString>::error("Expression not supported by BasicMath plugin");
}

bool BasicMathCalculator::canHandle(const QString& expression) const
{
    QString expr = expression.trimmed();
    
    // Check for factorial
    if (expr.contains(QRegularExpression(R"(\d+!)"))) {
        return true;
    }
    
    // Check for power operation
    if (expr.contains(QRegularExpression(R"(\d+(?:\.\d+)?\s*\^\s*\d+(?:\.\d+)?)"))) {
        return true;
    }
    
    // Check for square root
    if (expr.contains(QRegularExpression(R"(sqrt\s*\(\s*\d+(?:\.\d+)?\s*\))"))) {
        return true;
    }
    
    return false;
}

QStringList BasicMathCalculator::getExamples() const
{
    return {
        "5! (factorial)",
        "2^8 (power)",
        "sqrt(16) (square root)",
        "10!",
        "3.5^2",
        "sqrt(25)"
    };
}

double BasicMathCalculator::factorial(int n) const
{
    if (n <= 1) return 1.0;
    double result = 1.0;
    for (int i = 2; i <= n; ++i) {
        result *= i;
    }
    return result;
}

double BasicMathCalculator::power(double base, double exponent) const
{
    return std::pow(base, exponent);
}
