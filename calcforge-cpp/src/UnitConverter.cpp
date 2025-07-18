#include "UnitConverter.h"
#include "Logger.h"
#include <QDebug>
#include <QRegularExpression>

UnitConverter::UnitConverter()
{
    initializeUnits();
}

UnitConversionResult UnitConverter::convertExpression(const QString &expression)
{
    // Pattern to match unit conversions like "5 feet to meters"
    QRegularExpression pattern(R"(^([\d.]+)\s+(.+?)\s+to\s+(.+?)$)", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match = pattern.match(expression.trimmed());

    if (match.hasMatch()) {
        bool ok;
        double value = match.captured(1).toDouble(&ok);
        if (!ok) {
            LOG_DEBUG(QString("Invalid numeric value in unit conversion: %1").arg(match.captured(1)));
            return UnitConversionResult(QString("Error: Invalid number '%1'").arg(match.captured(1)));
        }

        QString fromUnit = match.captured(2).trimmed().toLower();
        QString toUnit = match.captured(3).trimmed().toLower();

        return convert(value, fromUnit, toUnit);
    }

    return UnitConversionResult();
}

UnitConversionResult UnitConverter::convert(double value, const QString &fromUnit, const QString &toUnit)
{
    // Normalize unit names to standard abbreviations
    QString fromAbbr = normalizeUnit(fromUnit.toLower());
    QString toAbbr = normalizeUnit(toUnit.toLower());

    if (fromAbbr.isEmpty()) {
        LOG_DEBUG(QString("Unknown from unit in conversion: %1").arg(fromUnit));
        return UnitConversionResult(QString("Error: Unknown unit '%1'").arg(fromUnit));
    }

    if (toAbbr.isEmpty()) {
        LOG_DEBUG(QString("Unknown to unit in conversion: %1").arg(toUnit));
        return UnitConversionResult(QString("Error: Unknown unit '%1'").arg(toUnit));
    }

    // Check if units are in the same category
    QString fromCategory = getUnitCategory(fromAbbr);
    QString toCategory = getUnitCategory(toAbbr);

    if (fromCategory.isEmpty() || toCategory.isEmpty() || fromCategory != toCategory) {
        LOG_DEBUG(QString("Units not in same category: %1 (%2) to %3 (%4)")
                  .arg(fromUnit).arg(fromCategory).arg(toUnit).arg(toCategory));
        return UnitConversionResult(QString("Error: Cannot convert %1 to %2 (different unit types)").arg(fromUnit).arg(toUnit));
    }

    double convertedValue;

    // Handle temperature conversions specially
    if (fromCategory == "temperature") {
        convertedValue = convertTemperature(value, fromAbbr, toAbbr);
    } else {
        convertedValue = convertWithinCategory(value, fromAbbr, toAbbr, fromCategory);
    }

    if (std::isnan(convertedValue)) {
        return UnitConversionResult(QString("Error: Conversion failed for %1 to %2").arg(fromUnit).arg(toUnit));
    }

    // Get display name for result
    QString displayName = getDisplayName(toAbbr);

    return UnitConversionResult(convertedValue, displayName);
}

bool UnitConverter::isValidUnit(const QString &unit) const
{
    return !normalizeUnit(unit.toLower()).isEmpty();
}

QString UnitConverter::getDisplayName(const QString &unitAbbr) const
{
    return m_displayNames.value(unitAbbr, unitAbbr);
}

QString UnitConverter::getAbbreviation(const QString &unitName) const
{
    return m_unitAbbreviations.value(unitName.toLower(), QString());
}

void UnitConverter::initializeUnits()
{
    initializeDistanceUnits();
    initializeAreaUnits();
    initializeWeightUnits();
    initializeVolumeUnits();
    initializeTemperatureUnits();
    initializeTimeUnits();
}

void UnitConverter::initializeDistanceUnits()
{
    // Distance unit abbreviations (name -> abbreviation)
    m_unitAbbreviations["meter"] = "m";
    m_unitAbbreviations["meters"] = "m";
    m_unitAbbreviations["m"] = "m";
    m_unitAbbreviations["kilometer"] = "km";
    m_unitAbbreviations["kilometers"] = "km";
    m_unitAbbreviations["km"] = "km";
    m_unitAbbreviations["mile"] = "mi";
    m_unitAbbreviations["miles"] = "mi";
    m_unitAbbreviations["mi"] = "mi";
    m_unitAbbreviations["yard"] = "yd";
    m_unitAbbreviations["yards"] = "yd";
    m_unitAbbreviations["yd"] = "yd";
    m_unitAbbreviations["foot"] = "ft";
    m_unitAbbreviations["feet"] = "ft";
    m_unitAbbreviations["ft"] = "ft";
    m_unitAbbreviations["inch"] = "in";
    m_unitAbbreviations["inches"] = "in";
    m_unitAbbreviations["in"] = "in";
    m_unitAbbreviations["centimeter"] = "cm";
    m_unitAbbreviations["centimeters"] = "cm";
    m_unitAbbreviations["cm"] = "cm";
    m_unitAbbreviations["millimeter"] = "mm";
    m_unitAbbreviations["millimeters"] = "mm";
    m_unitAbbreviations["mm"] = "mm";
    
    // Display names (abbreviation -> display name)
    m_displayNames["m"] = "meters";
    m_displayNames["km"] = "kilometers";
    m_displayNames["mi"] = "miles";
    m_displayNames["yd"] = "yards";
    m_displayNames["ft"] = "feet";
    m_displayNames["in"] = "inches";
    m_displayNames["cm"] = "centimeters";
    m_displayNames["mm"] = "millimeters";
    
    // Categories
    m_unitCategories["m"] = "distance";
    m_unitCategories["km"] = "distance";
    m_unitCategories["mi"] = "distance";
    m_unitCategories["yd"] = "distance";
    m_unitCategories["ft"] = "distance";
    m_unitCategories["in"] = "distance";
    m_unitCategories["cm"] = "distance";
    m_unitCategories["mm"] = "distance";
    
    // Conversion factors to meters
    m_distanceFactors["m"] = 1.0;
    m_distanceFactors["km"] = 1000.0;
    m_distanceFactors["mi"] = 1609.344;
    m_distanceFactors["yd"] = 0.9144;
    m_distanceFactors["ft"] = 0.3048;
    m_distanceFactors["in"] = 0.0254;
    m_distanceFactors["cm"] = 0.01;
    m_distanceFactors["mm"] = 0.001;
}

void UnitConverter::initializeAreaUnits()
{
    // Area unit abbreviations
    m_unitAbbreviations["square meter"] = "m²";
    m_unitAbbreviations["square meters"] = "m²";
    m_unitAbbreviations["sq m"] = "m²";
    m_unitAbbreviations["m²"] = "m²";
    m_unitAbbreviations["m^2"] = "m²";

    m_unitAbbreviations["square kilometer"] = "km²";
    m_unitAbbreviations["square kilometers"] = "km²";
    m_unitAbbreviations["sq km"] = "km²";
    m_unitAbbreviations["km²"] = "km²";
    m_unitAbbreviations["km^2"] = "km²";

    m_unitAbbreviations["square foot"] = "ft²";
    m_unitAbbreviations["square feet"] = "ft²";
    m_unitAbbreviations["sq ft"] = "ft²";
    m_unitAbbreviations["ft²"] = "ft²";
    m_unitAbbreviations["ft^2"] = "ft²";

    m_unitAbbreviations["square inch"] = "in²";
    m_unitAbbreviations["square inches"] = "in²";
    m_unitAbbreviations["sq in"] = "in²";
    m_unitAbbreviations["in²"] = "in²";
    m_unitAbbreviations["in^2"] = "in²";

    m_unitAbbreviations["square yard"] = "yd²";
    m_unitAbbreviations["square yards"] = "yd²";
    m_unitAbbreviations["sq yd"] = "yd²";
    m_unitAbbreviations["yd²"] = "yd²";
    m_unitAbbreviations["yd^2"] = "yd²";

    m_unitAbbreviations["square mile"] = "mi²";
    m_unitAbbreviations["square miles"] = "mi²";
    m_unitAbbreviations["sq mi"] = "mi²";
    m_unitAbbreviations["mi²"] = "mi²";
    m_unitAbbreviations["mi^2"] = "mi²";

    m_unitAbbreviations["acre"] = "acre";
    m_unitAbbreviations["acres"] = "acre";

    m_unitAbbreviations["hectare"] = "ha";
    m_unitAbbreviations["hectares"] = "ha";
    m_unitAbbreviations["ha"] = "ha";

    // Display names
    m_displayNames["m²"] = "square meters";
    m_displayNames["km²"] = "square kilometers";
    m_displayNames["ft²"] = "square feet";
    m_displayNames["in²"] = "square inches";
    m_displayNames["yd²"] = "square yards";
    m_displayNames["mi²"] = "square miles";
    m_displayNames["acre"] = "acres";
    m_displayNames["ha"] = "hectares";

    // Categories
    m_unitCategories["m²"] = "area";
    m_unitCategories["km²"] = "area";
    m_unitCategories["ft²"] = "area";
    m_unitCategories["in²"] = "area";
    m_unitCategories["yd²"] = "area";
    m_unitCategories["mi²"] = "area";
    m_unitCategories["acre"] = "area";
    m_unitCategories["ha"] = "area";

    // Conversion factors to square meters
    m_areaFactors["m²"] = 1.0;
    m_areaFactors["km²"] = 1000000.0;  // 1 km² = 1,000,000 m²
    m_areaFactors["ft²"] = 0.092903;   // 1 ft² = 0.092903 m²
    m_areaFactors["in²"] = 0.00064516; // 1 in² = 0.00064516 m²
    m_areaFactors["yd²"] = 0.836127;   // 1 yd² = 0.836127 m²
    m_areaFactors["mi²"] = 2589988.11; // 1 mi² = 2,589,988.11 m²
    m_areaFactors["acre"] = 4046.86;   // 1 acre = 4,046.86 m²
    m_areaFactors["ha"] = 10000.0;     // 1 hectare = 10,000 m²
}

void UnitConverter::initializeWeightUnits()
{
    // Weight unit abbreviations
    m_unitAbbreviations["pound"] = "lb";
    m_unitAbbreviations["pounds"] = "lb";
    m_unitAbbreviations["lb"] = "lb";
    m_unitAbbreviations["lbs"] = "lb";
    m_unitAbbreviations["kilogram"] = "kg";
    m_unitAbbreviations["kilograms"] = "kg";
    m_unitAbbreviations["kg"] = "kg";
    m_unitAbbreviations["gram"] = "g";
    m_unitAbbreviations["grams"] = "g";
    m_unitAbbreviations["g"] = "g";
    m_unitAbbreviations["ounce"] = "oz";
    m_unitAbbreviations["ounces"] = "oz";
    m_unitAbbreviations["oz"] = "oz";
    m_unitAbbreviations["ton"] = "t";
    m_unitAbbreviations["tons"] = "t";
    m_unitAbbreviations["t"] = "t";
    
    // Display names
    m_displayNames["lb"] = "pounds";
    m_displayNames["kg"] = "kilograms";
    m_displayNames["g"] = "grams";
    m_displayNames["oz"] = "ounces";
    m_displayNames["t"] = "tons";
    
    // Categories
    m_unitCategories["lb"] = "weight";
    m_unitCategories["kg"] = "weight";
    m_unitCategories["g"] = "weight";
    m_unitCategories["oz"] = "weight";
    m_unitCategories["t"] = "weight";
    
    // Conversion factors to kilograms
    m_weightFactors["kg"] = 1.0;
    m_weightFactors["lb"] = 0.453592;
    m_weightFactors["g"] = 0.001;
    m_weightFactors["oz"] = 0.0283495;
    m_weightFactors["t"] = 1000.0;
}

void UnitConverter::initializeVolumeUnits()
{
    // Volume unit abbreviations
    m_unitAbbreviations["liter"] = "L";
    m_unitAbbreviations["liters"] = "L";
    m_unitAbbreviations["litre"] = "L";
    m_unitAbbreviations["litres"] = "L";
    m_unitAbbreviations["l"] = "L";
    m_unitAbbreviations["L"] = "L";
    m_unitAbbreviations["gallon"] = "gal";
    m_unitAbbreviations["gallons"] = "gal";
    m_unitAbbreviations["gal"] = "gal";
    m_unitAbbreviations["quart"] = "qt";
    m_unitAbbreviations["quarts"] = "qt";
    m_unitAbbreviations["qt"] = "qt";
    m_unitAbbreviations["pint"] = "pt";
    m_unitAbbreviations["pints"] = "pt";
    m_unitAbbreviations["pt"] = "pt";
    m_unitAbbreviations["cup"] = "cup";
    m_unitAbbreviations["cups"] = "cup";
    m_unitAbbreviations["milliliter"] = "mL";
    m_unitAbbreviations["milliliters"] = "mL";
    m_unitAbbreviations["ml"] = "mL";
    m_unitAbbreviations["mL"] = "mL";
    m_unitAbbreviations["fluid ounce"] = "fl oz";
    m_unitAbbreviations["fluid ounces"] = "fl oz";
    m_unitAbbreviations["fl oz"] = "fl oz";
    m_unitAbbreviations["floz"] = "fl oz";

    // Cubic volume units
    m_unitAbbreviations["cubic meter"] = "m³";
    m_unitAbbreviations["cubic meters"] = "m³";
    m_unitAbbreviations["cu m"] = "m³";
    m_unitAbbreviations["m³"] = "m³";
    m_unitAbbreviations["m^3"] = "m³";

    m_unitAbbreviations["cubic foot"] = "ft³";
    m_unitAbbreviations["cubic feet"] = "ft³";
    m_unitAbbreviations["cu ft"] = "ft³";
    m_unitAbbreviations["ft³"] = "ft³";
    m_unitAbbreviations["ft^3"] = "ft³";

    m_unitAbbreviations["cubic inch"] = "in³";
    m_unitAbbreviations["cubic inches"] = "in³";
    m_unitAbbreviations["cu in"] = "in³";
    m_unitAbbreviations["in³"] = "in³";
    m_unitAbbreviations["in^3"] = "in³";

    m_unitAbbreviations["cubic yard"] = "yd³";
    m_unitAbbreviations["cubic yards"] = "yd³";
    m_unitAbbreviations["cu yd"] = "yd³";
    m_unitAbbreviations["yd³"] = "yd³";
    m_unitAbbreviations["yd^3"] = "yd³";

    m_unitAbbreviations["cubic centimeter"] = "cm³";
    m_unitAbbreviations["cubic centimeters"] = "cm³";
    m_unitAbbreviations["cu cm"] = "cm³";
    m_unitAbbreviations["cm³"] = "cm³";
    m_unitAbbreviations["cm^3"] = "cm³";
    m_unitAbbreviations["cc"] = "cm³";

    // Display names
    m_displayNames["L"] = "liters";
    m_displayNames["gal"] = "gallons";
    m_displayNames["qt"] = "quarts";
    m_displayNames["pt"] = "pints";
    m_displayNames["cup"] = "cups";
    m_displayNames["mL"] = "milliliters";
    m_displayNames["fl oz"] = "fluid ounces";
    m_displayNames["m³"] = "cubic meters";
    m_displayNames["ft³"] = "cubic feet";
    m_displayNames["in³"] = "cubic inches";
    m_displayNames["yd³"] = "cubic yards";
    m_displayNames["cm³"] = "cubic centimeters";

    // Categories
    m_unitCategories["L"] = "volume";
    m_unitCategories["gal"] = "volume";
    m_unitCategories["qt"] = "volume";
    m_unitCategories["pt"] = "volume";
    m_unitCategories["cup"] = "volume";
    m_unitCategories["mL"] = "volume";
    m_unitCategories["fl oz"] = "volume";
    m_unitCategories["m³"] = "volume";
    m_unitCategories["ft³"] = "volume";
    m_unitCategories["in³"] = "volume";
    m_unitCategories["yd³"] = "volume";
    m_unitCategories["cm³"] = "volume";

    // Conversion factors to liters
    m_volumeFactors["L"] = 1.0;
    m_volumeFactors["gal"] = 3.78541;  // US gallon
    m_volumeFactors["qt"] = 0.946353;  // US quart
    m_volumeFactors["pt"] = 0.473176;  // US pint
    m_volumeFactors["cup"] = 0.236588; // US cup
    m_volumeFactors["mL"] = 0.001;
    m_volumeFactors["fl oz"] = 0.0295735; // US fluid ounce

    // Cubic volume units (converted to liters)
    m_volumeFactors["m³"] = 1000.0;      // 1 m³ = 1,000 L
    m_volumeFactors["ft³"] = 28.3168;    // 1 ft³ = 28.3168 L
    m_volumeFactors["in³"] = 0.0163871;  // 1 in³ = 0.0163871 L
    m_volumeFactors["yd³"] = 764.555;    // 1 yd³ = 764.555 L
    m_volumeFactors["cm³"] = 0.001;      // 1 cm³ = 0.001 L (same as mL)
}

void UnitConverter::initializeTemperatureUnits()
{
    // Temperature unit abbreviations
    m_unitAbbreviations["celsius"] = "C";
    m_unitAbbreviations["c"] = "C";
    m_unitAbbreviations["C"] = "C";  // Uppercase C
    m_unitAbbreviations["fahrenheit"] = "F";
    m_unitAbbreviations["f"] = "F";
    m_unitAbbreviations["F"] = "F";  // Uppercase F
    m_unitAbbreviations["kelvin"] = "K";
    m_unitAbbreviations["k"] = "K";
    m_unitAbbreviations["K"] = "K";  // Uppercase K

    // Display names
    m_displayNames["C"] = "Celsius";
    m_displayNames["F"] = "Fahrenheit";
    m_displayNames["K"] = "Kelvin";

    // Categories
    m_unitCategories["C"] = "temperature";
    m_unitCategories["F"] = "temperature";
    m_unitCategories["K"] = "temperature";

    // Temperature units for reference
    m_temperatureUnits["C"] = "Celsius";
    m_temperatureUnits["F"] = "Fahrenheit";
    m_temperatureUnits["K"] = "Kelvin";
}

void UnitConverter::initializeTimeUnits()
{
    // Time unit abbreviations
    m_unitAbbreviations["second"] = "s";
    m_unitAbbreviations["seconds"] = "s";
    m_unitAbbreviations["s"] = "s";
    m_unitAbbreviations["minute"] = "min";
    m_unitAbbreviations["minutes"] = "min";
    m_unitAbbreviations["min"] = "min";
    m_unitAbbreviations["hour"] = "h";
    m_unitAbbreviations["hours"] = "h";
    m_unitAbbreviations["h"] = "h";
    m_unitAbbreviations["day"] = "d";
    m_unitAbbreviations["days"] = "d";
    m_unitAbbreviations["d"] = "d";
    m_unitAbbreviations["week"] = "w";
    m_unitAbbreviations["weeks"] = "w";
    m_unitAbbreviations["w"] = "w";
    
    // Display names
    m_displayNames["s"] = "seconds";
    m_displayNames["min"] = "minutes";
    m_displayNames["h"] = "hours";
    m_displayNames["d"] = "days";
    m_displayNames["w"] = "weeks";
    
    // Categories
    m_unitCategories["s"] = "time";
    m_unitCategories["min"] = "time";
    m_unitCategories["h"] = "time";
    m_unitCategories["d"] = "time";
    m_unitCategories["w"] = "time";
    
    // Conversion factors to seconds
    m_timeFactors["s"] = 1.0;
    m_timeFactors["min"] = 60.0;
    m_timeFactors["h"] = 3600.0;
    m_timeFactors["d"] = 86400.0;
    m_timeFactors["w"] = 604800.0;
}

double UnitConverter::convertWithinCategory(double value, const QString &fromUnit,
                                           const QString &toUnit, const QString &category)
{
    if (category == "distance") {
        if (!m_distanceFactors.contains(fromUnit) || !m_distanceFactors.contains(toUnit)) {
            return std::numeric_limits<double>::quiet_NaN();
        }
        // Convert to base unit (meters) then to target unit
        double baseValue = value * m_distanceFactors[fromUnit];
        return baseValue / m_distanceFactors[toUnit];
    }
    else if (category == "area") {
        if (!m_areaFactors.contains(fromUnit) || !m_areaFactors.contains(toUnit)) {
            return std::numeric_limits<double>::quiet_NaN();
        }
        // Convert to base unit (square meters) then to target unit
        double baseValue = value * m_areaFactors[fromUnit];
        return baseValue / m_areaFactors[toUnit];
    }
    else if (category == "weight") {
        if (!m_weightFactors.contains(fromUnit) || !m_weightFactors.contains(toUnit)) {
            return std::numeric_limits<double>::quiet_NaN();
        }
        // Convert to base unit (kilograms) then to target unit
        double baseValue = value * m_weightFactors[fromUnit];
        return baseValue / m_weightFactors[toUnit];
    }
    else if (category == "volume") {
        if (!m_volumeFactors.contains(fromUnit) || !m_volumeFactors.contains(toUnit)) {
            return std::numeric_limits<double>::quiet_NaN();
        }
        // Convert to base unit (liters) then to target unit
        double baseValue = value * m_volumeFactors[fromUnit];
        return baseValue / m_volumeFactors[toUnit];
    }
    else if (category == "time") {
        if (!m_timeFactors.contains(fromUnit) || !m_timeFactors.contains(toUnit)) {
            return std::numeric_limits<double>::quiet_NaN();
        }
        // Convert to base unit (seconds) then to target unit
        double baseValue = value * m_timeFactors[fromUnit];
        return baseValue / m_timeFactors[toUnit];
    }

    return std::numeric_limits<double>::quiet_NaN();
}

double UnitConverter::convertTemperature(double value, const QString &fromUnit, const QString &toUnit)
{
    // Handle temperature conversions with offset formulas
    if (fromUnit == toUnit) {
        return value;
    }

    // Convert to Celsius first, then to target unit
    double celsius = value;

    // Convert from source unit to Celsius
    if (fromUnit == "F") {
        celsius = (value - 32.0) * 5.0 / 9.0;
    } else if (fromUnit == "K") {
        celsius = value - 273.15;
    }
    // If fromUnit is "C", celsius is already correct

    // Convert from Celsius to target unit
    if (toUnit == "F") {
        return celsius * 9.0 / 5.0 + 32.0;
    } else if (toUnit == "K") {
        return celsius + 273.15;
    } else if (toUnit == "C") {
        return celsius;
    }

    return std::numeric_limits<double>::quiet_NaN();
}

QString UnitConverter::normalizeUnit(const QString &unit) const
{
    return m_unitAbbreviations.value(unit.toLower(), QString());
}

QString UnitConverter::getUnitCategory(const QString &unit) const
{
    return m_unitCategories.value(unit, QString());
}
