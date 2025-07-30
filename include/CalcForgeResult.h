#ifndef CALCFORGERESULT_H
#define CALCFORGERESULT_H

#include <QString>
#include <utility>

/**
 * Generic result template for all CalcForge calculator operations
 * Eliminates code duplication across UnitConversionResult, TimecodeResult, 
 * AspectRatioResult, DateResult, CurrencyResult, and PercentageResult
 * 
 * Template parameter T represents the type of the result value:
 * - std::pair<double, QString> for unit conversions (value + unit)
 * - QString for timecode, aspect ratio, date calculations
 * - double for percentage calculations
 */
template<typename T>
class CalcForgeResult
{
public:
    // Constructors
    CalcForgeResult() : m_isValid(false) {}
    
    explicit CalcForgeResult(const T& value) 
        : m_value(value), m_isValid(true) {}
    
    // Error constructor - uses tag dispatch to avoid ambiguity with T=QString
    struct ErrorTag {};
    CalcForgeResult(ErrorTag, const QString& errorMessage) 
        : m_isValid(false), m_errorMessage(errorMessage) {}
    
    // Copy constructor and assignment operator
    CalcForgeResult(const CalcForgeResult& other) = default;
    CalcForgeResult& operator=(const CalcForgeResult& other) = default;
    
    // Move constructor and assignment operator
    CalcForgeResult(CalcForgeResult&& other) noexcept = default;
    CalcForgeResult& operator=(CalcForgeResult&& other) noexcept = default;
    
    // Accessors
    const T& value() const { return m_value; }
    T& value() { return m_value; }
    
    bool isValid() const { return m_isValid; }
    const QString& errorMessage() const { return m_errorMessage; }
    
    // Setters
    void setValue(const T& value) { 
        m_value = value; 
        m_isValid = true; 
        m_errorMessage.clear();
    }
    
    void setError(const QString& errorMessage) { 
        m_isValid = false; 
        m_errorMessage = errorMessage; 
        m_value = T{}; // Reset to default value
    }
    
    // Convenience operators
    explicit operator bool() const { return m_isValid; }
    
    // Factory methods for common result types
    static CalcForgeResult<T> success(const T& value) {
        return CalcForgeResult<T>(value);
    }
    
    static CalcForgeResult<T> error(const QString& errorMessage) {
        return CalcForgeResult<T>(ErrorTag{}, errorMessage);
    }

private:
    T m_value{};
    bool m_isValid;
    QString m_errorMessage;
};

// Type aliases for specific calculator results
using UnitConversionResult = CalcForgeResult<std::pair<double, QString>>;
using TimecodeResult = CalcForgeResult<QString>;
using AspectRatioResult = CalcForgeResult<QString>;
using DateResult = CalcForgeResult<QString>;
using CurrencyResult = CalcForgeResult<std::pair<double, QString>>;
using PercentageResult = CalcForgeResult<double>;

// Convenience functions for unit conversion results
inline UnitConversionResult makeUnitResult(double value, const QString& unit) {
    return UnitConversionResult::success(std::make_pair(value, unit));
}

inline UnitConversionResult makeUnitError(const QString& error) {
    return UnitConversionResult::error(error);
}

// Convenience functions for currency conversion results  
inline CurrencyResult makeCurrencyResult(double value, const QString& currency) {
    return CurrencyResult::success(std::make_pair(value, currency));
}

inline CurrencyResult makeCurrencyError(const QString& error) {
    return CurrencyResult::error(error);
}

#endif // CALCFORGERESULT_H