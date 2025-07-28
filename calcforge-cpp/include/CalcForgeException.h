#ifndef CALCFORGEEXCEPTION_H
#define CALCFORGEEXCEPTION_H

#include <stdexcept>
#include <QString>

/**
 * Base exception class for all CalcForge calculator operations
 * Replaces TimecodeError, AspectRatioError, DateError, CurrencyError, 
 * PercentageError, and UnitConversionError with a unified exception hierarchy
 */
class CalcForgeException : public std::runtime_error
{
public:
    explicit CalcForgeException(const QString& message) 
        : std::runtime_error(message.toStdString()), m_message(message) {}
    
    explicit CalcForgeException(const char* message) 
        : std::runtime_error(message), m_message(QString::fromUtf8(message)) {}
    
    virtual ~CalcForgeException() noexcept = default;
    
    // Get the original QString message (preserves Unicode)
    const QString& qMessage() const noexcept { return m_message; }
    
    // Get the calculator type that generated this exception
    virtual QString calculatorType() const { return "Generic"; }

private:
    QString m_message;
};

/**
 * Specific exception types for different calculator modules
 * These provide better error categorization while maintaining the common interface
 */

class UnitConversionException : public CalcForgeException
{
public:
    explicit UnitConversionException(const QString& message) 
        : CalcForgeException(message) {}
    
    QString calculatorType() const override { return "UnitConverter"; }
};

class TimecodeException : public CalcForgeException
{
public:
    explicit TimecodeException(const QString& message) 
        : CalcForgeException(message) {}
    
    QString calculatorType() const override { return "TimecodeCalculator"; }
};

class AspectRatioException : public CalcForgeException
{
public:
    explicit AspectRatioException(const QString& message) 
        : CalcForgeException(message) {}
    
    QString calculatorType() const override { return "AspectRatioCalculator"; }
};

class DateException : public CalcForgeException
{
public:
    explicit DateException(const QString& message) 
        : CalcForgeException(message) {}
    
    QString calculatorType() const override { return "DateCalculator"; }
};

class CurrencyException : public CalcForgeException
{
public:
    explicit CurrencyException(const QString& message) 
        : CalcForgeException(message) {}
    
    QString calculatorType() const override { return "CurrencyConverter"; }
};

class PercentageException : public CalcForgeException
{
public:
    explicit PercentageException(const QString& message) 
        : CalcForgeException(message) {}
    
    QString calculatorType() const override { return "PercentageCalculator"; }
};

class ExpressionParsingException : public CalcForgeException
{
public:
    explicit ExpressionParsingException(const QString& message) 
        : CalcForgeException(message) {}
    
    QString calculatorType() const override { return "ExpressionParser"; }
};

// Convenience macros for common exception patterns
#define CALCFORGE_THROW_UNIT(msg) throw UnitConversionException(QString(msg))
#define CALCFORGE_THROW_TIMECODE(msg) throw TimecodeException(QString(msg))
#define CALCFORGE_THROW_ASPECTRATIO(msg) throw AspectRatioException(QString(msg))
#define CALCFORGE_THROW_DATE(msg) throw DateException(QString(msg))
#define CALCFORGE_THROW_CURRENCY(msg) throw CurrencyException(QString(msg))
#define CALCFORGE_THROW_PERCENTAGE(msg) throw PercentageException(QString(msg))
#define CALCFORGE_THROW_PARSING(msg) throw ExpressionParsingException(QString(msg))

#endif // CALCFORGEEXCEPTION_H