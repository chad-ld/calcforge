#ifndef UNITCONVERTER_H
#define UNITCONVERTER_H

#include <QString>
#include <QHash>
#include <QRegularExpression>
#include <cmath>

/**
 * Unit conversion result structure
 * Contains the converted value and the target unit name for display
 */
struct UnitConversionResult {
    double value;
    QString unit;
    bool isValid;
    QString errorMessage;

    UnitConversionResult() : value(0.0), isValid(false) {}
    UnitConversionResult(double val, const QString& unitName)
        : value(val), unit(unitName), isValid(true) {}
    UnitConversionResult(const QString& error)
        : value(0.0), isValid(false), errorMessage(error) {}
};

/**
 * Comprehensive unit conversion system for CalcForge C++
 * Replaces Python Pint library functionality with native C++ implementation
 * Supports distance, weight, volume, temperature, and time conversions
 */
class UnitConverter
{
public:
    UnitConverter();
    
    /**
     * Convert a unit expression like "5 feet to meters"
     * @param expression The conversion expression to parse and evaluate
     * @return UnitConversionResult with converted value and unit, or invalid result
     */
    UnitConversionResult convertExpression(const QString &expression);
    
    /**
     * Convert between two specific units
     * @param value The numeric value to convert
     * @param fromUnit Source unit (e.g., "feet", "ft")
     * @param toUnit Target unit (e.g., "meters", "m")
     * @return UnitConversionResult with converted value and unit, or invalid result
     */
    UnitConversionResult convert(double value, const QString &fromUnit, const QString &toUnit);
    
    /**
     * Check if a string represents a valid unit
     * @param unit The unit string to validate
     * @return True if the unit is recognized
     */
    bool isValidUnit(const QString &unit) const;
    
    /**
     * Get the display name for a unit abbreviation
     * @param unitAbbr The unit abbreviation (e.g., "m", "ft")
     * @return Full unit name for display (e.g., "meters", "feet")
     */
    QString getDisplayName(const QString &unitAbbr) const;
    
    /**
     * Get the standard abbreviation for a unit name
     * @param unitName The unit name (e.g., "meters", "feet")
     * @return Standard abbreviation (e.g., "m", "ft")
     */
    QString getAbbreviation(const QString &unitName) const;

private:
    /**
     * Initialize all unit definitions and conversion factors
     */
    void initializeUnits();
    
    /**
     * Initialize distance/length units
     */
    void initializeDistanceUnits();
    
    /**
     * Initialize weight/mass units
     */
    void initializeWeightUnits();
    
    /**
     * Initialize volume units
     */
    void initializeVolumeUnits();
    
    /**
     * Initialize temperature units
     */
    void initializeTemperatureUnits();
    
    /**
     * Initialize time units
     */
    void initializeTimeUnits();
    
    /**
     * Convert between units within the same category
     * @param value Value to convert
     * @param fromUnit Source unit abbreviation
     * @param toUnit Target unit abbreviation
     * @param category Unit category (distance, weight, volume, etc.)
     * @return Converted value, or NaN if conversion not possible
     */
    double convertWithinCategory(double value, const QString &fromUnit, 
                                const QString &toUnit, const QString &category);
    
    /**
     * Handle temperature conversions (special case due to offset formulas)
     * @param value Temperature value to convert
     * @param fromUnit Source temperature unit
     * @param toUnit Target temperature unit
     * @return Converted temperature value
     */
    double convertTemperature(double value, const QString &fromUnit, const QString &toUnit);
    
    /**
     * Normalize unit name to standard abbreviation
     * @param unit Input unit name or abbreviation
     * @return Standard abbreviation, or empty string if not found
     */
    QString normalizeUnit(const QString &unit) const;
    
    /**
     * Get the category for a given unit
     * @param unit Unit abbreviation
     * @return Category name (distance, weight, volume, temperature, time)
     */
    QString getUnitCategory(const QString &unit) const;

private:
    // Unit name to abbreviation mapping (e.g., "meters" -> "m")
    QHash<QString, QString> m_unitAbbreviations;
    
    // Abbreviation to display name mapping (e.g., "m" -> "meters")
    QHash<QString, QString> m_displayNames;
    
    // Unit to category mapping (e.g., "m" -> "distance")
    QHash<QString, QString> m_unitCategories;
    
    // Conversion factors to base units for each category
    // Distance: base unit is meters
    QHash<QString, double> m_distanceFactors;
    
    // Weight: base unit is kilograms
    QHash<QString, double> m_weightFactors;
    
    // Volume: base unit is liters
    QHash<QString, double> m_volumeFactors;
    
    // Time: base unit is seconds
    QHash<QString, double> m_timeFactors;
    
    // Temperature units (handled separately due to offset formulas)
    QHash<QString, QString> m_temperatureUnits;
};

#endif // UNITCONVERTER_H
